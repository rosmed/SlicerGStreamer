#ifndef __vtkSlicerGStreamerStreamerOut_h
#define __vtkSlicerGStreamerStreamerOut_h

#include "vtkObject.h"
#include "vtkSlicerGStreamerModuleLogicExport.h"
#include <gst/gst.h>
#include <string>

class vtkMRMLGStreamerStreamerNode;
class vtkMRMLScene;

class VTK_SLICER_GSTREAMER_MODULE_LOGIC_EXPORT vtkSlicerGStreamerStreamerOut : public vtkObject
{
public:
  static vtkSlicerGStreamerStreamerOut *New();
  vtkTypeMacro(vtkSlicerGStreamerStreamerOut, vtkObject);

  bool Start(vtkMRMLGStreamerStreamerNode* node);
  void Stop();
  void PushFrame();

  void SetMRMLScene(vtkMRMLScene* scene);

protected:
  vtkSlicerGStreamerStreamerOut();
  ~vtkSlicerGStreamerStreamerOut() override;

  GstElement* Pipeline;
  GstElement* AppSrc;
  std::string StreamerNodeID;
  vtkMRMLScene* MRMLScene;

private:
  vtkSlicerGStreamerStreamerOut(const vtkSlicerGStreamerStreamerOut&);
  void operator=(const vtkSlicerGStreamerStreamerOut&);
};

#endif
