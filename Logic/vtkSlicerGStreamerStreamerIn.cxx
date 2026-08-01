#include "vtkSlicerGStreamerStreamerIn.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLStreamingVolumeNode.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include <gst/app/gstappsink.h>
#include <sstream>
#include <string>

namespace
{
// A socket path prefixed with '@' selects a Linux abstract-namespace unix socket
// (no filesystem entry) instead of a path-based one.
std::string UnixFdSocketPathClause(const std::string& rawPath)
{
  if (!rawPath.empty() && rawPath[0] == '@')
  {
    return "socket-path=" + rawPath.substr(1) + " socket-type=abstract";
  }
  return "socket-path=" + rawPath;
}

// Pulls the first pending ERROR message (if any) off the pipeline's bus and
// describes it, for logging when a synchronous state change fails.
std::string DescribeBusError(GstElement* pipeline)
{
  GstBus* bus = gst_element_get_bus(pipeline);
  GstMessage* msg = gst_bus_timed_pop_filtered(bus, 0, GST_MESSAGE_ERROR);
  gst_object_unref(bus);
  if (!msg)
  {
    return "no error message available on bus";
  }
  GError* error = nullptr;
  gchar* debugInfo = nullptr;
  gst_message_parse_error(msg, &error, &debugInfo);
  std::string description = error ? error->message : "unknown error";
  if (debugInfo)
  {
    description += " (";
    description += debugInfo;
    description += ")";
  }
  if (error) g_error_free(error);
  if (debugInfo) g_free(debugInfo);
  gst_message_unref(msg);
  return description;
}
}

vtkStandardNewMacro(vtkSlicerGStreamerStreamerIn);

vtkSlicerGStreamerStreamerIn::vtkSlicerGStreamerStreamerIn() : Pipeline(nullptr), AppSink(nullptr), MRMLScene(nullptr), HasPendingFrame(false) {}

vtkSlicerGStreamerStreamerIn::~vtkSlicerGStreamerStreamerIn() { this->Stop(); }

void vtkSlicerGStreamerStreamerIn::SetMRMLScene(vtkMRMLScene* scene) { this->MRMLScene = scene; }

bool vtkSlicerGStreamerStreamerIn::Start(vtkMRMLGStreamerStreamerNode* node)
{
  if (!node || !node->GetUnixFDPath())
  {
    vtkErrorMacro("Cannot start streamer: node is null or has no socket path set");
    return false;
  }
  this->Stop();
  this->StreamerNodeID = node->GetID();
  std::stringstream ss;
  if (node->GetStreamType() == vtkMRMLGStreamerStreamerNode::TYPE_RTSP)
  {
    ss << "rtspsrc location=rtsp://127.0.0.1:" << node->GetUnixFDPath() << "/stream latency=0 ! rtph264depay ! decodebin ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink emit-signals=true sync=false";
  }
  else
  {
    ss << "unixfdsrc " << UnixFdSocketPathClause(node->GetUnixFDPath()) << " ! decodebin ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink emit-signals=true sync=false";
  }

  vtkWarningMacro("Starting GStreamer In pipeline: " << ss.str());

  GError* error = nullptr;
  this->Pipeline = gst_parse_launch(ss.str().c_str(), &error);
  if (error)
  {
    vtkErrorMacro("Failed to parse GStreamer pipeline: " << error->message << " (pipeline: " << ss.str() << ")");
    g_error_free(error);
    this->Pipeline = nullptr;
    return false;
  }

  this->AppSink = gst_bin_get_by_name(GST_BIN(this->Pipeline), "sink");
  g_signal_connect(this->AppSink, "new-sample", G_CALLBACK(OnNewSample), this);

  GstStateChangeReturn stateResult = gst_element_set_state(this->Pipeline, GST_STATE_PLAYING);
  if (stateResult == GST_STATE_CHANGE_FAILURE)
  {
    vtkErrorMacro("GStreamer pipeline failed to reach PLAYING state: " << DescribeBusError(this->Pipeline));
    this->Stop();
    return false;
  }
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
