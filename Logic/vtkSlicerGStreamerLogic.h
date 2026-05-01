#ifndef __vtkSlicerGStreamerLogic_h
#define __vtkSlicerGStreamerLogic_h

#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerGStreamerModuleLogicExport.h"
#include <map>
#include <string>
#include "vtkSmartPointer.h"

class vtkMRMLGStreamerStreamerNode;
class vtkSlicerGStreamerStreamerOut;
class vtkSlicerGStreamerStreamerIn;

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

private:
  vtkSlicerGStreamerLogic(const vtkSlicerGStreamerLogic&);
  void operator=(const vtkSlicerGStreamerLogic&);

  std::map<std::string, vtkSmartPointer<vtkSlicerGStreamerStreamerOut>> StreamersOut;
  std::map<std::string, vtkSmartPointer<vtkSlicerGStreamerStreamerIn>> StreamersIn;
};

#endif
