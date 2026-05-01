
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

  QString helpText() const override {
    return "This module allows streaming images to/from GStreamer.<br><br>"
      "<b>Stream Out Example:</b><br>"
      "In Slicer, create a Streamer node with Stream Out. Then run:<br>"
      "gst-launch-1.0 unixfdsrc socket-path=/tmp/slicer_gstreamer_View1_anton.sock ! videoconvert ! autovideosink<br><br>"
      "<b>Stream In Example:</b><br>"
      "In Slicer, create a Streamer node with Stream In and socket path /tmp/videotest. Then run:<br>"
      "gst-launch-1.0 videotestsrc ! videoconvert ! unixfdsink socket-path=/tmp/videotest<br><br>"
      "<b>WebRTC Note:</b><br>"
      "WebRTC streaming uses webrtcbin and requires a signaling server. It does not use a simple IP/Port like RTSP.<br>"
      "You can test the pipeline locally using:<br>"
      "gst-launch-1.0 webrtcbin name=ws ! rtph264pay ! fakesink (requires signaling integration)";
  }
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
