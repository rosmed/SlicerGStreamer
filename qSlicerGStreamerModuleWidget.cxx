#include "qSlicerGStreamerModuleWidget.h"
#include "ui_qSlicerGStreamerModuleWidget.h"
#include "vtkSlicerGStreamerLogic.h"
#include "vtkMRMLGStreamerStreamerNode.h"
#include "vtkMRMLScene.h"
#include <QProcessEnvironment>
#include <QMessageBox>
#include "ctkPathLineEdit.h"

class qSlicerGStreamerModuleWidgetPrivate : public Ui_qSlicerGStreamerModuleWidget
{
public:
  qSlicerGStreamerModuleWidgetPrivate();
  vtkMRMLGStreamerStreamerNode* StreamerNode;
  vtkMRMLGStreamerStreamerNode* StreamerInNode;
};

qSlicerGStreamerModuleWidgetPrivate::qSlicerGStreamerModuleWidgetPrivate()
  : StreamerNode(nullptr)
  , StreamerInNode(nullptr)
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
  connect(d->streamerInNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onStreamerInNodeChanged(vtkMRMLNode*)));

  connect(d->addStreamerButton, SIGNAL(clicked()), this, SLOT(onAddStreamOut()));
  connect(d->deleteStreamerButton, SIGNAL(clicked()), this, SLOT(onDeleteStreamOut()));
  connect(d->addStreamerInButton, SIGNAL(clicked()), this, SLOT(onAddStreamIn()));
  connect(d->deleteStreamerInButton, SIGNAL(clicked()), this, SLOT(onDeleteStreamIn()));

  connect(d->nameOutLineEdit, SIGNAL(textEdited(const QString&)),
          this, SLOT(onNameOutEdited(const QString&)));
  connect(d->nameInLineEdit, SIGNAL(textEdited(const QString&)),
          this, SLOT(onNameInEdited(const QString&)));

  connect(d->sourceNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onSourceNodeChanged(vtkMRMLNode*)));
  connect(d->startStreamingButton, SIGNAL(toggled(bool)),
          this, SLOT(onStartStreamingToggled(bool)));
  connect(d->protocolOutComboBox, SIGNAL(currentIndexChanged(int)),
          this, SLOT(onProtocolOutChanged(int)));
  connect(d->unixfdPathLineEdit, SIGNAL(currentPathChanged(const QString&)),
          this, SLOT(onUnixFDPathEdited(const QString&)));

  connect(d->sinkNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onSinkNodeChanged(vtkMRMLNode*)));
  connect(d->startStreamingInButton, SIGNAL(toggled(bool)),
          this, SLOT(onStartStreamingInToggled(bool)));
  connect(d->protocolInComboBox, SIGNAL(currentIndexChanged(int)),
          this, SLOT(onProtocolInChanged(int)));
  connect(d->unixfdInPathLineEdit, SIGNAL(currentPathChanged(const QString&)),
          this, SLOT(onUnixFDInPathEdited(const QString&)));

  d->streamerNodeSelector->addAttribute("vtkMRMLGStreamerStreamerNode", "StreamerType", "Out");
  d->streamerInNodeSelector->addAttribute("vtkMRMLGStreamerStreamerNode", "StreamerType", "In");
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
  if (d->streamerInNodeSelector)
  {
    d->streamerInNodeSelector->setMRMLScene(scene);
  }

  if (scene)
  {
    // Check if we already have streamer nodes in the scene
    vtkMRMLNode* outNode = scene->GetFirstNodeByClass("vtkMRMLGStreamerStreamerNode");
    if (outNode)
    {
      if (vtkMRMLGStreamerStreamerNode::SafeDownCast(outNode)->GetStreamIn())
      {
         d->streamerInNodeSelector->setCurrentNode(outNode);
      }
      else
      {
         d->streamerNodeSelector->setCurrentNode(outNode);
      }
    }
  }
}

void qSlicerGStreamerModuleWidget::onStreamerNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerGStreamerModuleWidget);
  d->StreamerNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(node);
  this->updateWidgetFromMRML();
}

void qSlicerGStreamerModuleWidget::onStreamerInNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerGStreamerModuleWidget);
  d->StreamerInNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(node);
  this->updateWidgetFromMRML();
}

void qSlicerGStreamerModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerGStreamerModuleWidget);
  
  // Disable configuration if no node selected or if streaming
  bool hasOutNode = (d->StreamerNode != nullptr);
  bool isOutStreaming = hasOutNode && d->StreamerNode->GetEnabled();
  bool hasOutSource = hasOutNode && d->StreamerNode->GetVideoNodeID() && d->StreamerNode->GetVideoNodeID()[0] != '\0';

  d->sourceNodeSelector->setEnabled(hasOutNode && !isOutStreaming);
  d->nameOutLineEdit->setEnabled(hasOutNode && !isOutStreaming);
  d->protocolOutComboBox->setEnabled(hasOutNode && !isOutStreaming);
  d->unixfdPathLineEdit->setEnabled(hasOutNode && !isOutStreaming);
  d->startStreamingButton->setEnabled(hasOutNode && (isOutStreaming || hasOutSource));
  d->deleteStreamerButton->setEnabled(hasOutNode && !isOutStreaming);

  if (hasOutNode)
  {
    d->startStreamingButton->setText(isOutStreaming ? "Stop Streaming" : "Start Streaming");
    bool wasBlocking = d->sourceNodeSelector->blockSignals(true);
    d->sourceNodeSelector->setCurrentNodeID(d->StreamerNode->GetVideoNodeID());
    d->sourceNodeSelector->blockSignals(wasBlocking);

    wasBlocking = d->nameOutLineEdit->blockSignals(true);
    d->nameOutLineEdit->setText(d->StreamerNode->GetName());
    d->nameOutLineEdit->blockSignals(wasBlocking);

    bool isRTSP = d->StreamerNode->GetStreamType() == vtkMRMLGStreamerStreamerNode::TYPE_RTSP;
    wasBlocking = d->protocolOutComboBox->blockSignals(true);
    d->protocolOutComboBox->setCurrentIndex(d->StreamerNode->GetStreamType());
    d->protocolOutComboBox->blockSignals(wasBlocking);
    d->label_unixfd->setText(isRTSP ? "RTSP Port:" : "Unix FD Path:");
    d->unixfdPathLineEdit->setShowBrowseButton(!isRTSP);
    d->unixfdPathLineEdit->setCurrentPath(d->StreamerNode->GetUnixFDPath() ? d->StreamerNode->GetUnixFDPath() : "");

    // Update connection URL label
    QString pathStr = d->StreamerNode->GetUnixFDPath() ? d->StreamerNode->GetUnixFDPath() : "";
    if (isRTSP)
    {
      QStringList urls;
      QStringList ports = pathStr.split(",", Qt::SkipEmptyParts);
      for (const QString& port : ports)
      {
        urls << QString("rtsp://127.0.0.1:%1/stream").arg(port.trimmed());
      }
      d->connectionUrlLabel->setText(urls.join(", "));
      d->connectionUrlLabel->show();
      d->label_connection_url->show();
    }
    else
    {
      d->connectionUrlLabel->hide();
      d->label_connection_url->hide();
    }

    d->startStreamingButton->setChecked(isOutStreaming);
  }
  else
  {
    d->startStreamingButton->setText("Start Streaming");
    d->sourceNodeSelector->setCurrentNodeID("");
    d->nameOutLineEdit->setText("");
    d->unixfdPathLineEdit->setCurrentPath("");
    d->startStreamingButton->setChecked(false);
  }

  bool hasInNode = (d->StreamerInNode != nullptr);
  bool isInStreaming = hasInNode && d->StreamerInNode->GetEnabled();

  d->sinkNodeSelector->setEnabled(hasInNode && !isInStreaming);
  d->nameInLineEdit->setEnabled(hasInNode && !isInStreaming);
  d->protocolInComboBox->setEnabled(hasInNode && !isInStreaming);
  d->unixfdInPathLineEdit->setEnabled(hasInNode && !isInStreaming);
  d->startStreamingInButton->setEnabled(hasInNode);
  d->deleteStreamerInButton->setEnabled(hasInNode && !isInStreaming);

  if (hasInNode)
  {
    d->startStreamingInButton->setText(isInStreaming ? "Stop Receiving" : "Start Receiving");
    bool wasBlocking = d->sinkNodeSelector->blockSignals(true);
    d->sinkNodeSelector->setCurrentNodeID(d->StreamerInNode->GetVideoNodeID());
    d->sinkNodeSelector->blockSignals(wasBlocking);

    wasBlocking = d->nameInLineEdit->blockSignals(true);
    d->nameInLineEdit->setText(d->StreamerInNode->GetName());
    d->nameInLineEdit->blockSignals(wasBlocking);

    bool isInRTSP = hasInNode && d->StreamerInNode->GetStreamType() == vtkMRMLGStreamerStreamerNode::TYPE_RTSP;
    wasBlocking = d->protocolInComboBox->blockSignals(true);
    d->protocolInComboBox->setCurrentIndex(d->StreamerInNode->GetStreamType());
    d->protocolInComboBox->blockSignals(wasBlocking);
    d->label_unixfd_in->setText(isInRTSP ? "RTSP Port:" : "Unix FD Path:");
    d->unixfdInPathLineEdit->setShowBrowseButton(!isInRTSP);

    d->unixfdInPathLineEdit->setCurrentPath(d->StreamerInNode->GetUnixFDPath() ? d->StreamerInNode->GetUnixFDPath() : "");
    d->startStreamingInButton->setChecked(isInStreaming);
  }
  else
  {
    d->startStreamingInButton->setText("Start Receiving");
    d->sinkNodeSelector->setCurrentNodeID("");
    d->nameInLineEdit->setText("");
    d->unixfdInPathLineEdit->setCurrentPath("");
    d->startStreamingInButton->setChecked(false);
  }
}

void qSlicerGStreamerModuleWidget::onAddStreamOut()
{
  if (!this->mrmlScene())
  {
    return;
  }
  
  // Find index for the new streamer
  int maxIndex = 0;
  std::vector<vtkMRMLNode*> nodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLGStreamerStreamerNode", nodes);
  for (vtkMRMLNode* n : nodes)
  {
    vtkMRMLGStreamerStreamerNode* sn = vtkMRMLGStreamerStreamerNode::SafeDownCast(n);
    if (sn && !sn->GetStreamIn())
    {
      QString name = sn->GetName();
      if (name.startsWith("StreamOut:"))
      {
        int idx = name.mid(10).toInt();
        if (idx > maxIndex) maxIndex = idx;
      }
    }
  }
  
  QString nodeName = QString("StreamOut:%1").arg(maxIndex + 1);
  vtkMRMLNode* node = this->mrmlScene()->AddNewNodeByClass("vtkMRMLGStreamerStreamerNode", nodeName.toUtf8().constData());
  vtkMRMLGStreamerStreamerNode* sNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(node);
  if (sNode)
  {
    sNode->SetStreamIn(false);
  }
  Q_D(qSlicerGStreamerModuleWidget);
  d->streamerNodeSelector->setCurrentNode(node);
}

void qSlicerGStreamerModuleWidget::onDeleteStreamOut()
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (!d->StreamerNode || !this->mrmlScene())
  {
    return;
  }
  QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Streamer", 
      "Are you sure you want to delete this streamer node?", QMessageBox::Yes|QMessageBox::No);
  if (reply == QMessageBox::Yes)
  {
    this->mrmlScene()->RemoveNode(d->StreamerNode);
  }
}

void qSlicerGStreamerModuleWidget::onAddStreamIn()
{
  if (!this->mrmlScene())
  {
    return;
  }
  vtkMRMLNode* node = this->mrmlScene()->AddNewNodeByClass("vtkMRMLGStreamerStreamerNode", "StreamIn");
  vtkMRMLGStreamerStreamerNode* sNode = vtkMRMLGStreamerStreamerNode::SafeDownCast(node);
  if (sNode)
  {
    sNode->SetStreamIn(true);
    // Create a new volume node by default
    vtkMRMLNode* volumeNode = this->mrmlScene()->AddNewNodeByClass("vtkMRMLStreamingVolumeNode", "Stream");
    if (!volumeNode)
    {
      volumeNode = this->mrmlScene()->AddNewNodeByClass("vtkMRMLScalarVolumeNode", "Stream");
    }
    if (volumeNode)
    {
      sNode->SetVideoNodeID(volumeNode->GetID());
    }
  }
  Q_D(qSlicerGStreamerModuleWidget);
  d->streamerInNodeSelector->setCurrentNode(node);
}

void qSlicerGStreamerModuleWidget::onDeleteStreamIn()
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (!d->StreamerInNode || !this->mrmlScene())
  {
    return;
  }
  QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Streamer", 
      "Are you sure you want to delete this streamer node?", QMessageBox::Yes|QMessageBox::No);
  if (reply == QMessageBox::Yes)
  {
    this->mrmlScene()->RemoveNode(d->StreamerInNode);
  }
}

void qSlicerGStreamerModuleWidget::onNameOutEdited(const QString& name)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (d->StreamerNode)
  {
    d->StreamerNode->SetName(name.toUtf8().constData());
  }
}

void qSlicerGStreamerModuleWidget::onNameInEdited(const QString& name)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (d->StreamerInNode)
  {
    d->StreamerInNode->SetName(name.toUtf8().constData());
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
  
  // Update node name if it's the default "StreamOut"
  if (QString(d->StreamerNode->GetName()) == "StreamOut")
  {
    d->StreamerNode->SetName(QString("Out: %1").arg(node->GetName()).toUtf8().constData());
  }

  // Auto-generate path for Unix FD if it's empty or looks like an auto-generated path
  bool isUnixFD = (d->StreamerNode->GetStreamType() == vtkMRMLGStreamerStreamerNode::TYPE_UNIX_FD);
  QString currentPath = d->StreamerNode->GetUnixFDPath() ? d->StreamerNode->GetUnixFDPath() : "";
  if (isUnixFD && (currentPath.isEmpty() || currentPath.startsWith("/tmp/slicer_gstreamer_")))
  {
    QString userId = QProcessEnvironment::systemEnvironment().value("USER", "default");
    QString path = QString("/tmp/slicer_gstreamer_%1_%2.sock").arg(node->GetName()).arg(userId);
    d->StreamerNode->SetUnixFDPath(path.toUtf8().constData());
    d->unixfdPathLineEdit->setCurrentPath(path);
  }

  this->updateWidgetFromMRML();
}

void qSlicerGStreamerModuleWidget::onUnixFDPathEdited(const QString& path)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (d->StreamerNode)
  {
    d->StreamerNode->SetUnixFDPath(path.toUtf8().constData());
  }
}

void qSlicerGStreamerModuleWidget::onProtocolOutChanged(int index)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (d->StreamerNode)
  {
    d->StreamerNode->SetStreamType(index);
    // If switching to RTSP and path is empty or a socket path, suggest a default port
    QString currentPath = d->StreamerNode->GetUnixFDPath() ? d->StreamerNode->GetUnixFDPath() : "";
    if (index == vtkMRMLGStreamerStreamerNode::TYPE_RTSP && (currentPath.isEmpty() || currentPath.contains("/")))
    {
      d->StreamerNode->SetUnixFDPath("8554");
    }
    this->updateWidgetFromMRML();
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
  this->updateWidgetFromMRML();
}

void qSlicerGStreamerModuleWidget::onSinkNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (!d->StreamerInNode || !node)
  {
    return;
  }

  d->StreamerInNode->SetVideoNodeID(node->GetID());
  
  // Update node name if it's the default "StreamIn"
  if (QString(d->StreamerInNode->GetName()) == "StreamIn")
  {
    d->StreamerInNode->SetName(QString("In: %1").arg(node->GetName()).toUtf8().constData());
  }

  if (QString(d->StreamerInNode->GetUnixFDPath()).isEmpty())
  {
    QString userId = QProcessEnvironment::systemEnvironment().value("USER", "default");
    QString path = QString("/tmp/slicer_gstreamer_in_%1_%2.sock").arg(node->GetName()).arg(userId);
    d->StreamerInNode->SetUnixFDPath(path.toUtf8().constData());
    d->unixfdInPathLineEdit->setCurrentPath(path);
  }
}

void qSlicerGStreamerModuleWidget::onUnixFDInPathEdited(const QString& path)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (d->StreamerInNode)
  {
    d->StreamerInNode->SetUnixFDPath(path.toUtf8().constData());
  }
}

void qSlicerGStreamerModuleWidget::onProtocolInChanged(int index)
{
  Q_D(qSlicerGStreamerModuleWidget);
  if (d->StreamerInNode)
  {
    d->StreamerInNode->SetStreamType(index);
    // If switching to RTSP and path is empty or a socket path, suggest a default port
    QString currentPath = d->StreamerInNode->GetUnixFDPath() ? d->StreamerInNode->GetUnixFDPath() : "";
    if (index == vtkMRMLGStreamerStreamerNode::TYPE_RTSP && (currentPath.isEmpty() || currentPath.contains("/")))
    {
      d->StreamerInNode->SetUnixFDPath("8554");
    }
    this->updateWidgetFromMRML();
  }
}

void qSlicerGStreamerModuleWidget::onStartStreamingInToggled(bool checked)
{
  Q_D(qSlicerGStreamerModuleWidget);
  vtkSlicerGStreamerLogic* logic = vtkSlicerGStreamerLogic::SafeDownCast(this->logic());
  if (!d->StreamerInNode || !logic)
  {
    return;
  }

  d->StreamerInNode->SetEnabled(checked);
  if (checked)
  {
    logic->StartStreaming(d->StreamerInNode);
  }
  else
  {
    logic->StopStreaming(d->StreamerInNode);
  }
  this->updateWidgetFromMRML();
}

