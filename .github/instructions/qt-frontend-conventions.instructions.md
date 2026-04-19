---
applyTo: "app/**"
---
# Qt6 / QtNodes Frontend Conventions

Use when editing files in `app/` (the node editor frontend).

## Framework
- Qt6 (not Qt5)
- QtNodes v3 (paceholder/nodeeditor) as git submodule in `external/nodeeditor`
- Pure C++ — no QML for node editor, QML acceptable for auxiliary panels if needed

## QtNodes Architecture
- Use `DataFlowGraphModel` for the graph (built-in undo/redo, JSON serialization)
- Implement nodes as `NodeDelegateModel` subclasses
- Register nodes via `NodeDelegateModelRegistry`
- Data types exchanged between ports: subclass `NodeData` (e.g., `TensorData`, `ImageData`)

## Naming
- Qt classes: `PascalCase` with `Noddle` prefix for top-level (e.g., `NoddleMainWindow`)
- Signals: `verbNoun` (e.g., `frameReady`, `pipelineChanged`)
- Slots: `onVerbNoun` (e.g., `onFrameReady`)
- Private members: `m_camelCase`
- UI component files: `PascalCase.hpp` / `PascalCase.cpp`

## Patterns
- Signals & slots with new syntax: `connect(sender, &Sender::signal, receiver, &Receiver::slot)`
- `Q_OBJECT` macro in every QObject subclass header
- Use `QThread` or `QtConcurrent` for background work — never block the UI thread
- Preview widgets: embed `QLabel` or custom `QWidget` inside NodeDelegateModel via `embeddedWidget()`
- Side panel preview: `QDockWidget` updated on node selection change

## Data Flow
-  Source nodes output `std::shared_ptr<NodeData>` downstream
- Processing nodes receive `NodeData`, cast to concrete type, compute, output result
- All inter-node data is `shared_ptr<NodeData>` — QtNodes manages lifetime

## File Structure
```
app/
├── CMakeLists.txt
├── main.cpp              # QApplication + NoddleMainWindow
├── NoddleMainWindow.hpp/cpp
├── PreviewPanel.hpp/cpp  # QDockWidget for side preview
├── nodes/                # NodeDelegateModel subclasses
│   ├── ImageSourceModel.hpp/cpp
│   ├── CsvSourceModel.hpp/cpp
│   ├── PointCloudSourceModel.hpp/cpp
│   ├── CameraSourceModel.hpp/cpp
│   ├── ImageDisplayModel.hpp/cpp
│   └── opencv/           # OpenCV processing nodes
│       ├── ColorConvertModel.hpp/cpp
│       ├── ResizeModel.hpp/cpp
│       └── ...
├── data/                 # NodeData subclasses
│   ├── TensorData.hpp
│   ├── ImageData.hpp
│   ├── TableData.hpp
│   └── PointCloudData.hpp
└── widgets/              # Reusable embedded widgets
    └── ImagePreviewWidget.hpp/cpp
```

## Build
- CMake target: `noddle_app`
- Link against: `QtNodes::QtNodes`, `Qt6::Widgets`, `noddle_core`
- Optional: `Qt6::OpenGLWidgets` for 3D preview
