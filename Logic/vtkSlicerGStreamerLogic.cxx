#include "vtkSlicerGStreamerLogic.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkSlicerGStreamerStreamerOut.h"
#include "vtkSlicerGStreamerStreamerIn.h"
#include "vtkMRMLScene.h"
#include "vtkObjectFactory.h"
#include <string>

vtkStandardNewMacro(vtkSlicerGStreamerLogic);

vtkSlicerGStreamerLogic::vtkSlicerGStreamerLogic()
{
  gst_init(NULL, NULL);
}

vtkSlicerGStreamerLogic::~vtkSlicerGStreamerLogic()
{
  this->StreamersOut.clear();
  this->StreamersIn.clear();
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

  std::string newPath = node->GetUnixFDPath();
  for (auto const& pair : this->StreamersOut)
  {
    if (pair.first == node->GetID()) continue;
    vtkMRMLGStreamerStreamerNode* other = vtkMRMLGStreamerStreamerNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(pair.first.c_str()));
    if (other && other->GetUnixFDPath() && newPath == other->GetUnixFDPath())
    {
       vtkErrorMacro("Socket path " << newPath << " in use by " << pair.first);
       return false;
    }
  }
  for (auto const& pair : this->StreamersIn)
  {
    if (pair.first == node->GetID()) continue;
    vtkMRMLGStreamerStreamerNode* other = vtkMRMLGStreamerStreamerNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(pair.first.c_str()));
    if (other && other->GetUnixFDPath() && newPath == other->GetUnixFDPath())
    {
       vtkErrorMacro("Socket path " << newPath << " in use by " << pair.first);
       return false;
    }
  }

  this->StopStreaming(node);

  if (!node->GetStreamIn())
  {
    vtkSmartPointer<vtkSlicerGStreamerStreamerOut> streamer = vtkSmartPointer<vtkSlicerGStreamerStreamerOut>::New();
    streamer->SetMRMLScene(this->GetMRMLScene());
    if (streamer->Start(node))
    {
      this->StreamersOut[node->GetID()] = streamer;
      return true;
    }
  }
  else
  {
    vtkSmartPointer<vtkSlicerGStreamerStreamerIn> streamer = vtkSmartPointer<vtkSlicerGStreamerStreamerIn>::New();
    streamer->SetMRMLScene(this->GetMRMLScene());
    if (streamer->Start(node))
    {
      this->StreamersIn[node->GetID()] = streamer;
      return true;
    }
  }
  return false;
}

void vtkSlicerGStreamerLogic::StopStreaming(vtkMRMLGStreamerStreamerNode* node)
{
  if (!node || !node->GetID()) return;
  auto outIt = this->StreamersOut.find(node->GetID());
  if (outIt != this->StreamersOut.end())
  {
    outIt->second->Stop();
    this->StreamersOut.erase(outIt);
  }

  auto inIt = this->StreamersIn.find(node->GetID());
  if (inIt != this->StreamersIn.end())
  {
    inIt->second->Stop();
    this->StreamersIn.erase(inIt);
  }
}

void vtkSlicerGStreamerLogic::ProcessMRMLNodes()
{
  if (!this->GetMRMLScene()) return;

  for (auto const& pair : this->StreamersIn)
  {
    pair.second->ProcessPendingFrames();
  }

  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLGStreamerStreamerNode", nodes);
  for (vtkMRMLNode* n : nodes)
  {
    vtkMRMLGStreamerStreamerNode* sNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(n);
    if (!sNode || !sNode->GetEnabled() || sNode->GetStreamIn()) continue;
    auto it = this->StreamersOut.find(sNode->GetID());
    if (it != this->StreamersOut.end())
    {
      it->second->PushFrame();
    }
  }
}

void vtkSlicerGStreamerLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}
