
#ifndef __vtkSlicerGStreamerLogic_h
#define __vtkSlicerGStreamerLogic_h

#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerGStreamerModuleLogicExport.h"

#include <gst/gst.h>
#include <map>

class vtkMRMLGStreamerStreamerNode;

class VTK_SLICER_GSTREAMER_MODULE_LOGIC_EXPORT vtkSlicerGStreamerLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerGStreamerLogic *New();
  vtkTypeMacro(vtkSlicerGStreamerLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool StartStreaming(vtkMRMLGStreamerStreamerNode* node);
  void StopStreaming(vtkMRMLGStreamerStreamerNode* node);

  void ProcessMRMLNodes();

protected:
  vtkSlicerGStreamerLogic();
  ~vtkSlicerGStreamerLogic() override;

  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;
  void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  void OnMRMLNodeModified(vtkMRMLNode* node) override;
  void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) override;

private:
  vtkSlicerGStreamerLogic(const vtkSlicerGStreamerLogic&);
  void operator=(const vtkSlicerGStreamerLogic&);

  struct PipelineData
  {
    GstElement* Pipeline;
    GstElement* AppSrc;
  };

  void PushFrame(vtkMRMLGStreamerStreamerNode* streamerNode, PipelineData& data);

  std::map<std::string, PipelineData> ActivePipelines;
};

#endif
