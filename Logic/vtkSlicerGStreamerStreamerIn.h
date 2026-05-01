#ifndef __vtkSlicerGStreamerStreamerIn_h
#define __vtkSlicerGStreamerStreamerIn_h

#include "vtkObject.h"
#include "vtkSlicerGStreamerModuleLogicExport.h"
#include <gst/gst.h>
#include <string>
#include <vector>
#include <mutex>

class vtkMRMLGStreamerStreamerNode;
class vtkMRMLScene;

class VTK_SLICER_GSTREAMER_MODULE_LOGIC_EXPORT vtkSlicerGStreamerStreamerIn : public vtkObject
{
public:
  static vtkSlicerGStreamerStreamerIn *New();
  vtkTypeMacro(vtkSlicerGStreamerStreamerIn, vtkObject);

  bool Start(vtkMRMLGStreamerStreamerNode* node);
  void Stop();
  void ProcessPendingFrames();

  void SetMRMLScene(vtkMRMLScene* scene);

protected:
  vtkSlicerGStreamerStreamerIn();
  ~vtkSlicerGStreamerStreamerIn() override;

  static GstFlowReturn OnNewSample(GstElement* sink, gpointer data);

  GstElement* Pipeline;
  GstElement* AppSink;
  std::string StreamerNodeID;
  vtkMRMLScene* MRMLScene;

  struct FrameData {
    std::vector<unsigned char> PixelData;
    int Width, Height;
  };
  FrameData PendingFrame;
  bool HasPendingFrame;
  std::mutex FrameMutex;

private:
  vtkSlicerGStreamerStreamerIn(const vtkSlicerGStreamerStreamerIn&);
  void operator=(const vtkSlicerGStreamerStreamerIn&);
};

#endif
