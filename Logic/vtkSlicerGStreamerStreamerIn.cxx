#include "vtkSlicerGStreamerStreamerIn.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLStreamingVolumeNode.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include <gst/app/gstappsink.h>
#include <sstream>

vtkStandardNewMacro(vtkSlicerGStreamerStreamerIn);

vtkSlicerGStreamerStreamerIn::vtkSlicerGStreamerStreamerIn() : Pipeline(nullptr), AppSink(nullptr), MRMLScene(nullptr), HasPendingFrame(false) {}

vtkSlicerGStreamerStreamerIn::~vtkSlicerGStreamerStreamerIn() { this->Stop(); }

void vtkSlicerGStreamerStreamerIn::SetMRMLScene(vtkMRMLScene* scene) { this->MRMLScene = scene; }

bool vtkSlicerGStreamerStreamerIn::Start(vtkMRMLGStreamerStreamerNode* node)
{
  if (!node || !node->GetUnixFDPath()) return false;
  this->Stop();
  this->StreamerNodeID = node->GetID();

  std::stringstream ss;
  ss << "unixfdsrc socket-path=" << node->GetUnixFDPath() << " ! decodebin ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink emit-signals=true sync=false";

  GError* error = nullptr;
  this->Pipeline = gst_parse_launch(ss.str().c_str(), &error);
  if (error) { g_error_free(error); return false; }

  this->AppSink = gst_bin_get_by_name(GST_BIN(this->Pipeline), "sink");
  g_signal_connect(this->AppSink, "new-sample", G_CALLBACK(OnNewSample), this);
  gst_element_set_state(this->Pipeline, GST_STATE_PLAYING);
  return true;
}

void vtkSlicerGStreamerStreamerIn::Stop()
{
  if (this->Pipeline) {
    gst_element_set_state(this->Pipeline, GST_STATE_NULL);
    gst_object_unref(this->Pipeline);
    this->Pipeline = nullptr;
    this->AppSink = nullptr;
  }
}

GstFlowReturn vtkSlicerGStreamerStreamerIn::OnNewSample(GstElement* sink, gpointer data)
{
  vtkSlicerGStreamerStreamerIn* self = static_cast<vtkSlicerGStreamerStreamerIn*>(data);
  GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
  if (!sample) return GST_FLOW_OK;

  GstBuffer* buf = gst_sample_get_buffer(sample);
  GstCaps* caps = gst_sample_get_caps(sample);
  GstStructure* s = gst_caps_get_structure(caps, 0);
  int w, h;
  gst_structure_get_int(s, "width", &w);
  gst_structure_get_int(s, "height", &h);

  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_READ);
  {
    std::lock_guard<std::mutex> lock(self->FrameMutex);
    self->PendingFrame.Width = w;
    self->PendingFrame.Height = h;
    self->PendingFrame.PixelData.assign(map.data, map.data + map.size);
    self->HasPendingFrame = true;
  }
  gst_buffer_unmap(buf, &map);
  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

void vtkSlicerGStreamerStreamerIn::ProcessPendingFrames()
{
  bool hasFrame = false;
  FrameData localFrame;
  {
    std::lock_guard<std::mutex> lock(this->FrameMutex);
    if (this->HasPendingFrame) {
      localFrame = std::move(this->PendingFrame);
      this->HasPendingFrame = false;
      hasFrame = true;
    }
  }
  if (!hasFrame || !this->MRMLScene) return;

  vtkMRMLGStreamerStreamerNode* node = vtkMRMLGStreamerStreamerNode::SafeDownCast(this->MRMLScene->GetNodeByID(this->StreamerNodeID));
  if (!node || !node->GetVideoNodeID()) return;

  vtkMRMLStreamingVolumeNode* vol = vtkMRMLStreamingVolumeNode::SafeDownCast(this->MRMLScene->GetNodeByID(node->GetVideoNodeID()));
  if (!vol) return;

  vtkImageData* img = vol->GetImageData();
  if (!img || img->GetDimensions()[0] != localFrame.Width || img->GetDimensions()[1] != localFrame.Height) {
    vtkNew<vtkImageData> newImg;
    newImg->SetDimensions(localFrame.Width, localFrame.Height, 1);
    newImg->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
    vol->SetAndObserveImageData(newImg);
    img = vol->GetImageData();
  }
  memcpy(img->GetScalarPointer(), localFrame.PixelData.data(), localFrame.PixelData.size());
  img->Modified();
  vol->Modified();
}
