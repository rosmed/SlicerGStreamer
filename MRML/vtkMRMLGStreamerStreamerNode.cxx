
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMRMLGStreamerStreamerNode);

vtkMRMLGStreamerStreamerNode::vtkMRMLGStreamerStreamerNode()
{
  this->UnixFDPath = nullptr;
  this->VideoNodeID = nullptr;
  this->Enabled = false;
  this->StreamIn = false;
}

vtkMRMLGStreamerStreamerNode::~vtkMRMLGStreamerStreamerNode()
{
  if (this->UnixFDPath) delete[] this->UnixFDPath;
  if (this->VideoNodeID) delete[] this->VideoNodeID;
}

void vtkMRMLGStreamerStreamerNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "UnixFDPath: " << (this->UnixFDPath ? this->UnixFDPath : "(none)") << "\n";
  os << indent << "VideoNodeID: " << (this->VideoNodeID ? this->VideoNodeID : "(none)") << "\n";
  os << indent << "Enabled: " << (this->Enabled ? "true" : "false") << "\n";
  os << indent << "StreamIn: " << (this->StreamIn ? "true" : "false") << "\n";
}

vtkMRMLNode* vtkMRMLGStreamerStreamerNode::CreateNodeInstance()
{
  return vtkMRMLGStreamerStreamerNode::New();
}
