
#ifndef __qSlicerGStreamerModuleWidget_h
#define __qSlicerGStreamerModuleWidget_h

#include "qSlicerAbstractModuleWidget.h"
#include "qSlicerGStreamerModuleExport.h"

class qSlicerGStreamerModuleWidgetPrivate;

class Q_SLICER_QTMODULES_GSTREAMER_EXPORT qSlicerGStreamerModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerGStreamerModuleWidget(QWidget *parent=nullptr);
  ~qSlicerGStreamerModuleWidget() override;

protected:
  QScopedPointer<qSlicerGStreamerModuleWidgetPrivate> d_ptr;
  void setup() override;

public slots:
  void setMRMLScene(vtkMRMLScene* scene) override;

protected slots:
  void updateWidgetFromMRML();
  void onSourceNodeChanged(vtkMRMLNode* node);
  void onUnixFDPathEdited(const QString& path);
  void onStartStreamingToggled(bool checked);

private:
  Q_DECLARE_PRIVATE(qSlicerGStreamerModuleWidget);
  Q_DISABLE_COPY(qSlicerGStreamerModuleWidget);
};

#endif
