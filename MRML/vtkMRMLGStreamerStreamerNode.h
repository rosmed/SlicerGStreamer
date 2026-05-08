
#ifndef __vtkMRMLGStreamerStreamerNode_h
#define __vtkMRMLGStreamerStreamerNode_h

#include "vtkMRMLNode.h"
#include "vtkSlicerGStreamerModuleMRMLExport.h"

class VTK_SLICER_GSTREAMER_MODULE_MRML_EXPORT vtkMRMLGStreamerStreamerNode : public vtkMRMLNode
{
public:
  static vtkMRMLGStreamerStreamerNode *New();
  vtkTypeMacro(vtkMRMLGStreamerStreamerNode, vtkMRMLNode);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  virtual vtkMRMLNode* CreateNodeInstance() override;
  virtual const char* GetNodeTagName() override { return "GStreamerStreamer"; };

  void ReadXMLAttributes(const char** atts) override;
  void WriteXML(std::ostream& of, int indent) override;
  void Copy(vtkMRMLNode *node) override;

  vtkGetStringMacro(UnixFDPath);
  vtkSetStringMacro(UnixFDPath);

  vtkGetStringMacro(VideoNodeID);
  vtkSetStringMacro(VideoNodeID);

  vtkGetMacro(Enabled, bool);
  vtkSetMacro(Enabled, bool);

  vtkGetMacro(StreamIn, bool);
  void SetStreamIn(bool in);

  enum {
    TYPE_UNIX_FD = 0,
    TYPE_RTSP = 1
  };
  vtkGetMacro(StreamType, int);
  vtkSetMacro(StreamType, int);

protected:
  vtkMRMLGStreamerStreamerNode();
  ~vtkMRMLGStreamerStreamerNode();
  vtkMRMLGStreamerStreamerNode(const vtkMRMLGStreamerStreamerNode&);
  void operator=(const vtkMRMLGStreamerStreamerNode&);

  char* UnixFDPath;
  char* VideoNodeID;
  bool Enabled;
  bool StreamIn;
  int StreamType;
};

#endif
