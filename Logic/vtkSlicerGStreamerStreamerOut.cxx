#include "vtkSlicerGStreamerStreamerOut.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLVolumeNode.h"
#include "vtkMRMLAbstractViewNode.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkWindowToImageFilter.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"
#include "qMRMLSliceWidget.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLSliceView.h"
#include "qMRMLThreeDView.h"
#include <gst/app/gstappsrc.h>
#include <sstream>
#include <vector>

namespace
{
bool IsLikelyBlankRgbFrame(vtkImageData* imageData)
{
  if (!imageData || imageData->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    return false;
  }

  int dims[3] = {0, 0, 0};
  imageData->GetDimensions(dims);
  const int components = imageData->GetNumberOfScalarComponents();
  if (dims[0] <= 0 || dims[1] <= 0 || components < 3)
  {
    return true;
  }

  const size_t pixelCount = static_cast<size_t>(dims[0]) * static_cast<size_t>(dims[1]);
  unsigned char* ptr = static_cast<unsigned char*>(imageData->GetScalarPointer());
  if (!ptr)
  {
    return true;
  }

  // Sample pixels sparsely to keep this check cheap.
  const size_t step = (pixelCount > 2000) ? (pixelCount / 2000) : 1;
  size_t nonZero = 0;
  for (size_t i = 0; i < pixelCount; i += step)
  {
    const size_t off = i * static_cast<size_t>(components);
    if (ptr[off] != 0 || ptr[off + 1] != 0 || ptr[off + 2] != 0)
    {
      ++nonZero;
      if (nonZero > 4)
      {
        return false;
      }
    }
  }
  return true;
}
}

vtkStandardNewMacro(vtkSlicerGStreamerStreamerOut);

vtkSlicerGStreamerStreamerOut::vtkSlicerGStreamerStreamerOut()
  : Pipeline(nullptr)
  , AppSrc(nullptr)
  , MRMLScene(nullptr)
{
}

vtkSlicerGStreamerStreamerOut::~vtkSlicerGStreamerStreamerOut() { this->Stop(); }

void vtkSlicerGStreamerStreamerOut::SetMRMLScene(vtkMRMLScene* scene) { this->MRMLScene = scene; }

bool vtkSlicerGStreamerStreamerOut::Start(vtkMRMLGStreamerStreamerNode* node)
{
  if (!node || !node->GetUnixFDPath()) return false;
  this->Stop();
  this->StreamerNodeID = node->GetID();

  std::remove(node->GetUnixFDPath());
  std::stringstream ss;
  ss << "appsrc name=src is-live=true format=time ! videoconvert ! video/x-raw,format=I420 ! queue max-size-buffers=1 leaky=downstream ! unixfdsink socket-path=" << node->GetUnixFDPath() << " sync=false";

  GError* error = nullptr;
  this->Pipeline = gst_parse_launch(ss.str().c_str(), &error);
  if (error) { g_error_free(error); return false; }

  this->AppSrc = gst_bin_get_by_name(GST_BIN(this->Pipeline), "src");
  if (this->AppSrc)
  {
    // Use pipeline clock timestamps to avoid long-term drift from fixed-step PTS generation.
    g_object_set(this->AppSrc,
      "do-timestamp", TRUE,
      "is-live", TRUE,
      "format", GST_FORMAT_TIME,
      nullptr);
  }
  gst_element_set_state(this->Pipeline, GST_STATE_PLAYING);
  return true;
}

void vtkSlicerGStreamerStreamerOut::Stop()
{
  if (this->AppSrc)
  {
    gst_object_unref(this->AppSrc);
    this->AppSrc = nullptr;
  }
  if (this->Pipeline) {
    gst_element_set_state(this->Pipeline, GST_STATE_NULL);
    gst_object_unref(this->Pipeline);
    this->Pipeline = nullptr;
  }
}

void vtkSlicerGStreamerStreamerOut::PushFrame()
{
  if (!this->AppSrc || !this->MRMLScene) return;
  vtkMRMLGStreamerStreamerNode* node = vtkMRMLGStreamerStreamerNode::SafeDownCast(this->MRMLScene->GetNodeByID(this->StreamerNodeID));
  if (!node || !node->GetVideoNodeID()) return;

  vtkMRMLNode* videoNode = this->MRMLScene->GetNodeByID(node->GetVideoNodeID());
  if (!videoNode) return;

  vtkSmartPointer<vtkImageData> imageData;
  if (vtkMRMLVolumeNode* vol = vtkMRMLVolumeNode::SafeDownCast(videoNode)) {
    imageData = vol->GetImageData();
  } else if (videoNode->IsA("vtkMRMLAbstractViewNode")) {
    qSlicerLayoutManager* lm = qSlicerApplication::application()->layoutManager();
    if (lm) {
      QWidget* qw = lm->viewWidget(videoNode);
      vtkRenderWindow* rw = nullptr;
      if (qMRMLSliceWidget* sw = qobject_cast<qMRMLSliceWidget*>(qw)) rw = sw->sliceView() ? sw->sliceView()->renderWindow() : nullptr;
      else if (qMRMLThreeDWidget* tw = qobject_cast<qMRMLThreeDWidget*>(qw)) rw = tw->threeDView() ? tw->threeDView()->renderWindow() : nullptr;
      if (rw) {
        vtkNew<vtkWindowToImageFilter> wif;
        wif->SetInput(rw);
        wif->SetInputBufferTypeToRGB();
        rw->Render();

        // Front/back behavior differs across OpenGL backends; try front first, then back if needed.
        wif->ReadFrontBufferOn();
        wif->Update();
        imageData = wif->GetOutput();

        if (IsLikelyBlankRgbFrame(imageData))
        {
          wif->ReadFrontBufferOff();
          wif->Modified();
          wif->Update();
          imageData = wif->GetOutput();
        }
      }
    }
  }

  if (!imageData) return;
  int dims[3]; imageData->GetDimensions(dims);
  int nc = imageData->GetNumberOfScalarComponents();
  const int outW = (dims[0] / 2) * 2;
  const int outH = (dims[1] / 2) * 2;
  if (outW <= 0 || outH <= 0 || nc < 3)
  {
    return;
  }
  const size_t size = static_cast<size_t>(outW) * static_cast<size_t>(outH) * static_cast<size_t>(nc);

  unsigned char* src = static_cast<unsigned char*>(imageData->GetScalarPointer());
  if (!src)
  {
    return;
  }

  std::vector<unsigned char> cropped;
  std::vector<unsigned char> oriented;
  const unsigned char* payload = src;
  if (outW != dims[0] || outH != dims[1])
  {
    cropped.resize(size);
    const size_t srcRowBytes = static_cast<size_t>(dims[0]) * static_cast<size_t>(nc);
    const size_t dstRowBytes = static_cast<size_t>(outW) * static_cast<size_t>(nc);
    for (int y = 0; y < outH; ++y)
    {
      memcpy(cropped.data() + static_cast<size_t>(y) * dstRowBytes,
             src + static_cast<size_t>(y) * srcRowBytes,
             dstRowBytes);
    }
    payload = cropped.data();
  }

  // Flip rows to present top-left origin to downstream consumers.
  oriented.resize(size);
  const size_t rowBytes = static_cast<size_t>(outW) * static_cast<size_t>(nc);
  for (int y = 0; y < outH; ++y)
  {
    const size_t srcOffset = static_cast<size_t>(outH - 1 - y) * rowBytes;
    const size_t dstOffset = static_cast<size_t>(y) * rowBytes;
    memcpy(oriented.data() + dstOffset, payload + srcOffset, rowBytes);
  }
  payload = oriented.data();

  GstBuffer* buffer = gst_buffer_new_allocate(NULL, size, NULL);
  if (!buffer)
  {
    return;
  }

  GstMapInfo map;
  if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE))
  {
    gst_buffer_unref(buffer);
    return;
  }

  memcpy(map.data, payload, size);
  gst_buffer_unmap(buffer, &map);

  const char* format = (nc == 3) ? "RGB" : "RGBA";
  GstCaps* caps = gst_caps_new_simple("video/x-raw",
    "format", G_TYPE_STRING, format,
    "width", G_TYPE_INT, outW,
    "height", G_TYPE_INT, outH,
    "framerate", GST_TYPE_FRACTION, 30, 1,
    static_cast<void*>(nullptr));
  GstCaps* cur = gst_app_src_get_caps(GST_APP_SRC(this->AppSrc));
  if (!cur || !gst_caps_is_equal(cur, caps)) gst_app_src_set_caps(GST_APP_SRC(this->AppSrc), caps);
  if (cur) gst_caps_unref(cur);
  gst_caps_unref(caps);

  gst_app_src_push_buffer(GST_APP_SRC(this->AppSrc), buffer);
}
