
#include "qSlicerGStreamerModuleWidget.h"
#include "ui_qSlicerGStreamerModuleWidget.h"
#include "vtkSlicerGStreamerLogic.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkMRMLScene.h"
#include <QProcessEnvironment>

class qSlicerGStreamerModuleWidgetPrivate : public Ui_qSlicerGStreamerModuleWidget
{
public:
  qSlicerGStreamerModuleWidgetPrivate();
  vtkMRMLGStreamerStreamerNode* StreamerNode;
};

qSlicerGStreamerModuleWidgetPrivate::qSlicerGStreamerModuleWidgetPrivate()
  : StreamerNode(nullptr)
{
}

qSlicerGStreamerModuleWidget::qSlicerGStreamerModuleWidget(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerGStreamerModuleWidgetPrivate)
{
}

qSlicerGStreamerModuleWidget::~qSlicerGStreamerModuleWidget()
{
}

void qSlicerGStreamerModuleWidget::setup()
{
  Q_D(qSlicerGStreamerModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  connect(d->sourceNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onSourceNodeChanged(vtkMRMLNode*)));
  connect(d->startStreamingButton, SIGNAL(toggled(bool)),
          this, SLOT(onStartStreamingToggled(bool)));
  connect(d->unixfdPathLineEdit, SIGNAL(textEdited(const QString&)),
          this, SLOT(onUnixFDPathEdited(const QString&)));
}

void qSlicerGStreamerModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerGStreamerModuleWidget);
  this->Superclass::setMRMLScene(scene);

  if (d->sourceNodeSelector)
  {
    d->sourceNodeSelector->setMRMLScene(scene);
  }

  if (scene)
  {
    // For this simple version, ensure we have at least one streamer node
    vtkMRMLNode* node = scene->GetFirstNodeByClass("vtkMRMLGStreamerStreamerNode");
    if (!node)
    {
      node = scene->AddNewNodeByClass("vtkMRMLGStreamerStreamerNode");
    }
    d->StreamerNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(node);
    this->updateWidgetFromMRML();
  }
}

void qSlicerGStreamerModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (!d->StreamerNode)
  {
    return;
  }

  d->sourceNodeSelector->setCurrentNodeID(d->StreamerNode->GetVideoNodeID());
  d->unixfdPathLineEdit->setText(d->StreamerNode->GetUnixFDPath() ? d->StreamerNode->GetUnixFDPath() : "");
  d->startStreamingButton->setChecked(d->StreamerNode->GetEnabled());
}

void qSlicerGStreamerModuleWidget::onSourceNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (!d->StreamerNode || !node)
  {
    return;
  }

  d->StreamerNode->SetVideoNodeID(node->GetID());
  
  // Auto-generate path if empty
  if (QString(d->StreamerNode->GetUnixFDPath()).isEmpty())
  {
    QString userId = QProcessEnvironment::systemEnvironment().value("USER", "default");
    QString path = QString("/tmp/slicer_gstreamer_%1_%2.sock").arg(node->GetName()).arg(userId);
    d->StreamerNode->SetUnixFDPath(path.toUtf8().constData());
    d->unixfdPathLineEdit->setText(path);
  }
}

void qSlicerGStreamerModuleWidget::onUnixFDPathEdited(const QString& path)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (d->StreamerNode)
  {
    d->StreamerNode->SetUnixFDPath(path.toUtf8().constData());
  }
}

void qSlicerGStreamerModuleWidget::onStartStreamingToggled(bool checked)
{
  Q_D(qSlicerGStreamerModuleWidget);
  vtkSlicerGStreamerLogic* logic = vtkSlicerGStreamerLogic::SafeDownCast(this->logic());
  if (!d->StreamerNode || !logic)
  {
    return;
  }

  d->StreamerNode->SetEnabled(checked);
  if (checked)
  {
    logic->StartStreaming(d->StreamerNode);
  }
  else
  {
    logic->StopStreaming(d->StreamerNode);
  }
}

