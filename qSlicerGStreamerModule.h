
#ifndef __qSlicerGStreamerModule_h
#define __qSlicerGStreamerModule_h

// Slicer includes
#include "qSlicerLoadableModule.h"

#include "qSlicerGStreamerModuleExport.h"

class Q_SLICER_QTMODULES_GSTREAMER_EXPORT
qSlicerGStreamerModule
  : public qSlicerLoadableModule
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");

public:
  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerGStreamerModule(QObject *parent=nullptr);
  ~qSlicerGStreamerModule() override;

  qSlicerGetTitleMacro("GStreamer");

  QString helpText() const override { return "This module allows streaming images to/from GStreamer."; }
  QString acknowledgementText() const override { return "Developed by Anton Deguet."; }
  QStringList contributors() const override { return QStringList() << "Anton Deguet"; }

  QStringList categories() const override { return QStringList() << "GStreamer"; }

  void setMRMLScene(vtkMRMLScene* scene) override;

protected:
  void setup() override;
  qSlicerAbstractModuleRepresentation * createWidgetRepresentation() override;
  vtkMRMLAbstractLogic* createLogic() override;

public slots:
  void onTimerTimeout();

private:
  Q_DISABLE_COPY(qSlicerGStreamerModule);
  QTimer* Timer;
};

#endif
