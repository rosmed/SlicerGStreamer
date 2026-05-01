
#include "vtkSlicerGStreamerLogic.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkMRMLScene.h"
#include "vtkObjectFactory.h"

#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <sstream>
#include <cstdio>
#include <vtkImageData.h>
#include <vtkMRMLVolumeNode.h> 
#include <vtkMRMLAbstractViewNode.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageFlip.h>
#include <vtkRenderWindow.h>
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qMRMLSliceWidget.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLSliceView.h>
#include <qMRMLThreeDView.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkMRMLStreamingVolumeNode.h>
#include <mutex>

vtkStandardNewMacro(vtkSlicerGStreamerLogic);

vtkSlicerGStreamerLogic::vtkSlicerGStreamerLogic()
{
  gst_init(NULL, NULL);
}

vtkSlicerGStreamerLogic::~vtkSlicerGStreamerLogic()
{
  for (auto const& [id, data] : this->ActivePipelines)
  {
    if (data.Pipeline)
    {
      gst_element_set_state(data.Pipeline, GST_STATE_NULL);
      gst_object_unref(data.Pipeline);
    }
  }
}

void vtkSlicerGStreamerLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkSlicerGStreamerLogic::StartStreaming(vtkMRMLGStreamerStreamerNode* node)
{
  if (!node || !node->GetID() || !node->GetUnixFDPath())
  {
    return false;
  }

  this->StopStreaming(node);

  if (!node->GetStreamIn())
  {
    // For Stream Out, remove stale socket file if it exists to prevent unixfdsink errors
    std::remove(node->GetUnixFDPath());
  }

  std::stringstream ss;
  if (node->GetStreamIn())
  {
    // Stream IN: decode -> queue (leaky) -> appsink
    ss << "unixfdsrc socket-path=" << node->GetUnixFDPath() << " ! decodebin ! videoconvert ! video/x-raw,format=RGB ! queue max-size-buffers=1 leaky=downstream ! appsink name=sink emit-signals=true sync=false";
  }
  else
  {
    // Stream OUT: appsrc -> raw video -> unixfdsink
    // We use RGB/RGBA formats directly from VTK to avoid videoconvert issues before the socket.
    ss << "appsrc name=src is-live=true format=time ! videoflip method=vertical-flip ! videoconvert ! video/x-raw,format=I420 ! unixfdsink socket-path=" << node->GetUnixFDPath() << " sync=true";
  }

  GError* error = nullptr;
  GstElement* pipeline = gst_parse_launch(ss.str().c_str(), &error);

  if (error)
  {
    vtkErrorMacro("GStreamer error: " << error->message);
    g_error_free(error);
    return false;
  }

  PipelineData data;
  data.Pipeline = pipeline;
  data.NodeID = node->GetID();
  data.AppSrc = nullptr;
  data.AppSink = nullptr;

  if (node->GetStreamIn())
  {
    data.AppSink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    data.AppSrc = nullptr;
    g_signal_connect(data.AppSink, "new-sample", G_CALLBACK(vtkSlicerGStreamerLogic::OnNewSample), this);
  }
  else
  {
    data.AppSrc = gst_bin_get_by_name(GST_BIN(pipeline), "src");
    data.AppSink = nullptr;
  }

  this->ActivePipelines[node->GetID()] = data;

  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  return true;
}

GstFlowReturn vtkSlicerGStreamerLogic::OnNewSample(GstElement* sink, gpointer data)
{
  vtkSlicerGStreamerLogic* self = static_cast<vtkSlicerGStreamerLogic*>(data);
  GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
  if (!sample) return GST_FLOW_OK;

  GstBuffer* buffer = gst_sample_get_buffer(sample);
  GstCaps* caps = gst_sample_get_caps(sample);
  GstStructure* s = gst_caps_get_structure(caps, 0);

  int width, height;
  gst_structure_get_int(s, "width", &width);
  gst_structure_get_int(s, "height", &height);

  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READ);

  // Buffer sample data to be processed in the main thread (ProcessMRMLNodes/ProcessPendingFrames)
  ImageDataUpdate update;
  update.Width = width;
  update.Height = height;
  update.PixelData.assign(map.data, map.data + map.size);

  // Find the node ID associated with this sink
  {
    std::lock_guard<std::mutex> lock(self->UpdatesMutex);
    for (auto const& [id, pdata] : self->ActivePipelines)
    {
      if (pdata.AppSink == sink)
      {
        vtkMRMLGStreamerStreamerNode* sNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(self->GetMRMLScene()->GetNodeByID(pdata.NodeID));
        if (sNode && sNode->GetVideoNodeID())
        {
          update.NodeID = sNode->GetVideoNodeID();
          // Keep only the latest frame if multiple arrive before the next main loop pull
          bool found = false;
          for (auto& pending : self->PendingUpdates) {
             if (pending.NodeID == update.NodeID) {
                 pending = update;
                 found = true;
                 break;
             }
          }
          if (!found) self->PendingUpdates.push_back(update);
        }
        break;
      }
    }
  }

  gst_buffer_unmap(buffer, &map);
  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

void vtkSlicerGStreamerLogic::ProcessPendingFrames()
{
  std::vector<ImageDataUpdate> localUpdates;
  {
    std::lock_guard<std::mutex> lock(this->UpdatesMutex);
    localUpdates = std::move(this->PendingUpdates);
    this->PendingUpdates.clear();
  }

  for (const auto& update : localUpdates)
  {
    vtkMRMLStreamingVolumeNode* volumeNode = vtkMRMLStreamingVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(update.NodeID));
    if (!volumeNode) continue;

    vtkImageData* imageData = volumeNode->GetImageData();
    if (!imageData || imageData->GetDimensions()[0] != update.Width || imageData->GetDimensions()[1] != update.Height)
    {
      vtkNew<vtkImageData> newImageData;
      newImageData->SetDimensions(update.Width, update.Height, 1);
      newImageData->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
      volumeNode->SetAndObserveImageData(newImageData);
      imageData = volumeNode->GetImageData();
    }
    
    memcpy(imageData->GetScalarPointer(), update.PixelData.data(), update.PixelData.size());
    imageData->Modified();
    volumeNode->Modified();
  }
}

void vtkSlicerGStreamerLogic::StopStreaming(vtkMRMLGStreamerStreamerNode* node)
{
  if (!node || !node->GetID())
  {
    return;
  }

  auto it = this->ActivePipelines.find(node->GetID());
  if (it != this->ActivePipelines.end())
  {
    gst_element_set_state(it->second.Pipeline, GST_STATE_NULL);
    gst_object_unref(it->second.Pipeline);
    this->ActivePipelines.erase(it);
  }
}

void vtkSlicerGStreamerLogic::ProcessMRMLNodes()
{
  this->ProcessPendingFrames();

  if (!this->GetMRMLScene())
  {
    return;
  }

  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLGStreamerStreamerNode", nodes);

  for (vtkMRMLNode* node : nodes)
  {
    vtkMRMLGStreamerStreamerNode* streamerNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(node);
    if (!streamerNode || !streamerNode->GetEnabled())
    {
      continue;
    }

    auto it = this->ActivePipelines.find(streamerNode->GetID());
    if (it != this->ActivePipelines.end())
    {
      if (!streamerNode->GetStreamIn() && it->second.AppSrc)
      {
        this->PushFrame(streamerNode, it->second);
      }
    }
  }
}

void vtkSlicerGStreamerLogic::PushFrame(vtkMRMLGStreamerStreamerNode* streamerNode, PipelineData& data)
{
  if (!streamerNode || !streamerNode->GetVideoNodeID())
  {
    return;
  }

  vtkMRMLNode* videoNode = this->GetMRMLScene()->GetNodeByID(streamerNode->GetVideoNodeID());
  if (!videoNode)
  {
    return;
  }

  vtkSmartPointer<vtkImageData> imageData;

  // Case 1: Video/Volume Node
  vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(videoNode);
  if (volumeNode)
  {
    imageData = volumeNode->GetImageData();
  }
  // Case 2: View Node (Window capture)
  else if (videoNode->IsA("vtkMRMLAbstractViewNode"))
  {
    qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
    if (layoutManager)
    {
      QWidget* viewWidget = layoutManager->viewWidget(videoNode);
      vtkRenderWindow* renderWindow = nullptr;
      if (viewWidget)
      {
        // Slice and 3D view widgets expose typed accessors to their internal VTK views.
        if (qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget))
        {
          renderWindow = sliceWidget->sliceView() ? sliceWidget->sliceView()->renderWindow() : nullptr;
        }
        else if (qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget))
        {
          renderWindow = threeDWidget->threeDView() ? threeDWidget->threeDView()->renderWindow() : nullptr;
        }
      }

      if (renderWindow)
      {
        vtkNew<vtkWindowToImageFilter> windowToImageFilter;
        windowToImageFilter->SetInput(renderWindow);
        windowToImageFilter->SetInputBufferTypeToRGB();
        windowToImageFilter->ReadFrontBufferOff();
        windowToImageFilter->Update();
        imageData = windowToImageFilter->GetOutput();
      }
    }
  }

  if (!imageData)
  {
    return;
  }

  int dims[3];
  imageData->GetDimensions(dims);
  int numComponents = imageData->GetNumberOfScalarComponents();
  
  // Create GstBuffer
  size_t size = dims[0] * dims[1] * numComponents;
  GstBuffer* buffer = gst_buffer_new_allocate(NULL, size, NULL);
  
  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  memcpy(map.data, imageData->GetScalarPointer(), size);
  gst_buffer_unmap(buffer, &map);

  // Set caps on appsrc if not set
  const char* format = (numComponents == 3) ? "RGB" : "RGBA";
  GstCaps* caps = gst_caps_new_simple("video/x-raw",
    "format", G_TYPE_STRING, format,
    "width", G_TYPE_INT, dims[0],
    "height", G_TYPE_INT, dims[1],
    "framerate", GST_TYPE_FRACTION, 30, 1,
    static_cast<void*>(nullptr));
  gst_app_src_set_caps(GST_APP_SRC(data.AppSrc), caps);
  gst_caps_unref(caps);

  gst_app_src_push_buffer(GST_APP_SRC(data.AppSrc), buffer);
}

void vtkSlicerGStreamerLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

void vtkSlicerGStreamerLogic::OnMRMLNodeModified(vtkMRMLNode* node)
{
  this->Superclass::OnMRMLNodeModified(node);
}

void vtkSlicerGStreamerLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }
  if (node->IsA("vtkMRMLGStreamerStreamerNode"))
  {
    // Handle streamer node added
  }
}

void vtkSlicerGStreamerLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }
  if (node->IsA("vtkMRMLGStreamerStreamerNode"))
  {
    this->StopStreaming(vtkMRMLGStreamerStreamerNode::SafeDownCast(node));
  }
}

