# SlicerGStreamer

SlicerGStreamer is a 3D Slicer extension that provides video streaming capabilities using the GStreamer framework. It allows users to stream Slicer views to external applications or ingest external video streams directly into Slicer's MRML scene.

## Key Features

- **Bidirectional Streaming**: Supports both streaming *out* of Slicer and streaming *in* to Slicer.
- **Multiple Protocols**:
  - **Unix FD (Domain Sockets)**: Low-latency local streaming using file descriptors.
  - **RTSP**: Network-based streaming via the Real-Time Streaming Protocol (experimental).
- **Flexible Data Types**: Stream from any MRML volume or 3D/2D view.
- **Dynamic Configuration**: Configure encoders, ports, and paths directly from the Slicer UI.

## Usage

### Streaming Out (Slicer to External)

1. Open the **GStreamer** module.
2. In the **Stream Out** section, click **+** to add a new streamer node.
3. Select the **Source Node** (e.g., a View or Volume).
4. Choose the protocol:
   - **Unix FD**: Specify a socket path (e.g., `/tmp/slicer.sock`).
   - **RTSP**: Specify a port (default: `8554`).
5. Click **Start Streaming**.

**Example: Receive Unix FD stream locally**
```bash
gst-launch-1.0 unixfdsrc socket-path=/tmp/slicer.sock ! videoconvert ! autovideosink
```

### Streaming In (External to Slicer)

1. In the **Stream In** section, click **+** to add a receiver node.
2. Select or create a **Sink Node** (typically a Streaming Volume node).
3. Choose the protocol and path/port.
4. Click **Start Receiving**.

**Example: Send a test stream to Slicer**
```bash
gst-launch-1.0 videotestsrc ! videoconvert ! unixfdsink socket-path=/tmp/slicer_in.sock
```

## Developer Information

### MRML Nodes
- `vtkMRMLGStreamerStreamerNode`: Manages the state of a single stream (active, protocol, path, etc.).

### Logic
- `vtkSlicerGStreamerLogic`: Handles the GStreamer pipeline construction and lifecycle. It dynamically builds pipelines based on the node configuration.

## Credits
Developed by **Anton Deguet**.
Part of the Slicer ROS2 ecosystem.
