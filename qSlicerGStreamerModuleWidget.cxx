
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

  connect(d->streamerNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onStreamerNodeChanged(vtkMRMLNode*)));

  connect(d->sourceNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onSourceNodeChanged(vtkMRMLNode*)));
  connect(d->startStreamingButton, SIGNAL(toggled(bool)),
          this, SLOT(onStartStreamingToggled(bool)));
  connect(d->unixfdPathLineEdit, SIGNAL(textEdited(const QString&)),
          this, SLOT(onUnixFDPathEdited(const QString&)));

  connect(d->sinkNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onSinkNodeChanged(vtkMRMLNode*)));
  connect(d->startStreamingInButton, SIGNAL(toggled(bool)),
          this, SLOT(onStartStreamingInToggled(bool)));
  connect(d->unixfdInPathLineEdit, SIGNAL(textEdited(const QString&)),
          this, SLOT(onUnixFDInPathEdited(const QString&)));
}

void qSlicerGStreamerModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerGStreamerModuleWidget);
  this->Superclass::setMRMLScene(scene);

  if (d->streamerNodeSelector)
  {
    d->streamerNodeSelector->setMRMLScene(scene);
  }
  if (d->sourceNodeSelector)
  {
    d->sourceNodeSelector->setMRMLScene(scene);
  }
  if (d->sinkNodeSelector)
  {
    d->sinkNodeSelector->setMRMLScene(scene);
  }

  if (scene)
  {
    // For this version, ensure we have at least one streamer node to start with
    vtkMRMLNode* node = scene->GetFirstNodeByClass("vtkMRMLGStreamerStreamerNode");
    if (!node)
    {
      node = scene->AddNewNodeByClass("vtkMRMLGStreamerStreamerNode");
    }
    d->streamerNodeSelector->setCurrentNode(node);
  }
}

void qSlicerGStreamerModuleWidget::onStreamerNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerGStreamerModuleWidget);
  d->StreamerNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(node);
  this->updateWidgetFromMRML();
}

void qSlicerGStreamerModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerGStreamerModuleWidget);
  
  // Disable everything if no streamer node is selected
  bool hasNode = (d->StreamerNode != nullptr);
  d->streamOutCollapsibleButton->setEnabled(hasNode);
  d->streamInCollapsibleButton->setEnabled(hasNode);

  if (!hasNode)
  {
    return;
  }

  bool isStreamIn = d->StreamerNode->GetStreamIn();

  // Update Source selector
  bool wasBlocking = d->sourceNodeSelector->blockSignals(true);
  d->sourceNodeSelector->setCurrentNodeID(isStreamIn ? "" : d->StreamerNode->GetVideoNodeID());
  d->sourceNodeSelector->blockSignals(wasBlocking);

  // Update Sink selector
  wasBlocking = d->sinkNodeSelector->blockSignals(true);
  d->sinkNodeSelector->setCurrentNodeID(isStreamIn ? d->StreamerNode->GetVideoNodeID() : "");
  d->sinkNodeSelector->blockSignals(wasBlocking);

  // Update Paths and Buttons
  if (isStreamIn)
  {
    d->unixfdInPathLineEdit->setText(d->StreamerNode->GetUnixFDPath() ? d->StreamerNode->GetUnixFDPath() : "");
    d->startStreamingInButton->setChecked(d->StreamerNode->GetEnabled());
    d->unixfdPathLineEdit->setText("");
    d->startStreamingButton->setChecked(false);
  }
  else
  {
    d->unixfdPathLineEdit->setText(d->StreamerNode->GetUnixFDPath() ? d->StreamerNode->GetUnixFDPath() : "");
    d->startStreamingButton->setChecked(d->StreamerNode->GetEnabled());
    d->unixfdInPathLineEdit->setText("");
    d->startStreamingInButton->setChecked(false);
  }
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

void qSlicerGStreamerModuleWidget::onSinkNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (!d->StreamerNode || !node)
  {
    return;
  }

  // Reuse VideoNodeID for sink or create a separate property in MRML if needed
  // For now, let's assume one node can only do one or the other
  d->StreamerNode->SetVideoNodeID(node->GetID());
  d->StreamerNode->SetStreamIn(true);
  
  if (QString(d->StreamerNode->GetUnixFDPath()).isEmpty())
  {
    QString userId = QProcessEnvironment::systemEnvironment().value("USER", "default");
    QString path = QString("/tmp/slicer_gstreamer_in_%1_%2.sock").arg(node->GetName()).arg(userId);
    d->StreamerNode->SetUnixFDPath(path.toUtf8().constData());
    d->unixfdInPathLineEdit->setText(path);
  }
}

void qSlicerGStreamerModuleWidget::onUnixFDInPathEdited(const QString& path)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (d->StreamerNode)
  {
    d->StreamerNode->SetUnixFDPath(path.toUtf8().constData());
  }
}

void qSlicerGStreamerModuleWidget::onStartStreamingInToggled(bool checked)
{
  Q_D(qSlicerGStreamerModuleWidget);
  vtkSlicerGStreamerLogic* logic = vtkSlicerGStreamerLogic::SafeDownCast(this->logic());
  if (!d->StreamerNode || !logic)
  {
    return;
  }

  d->StreamerNode->SetEnabled(checked);
  d->StreamerNode->SetStreamIn(checked);
  if (checked)
  {
    // Logic needs to be updated to handle StreamIn
    logic->StartStreaming(d->StreamerNode);
  }
  else
  {
    logic->StopStreaming(d->StreamerNode);
  }
}

