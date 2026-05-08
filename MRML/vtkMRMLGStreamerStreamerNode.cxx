
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMRMLGStreamerStreamerNode);

vtkMRMLGStreamerStreamerNode::vtkMRMLGStreamerStreamerNode()
{
  this->UnixFDPath = nullptr;
  this->VideoNodeID = nullptr;
  this->Enabled = false;
  this->StreamIn = false;
  this->StreamType = TYPE_UNIX_FD;
  this->SetAttribute("StreamerType", "Out");
}

vtkMRMLGStreamerStreamerNode::~vtkMRMLGStreamerStreamerNode()
{
  if (this->UnixFDPath) delete[] this->UnixFDPath;
  if (this->VideoNodeID) delete[] this->VideoNodeID;
}

void vtkMRMLGStreamerStreamerNode::PrintSelf(std::ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "UnixFDPath: " << (this->UnixFDPath ? this->UnixFDPath : "(none)") << "\n";
  os << indent << "VideoNodeID: " << (this->VideoNodeID ? this->VideoNodeID : "(none)") << "\n";
  os << indent << "Enabled: " << (this->Enabled ? "true" : "false") << "\n";
  os << indent << "StreamIn: " << (this->StreamIn ? "true" : "false") << "\n";
  os << indent << "StreamType: " << this->StreamType << "\n";
}

vtkMRMLNode* vtkMRMLGStreamerStreamerNode::CreateNodeInstance()
{
  return vtkMRMLGStreamerStreamerNode::New();
}

void vtkMRMLGStreamerStreamerNode::ReadXMLAttributes(const char** atts)
{
  int wasModifying = this->StartModify();
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStringMacro(UnixFDPath, UnixFDPath);
  vtkMRMLReadXMLStringMacro(VideoNodeID, VideoNodeID);
  vtkMRMLReadXMLBooleanMacro(Enabled, Enabled);
  
  const char* attTag;
  const char* attValue;
  const char** attsPtr = atts;
  while (*attsPtr != nullptr)
  {
    attTag = *(attsPtr++);
    attValue = *(attsPtr++);
    if (!strcmp(attTag, "StreamIn")) { this->SetStreamIn(!strcmp(attValue, "true")); }
  }

  vtkMRMLReadXMLIntMacro(StreamType, StreamType);
  vtkMRMLReadXMLEndMacro();
  this->EndModify(wasModifying);
}

void vtkMRMLGStreamerStreamerNode::WriteXML(std::ostream& of, int indent)
{
  Superclass::WriteXML(of, indent);
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLStringMacro(UnixFDPath, UnixFDPath);
  vtkMRMLWriteXMLStringMacro(VideoNodeID, VideoNodeID);
  vtkMRMLWriteXMLBooleanMacro(Enabled, Enabled);
  vtkMRMLWriteXMLBooleanMacro(StreamIn, StreamIn);
  vtkMRMLWriteXMLIntMacro(StreamType, StreamType);
  vtkMRMLWriteXMLEndMacro();
}

void vtkMRMLGStreamerStreamerNode::Copy(vtkMRMLNode* anode)
{
  int wasModifying = this->StartModify();
  Superclass::Copy(anode);
  vtkMRMLGStreamerStreamerNode* node = vtkMRMLGStreamerStreamerNode::SafeDownCast(anode);
  if (node)
  {
    this->SetUnixFDPath(node->UnixFDPath);
    this->SetVideoNodeID(node->VideoNodeID);
    this->Enabled = node->Enabled;
    this->StreamIn = node->StreamIn;
    this->SetStreamType(node->StreamType);
  }
  this->EndModify(wasModifying);
}

void vtkMRMLGStreamerStreamerNode::SetStreamIn(bool in)
{
  if (this->StreamIn == in)
  {
    return;
  }
  this->StreamIn = in;
  this->SetAttribute("StreamerType", in ? "In" : "Out");
  this->Modified();
}
