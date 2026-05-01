
#include "vtkSlicerGStreamerLogic.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkMRMLScene.h"
#include "vtkObjectFactory.h"

#include <gst/app/gstappsrc.h>
#include <sstream>
#include <vtkImageData.h>
#include <vtkMRMLVolumeNode.h> 
#include <vtkMRMLAbstractViewNode.h>
#include <vtkWindowToImageFilter.h>
#include <vtkRenderWindow.h>
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qMRMLSliceWidget.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLSliceView.h>
#include <qMRMLThreeDView.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>

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

  std::stringstream ss;
  // Basic pipeline: appsrc -> videoconvert -> unixfdsink
  ss << "appsrc name=src is-live=true format=time ! videoconvert ! video/x-raw,format=I420 ! unixfdsink socket-path=" << node->GetUnixFDPath() << " sync=true";

  GError* error = nullptr;
  GstElement* pipeline = gst_parse_launch(ss.str().c_str(), &error);

  if (error)
  {
    vtkErrorMacro("GStreamer error: " << error->message);
    g_error_free(error);
    return false;
  }

  GstElement* appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "src");
  
  PipelineData data;
  data.Pipeline = pipeline;
  data.AppSrc = appsrc;

  this->ActivePipelines[node->GetID()] = data;

  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  return true;
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
      this->PushFrame(streamerNode, it->second);
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

