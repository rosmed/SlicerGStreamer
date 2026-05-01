
#include "qSlicerGStreamerModule.h"
#include "qSlicerGStreamerModuleWidget.h"
#include "vtkSlicerGStreamerLogic.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkMRMLScene.h"
#include <QTimer>

qSlicerGStreamerModule::qSlicerGStreamerModule(QObject* _parent)
  : Superclass(_parent)
  , Timer(nullptr)
{
}

qSlicerGStreamerModule::~qSlicerGStreamerModule()
{
  if (this->Timer)
  {
    this->Timer->stop();
  }
}

void qSlicerGStreamerModule::setup()
{
  this->Superclass::setup();

  // Register MRML node
  vtkMRMLScene* scene = this->mrmlScene();
  if (scene)
  {
    vtkMRMLGStreamerStreamerNode* node = vtkMRMLGStreamerStreamerNode::New();
    scene->RegisterNodeClass(node);
    node->Delete();
  }

  // Set up timer for logic processing
  this->Timer = new QTimer(this);
  this->Timer->setInterval(33); // ~30 FPS
  connect(this->Timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
  this->Timer->start();
}

void qSlicerGStreamerModule::onTimerTimeout()
{
  vtkSlicerGStreamerLogic* logic = vtkSlicerGStreamerLogic::SafeDownCast(this->logic());
  if (logic)
  {
    logic->ProcessMRMLNodes();
  }
}

qSlicerAbstractModuleRepresentation* qSlicerGStreamerModule::createWidgetRepresentation()
{
  return new qSlicerGStreamerModuleWidget();
}

vtkMRMLAbstractLogic* qSlicerGStreamerModule::createLogic()
{
  return vtkSlicerGStreamerLogic::New();
}

