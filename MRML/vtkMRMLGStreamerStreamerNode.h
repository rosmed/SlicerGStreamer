
#ifndef __vtkMRMLGStreamerStreamerNode_h
#define __vtkMRMLGStreamerStreamerNode_h

#include "vtkMRMLNode.h"
#include "vtkSlicerGStreamerModuleMRMLExport.h"

class VTK_SLICER_GSTREAMER_MODULE_MRML_EXPORT vtkMRMLGStreamerStreamerNode : public vtkMRMLNode
{
public:
  static vtkMRMLGStreamerStreamerNode *New();
  vtkTypeMacro(vtkMRMLGStreamerStreamerNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual vtkMRMLNode* CreateNodeInstance() override;
  virtual const char* GetNodeTagName() override { return "GStreamerStreamer"; };

  vtkGetStringMacro(UnixFDPath);
  vtkSetStringMacro(UnixFDPath);

  vtkGetStringMacro(VideoNodeID);
  vtkSetStringMacro(VideoNodeID);

  vtkGetMacro(Enabled, bool);
  vtkSetMacro(Enabled, bool);

protected:
  vtkMRMLGStreamerStreamerNode();
  ~vtkMRMLGStreamerStreamerNode();
  vtkMRMLGStreamerStreamerNode(const vtkMRMLGStreamerStreamerNode&);
  void operator=(const vtkMRMLGStreamerStreamerNode&);

  char* UnixFDPath;
  char* VideoNodeID;
  bool Enabled;
};

#endif
