# Noddle Knowledge Base

## Vision
- Noddle is a modular flow-programming software for computer vision with real-time visual feedback.
- Primary objective: high performance with a C++-heavy backend and hardware-accelerated functions whenever possible.

## Product Requirements
- Build visual pipelines from heterogeneous inputs: live camera, point cloud, image folder, single image, CSV table.
- Expose OpenCV base functions as blocks with strict input/output type handling.
- Support outputs such as files, video/audio streams, and classification results.
- Prioritize usability, fluid UX, and polished design.

## Agent Strategy
- Main orchestrator agent: Noddle Orchestrator.
- Orchestrator can plan, delegate to subagents, and edit files when needed.
- Preferred subagent domains: backend C++ performance, frontend UX graph editor, CV/OpenCV, test/QA, code review/architecture, DevOps/CI-CD.

## Confirmed Technical Direction (2026-04-11)
- Custom agent scope: workspace-level.
- Agent name: Noddle Orchestrator.
- Priority: global architecture and technical roadmap.
- UI baseline preference: Ryven (Python/Qt).
- Runtime preference: mixed C++ backend and Python workers.
- Python integration: native Python blocks in addition to C++ wrappers.
- MVP target: multimodal pipeline (image + point cloud + CSV).

## Architectural Decisions (Locked)
- Target platform for now: Linux only.
- Compute strategy: abstraction-first runtime, hardware backends implemented incrementally.
- Internal data model between nodes: raw tensor as canonical working format.
- File/hardware specific formats are handled by dedicated import nodes.
- UX principle: keep real-time visualization enabled whenever possible unless user explicitly disables it.
- Deployment model later: export executable for faster batch processing or optimized live processing where relevant.
- API strategy: internal API first, no stable plugin ABI commitment yet.
- UI strategy: stay on Ryven now, but keep frontend integration modular to allow future migration.
- Performance objective: no fixed latency target; optimize pipeline by pipeline.
- Packaging/distribution: not in scope at this stage (development project first).

## Validation Notes
- Initial C++ scaffold configured and built successfully with CMake on Linux.
- Runtime demo executable runs and confirms CPU backend execution path.

## MVP Decisions (2026-04-11)
- Minimal metadata contract: dimensions only.
- Overload handling modes: drop frame, downsample, pause, all selectable.
- Overload defaults: stream -> drop frame, single frame -> pause.
- Baseline OpenCV blocks approved: color convert, resize, gaussian blur, threshold, canny, morphology.

## MVP Implementation Status
- C++ core updated with dimension-only metadata validation.
- Visualization policy implemented with user override support.
- Source node contracts and initial image/csv/pointcloud nodes added.
- OpenCV block node classes added with optional OpenCV-backed implementation path.
- Build and demo execution re-validated successfully after MVP updates.
- **Color space tracking** added to `ImageData` (enum `ColorSpace` with 8 values: RGB, BGR, Grayscale, HSV, HLS, Lab, YCrCb, XYZ).
- `ColorConvertModel` rewritten with dual combo boxes (input space auto-follows incoming data, output space shows only valid targets).
- All OpenCV nodes propagate color space: pass-through for GaussianBlur/Resize/Morphology, `Grayscale` tag for Threshold/Canny.

## Frontend Status (2026-04-11)
- **PySide6 frontend REMOVED** (Phase 0, 2026-04-19). Replaced by C++/Qt6 native frontend.
- New frontend in `app/` using Qt6 + QtNodes (paceholder/nodeeditor).
- QtNodes integrated as git submodule at `external/nodeeditor`.
- Minimal main.cpp with DataFlowGraphModel + GraphicsView + QMainWindow scaffold.
- Build validated: all 4 targets compile (noddle_core, noddle_core_demo, QtNodes, noddle_app).

## Build System (2026-04-19)
- Root `CMakeLists.txt` orchestrates: backend/cpp, external/nodeeditor, app/.
- Qt6 6.4.2 installed via apt (`qt6-base-dev`, `qt6-tools-dev`).
- QtNodes: `QtNodes::QtNodes` CMake target, USE_QT6=ON, BSD-3-Clause.
- Backend: standalone-capable CMakeLists.txt, optional OpenCV via `NODDLE_WITH_OPENCV`.
- Build command: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build`

## Tooling Updates (2026-04-19)
- Git repo initialized with conventional commits.
- `.gitignore` created (build/, .venv/, IDE, Python cache, Qt generated, compiled objects).
- Auto-commit hook: `.github/hooks/auto-commit.json` (PostToolUse, counts uncommitted changes).
- TDD skill: `.github/skills/tdd/SKILL.md` (Catch2 + pytest templates, TDD workflow).
- Conventions: `.github/instructions/cpp-conventions.instructions.md` (C++20, RAII, naming, OpenCV guards).
- Conventions: `.github/instructions/qt-frontend-conventions.instructions.md` (Qt6, NodeDelegateModel, signals/slots, file structure).

## Competitive Research (2026-04-19)

### Key Findings
- 12 tools analyzed: ComfyUI, ML Forge, PaperVision, OpenCV-Flow, Starling/Harpia, awesome-node-based-uis, chaiNNer, plus arxiv/dev.to design resources.
- Usage principal confirmé: **prototypage rapide** (R&D).
- Plateforme **CV générique** (pas spécifique robotique).
- Priorité court terme: **vrai node editor graphique** (remplacer vue linéaire).

### chaiNNer Deep Dive
- **Stack**: Electron (React/TS/Chakra UI) + Python backend, JSON API.
- **Node system**: Python functions + rich decorators (`@group.register`), custom "Navi" type system, deterministic caching, data/type broadcasts for previews.
- **Forces**: 5.7k★, 38 contributors, mature node infra, type validation, OpenCV nodes, CLI batch, color-coded connections.
- **Faiblesses pour Noddle**: GPL-3.0, Python-only compute (pas de C++), Electron (pas Qt), batch-only (pas de temps réel caméra), image-only (pas point cloud/CSV).
- **Verdict fork**: Non recommandé comme fork direct. L'architecture Python/Electron diverge trop des objectifs C++/Qt/temps réel de Noddle.
- **À emprunter**: système de décorateurs pour nœuds, type system Navi, broadcasts pour preview, hiérarchie package/category/nodegroup, validation par couleurs de connexion.

### Fork Decision (Locked)
- **Aucun fork direct** — construire Noddle from scratch en empruntant les meilleurs patterns.
- Patterns retenus de chaiNNer: node metadata decorators, type validation, broadcast previews, category hierarchy.
- Patterns retenus de ComfyUI: ré-exécution sélective, workflow JSON, file d'exécution async.
- Patterns retenus de ML Forge: auto-inférence dimensions, export code autonome.
- Patterns retenus de Starling: génération C++ depuis graphe.
- Patterns retenus de dev.to: searchbox critique, pins dynamiques, gestion cyclique, side-effects marqués.

### Revised Feature Roadmap
- **Phase 1 (Priorité)**: Vrai node editor graphique Qt, searchbox, sérialisation JSON, auto-inférence types.
- **Phase 2**: Source caméra live, preview par nœud, overload strategies production, bibliothèque OpenCV étendue.
- **Phase 3** ✅: Export code C++, undo/redo, copy/paste, groupes/sous-pipelines, erreurs visuelles.

## Phase 3 — Live Camera Source (2026-04-19)
- `CameraSourceModel` added: Qt6 Multimedia-based live camera node (QCamera + QVideoSink).
  - Camera enumeration via `QMediaDevices::videoInputs()`.
  - Frame rate limiting at ~30fps via `QElapsedTimer` throttle.
  - Embedded widget: camera selector (QComboBox), Start/Stop toggle, 120x90 preview, FPS counter.
  - Split .hpp/.cpp (required for QMediaCaptureSession linking).
- `FpsCounter` utility class in `app/widgets/FpsCounter.hpp`.
- `PreviewPanel` updated: auto-refresh on data changes.
  - Connects to `DataFlowGraphModel::inPortDataWasSet` for downstream nodes.
  - Connects to selected node's `NodeDelegateModel::dataUpdated` for source nodes.
  - Tracks `m_selectedNodeId` — disconnects previous node on re-selection.
  - FPS display label in preview panel.
- `Qt6::Multimedia` added to CMake build (qt6-multimedia-dev 6.4.2).
- **Phase 4**: Plugins Python/C++ custom, file d'exécution async, ONNX Runtime, export exécutable.

## Phase 1 Status (2026-04-19) ✅ VALIDATED
- 4 NodeData types: TensorData, ImageData, TableData, PointCloudData (`app/data/`)
- 4 source/display models: ImageSourceModel, CsvSourceModel, PointCloudSourceModel, ImageDisplayModel (`app/nodes/`)
- 7 OpenCV models (conditional `NODDLE_WITH_OPENCV`): MatConvert, ColorConvert, Resize, GaussianBlur, Threshold, Canny, Morphology (`app/nodes/opencv/`)
- Full node registry in 3 categories (Sources, Display, OpenCV)
- Merged to `develop` from `feature/phase1-node-models`

## Phase 2 Status (2026-04-19) ✅ VALIDATED
- `NoddleMainWindow` class: proper QMainWindow subclass owning graph model, scene, view
- `PreviewPanel` QDockWidget: right dock, shows selected node caption + extracted ImageData preview
- File menu: New (Ctrl+N), Open (Ctrl+O), Save (Ctrl+S), Save As (Ctrl+Shift+S), Quit (Ctrl+Q)
- Edit menu: Undo/Redo via scene undo stack (shortcuts handled by QtNodes GraphicsView)
- View menu: Toggle Preview Panel
- JSON save/load: `.noddle` file format using DataFlowGraphModel::save()/load()
- Modified state tracking: `●` prefix in title, save prompt on close/new/open
- Refactored main.cpp to minimal 13-line entry point
- Merged to `develop` from `feature/phase2-main-window`

## Phase 3 Status (2026-04-19) ✅ VALIDATED
- `CameraSourceModel`: live camera source using Qt6 Multimedia (QCamera + QMediaCaptureSession + QVideoSink)
  - Camera device selection via QComboBox
  - Start/Stop toggle button
  - No frame throttle — delivers at camera native rate
  - FPS counter in embedded widget
  - 120x90 preview in node
- `FpsCounter`: utility class for frame rate measurement (averaged over 1 second)
- `ProcessingTimer`: utility class for processing time measurement (averaged over 10 samples)
  - Added to all 6 OpenCV models and ImageDisplayModel
  - Each node shows average processing time in ms in its embedded widget
- `PreviewPanel` enhanced:
  - Auto-refresh on `inPortDataWasSet` (downstream nodes) and `dataUpdated` (source nodes)
  - Node-specific delegate model connection/disconnection on selection change
  - FPS display for preview refresh rate
- `NoddleMainWindow` enhancements:
  - `clearScene()` + `undoStack().clear()` for proper load/new (fixes blocks disorder)
  - `centerScene()` after load to re-center view on loaded nodes
  - Default directory: `pipelines/` next to executable (creates if missing)
  - Recent Pipelines submenu (QSettings-based, max 8 entries, with Clear)
  - Open/Save dialogs accept both `.noddle` and `.ndl` extensions
- Qt6 Multimedia added: `qt6-multimedia-dev`, `Qt6::Multimedia` linked
- Build: 100%, Tests: 37/37, App: launches OK
- Branch: `feature/phase3-live-camera`

### Phase 3 — Camera Format Selector + Threaded Processing (2026-04-19)
- Camera format selector: QComboBox listing `QCameraDevice::videoFormats()` as "WxH @ fps"
  - Default selection: 640x480 if available
  - Camera restart on format change
- Threaded frame processing: `QtConcurrent::run()` offloads `toImage()` + format conversion
  - `std::atomic<bool> m_processing` frame-skip gate
  - `QMetaObject::invokeMethod` posts results back to UI thread
  - `m_processing.store(false)` released AFTER `invokeMethod` (in thread pool), not in UI callback
    - This decouples processing throughput from UI update latency
- Resolution display in FPS label: "640x480 @ 30.0 fps"
- Qt6::Concurrent added to build

### Phase 3 — Pipeline Profiling System (2026-04-19)
- `PipelineProfiler` singleton (`app/widgets/PipelineProfiler.hpp`):
  - Thread-safe (QMutex) central timing collection
  - `record(nodeCaption, stageName, durationMs)` — records stage timing
  - `markFrame(nodeCaption)` — marks frame boundary for FPS tracking
  - `stat(node, stage, StatType, TimeWindow)` — returns computed statistic
  - `fps(nodeCaption)` — FPS computed from frame markers (last 1.5s window)
  - `snapshot(StatType, TimeWindow)` — all stages with computed durations
  - `totalPipelineMs(StatType, TimeWindow)` — sum of all stage durations
  - StatType: Min, Max, Avg, Median
  - TimeWindow: AllTime, LastFrame, 10 Frames, 1s, 5s, 10s
  - Max 10000 entries per stage (ring buffer)
- `ScopeStageTimer` RAII helper: measures duration on construction/destruction, records to profiler
- All OpenCV nodes + ImageDisplayModel: `ProcessingTimer` replaced by `ScopeStageTimer`
  - Labels now query profiler for 1-second average
- CameraSourceModel: instrumented with stages "decode", "convert", "preview"
  - Frame boundary marked via `markFrame("Camera Source")`
- `TimelineView` QDockWidget (`app/TimelineView.hpp/.cpp`):
  - Custom-painted horizontal bar chart showing all profiled stages
  - Bars scaled relative to longest stage, color-coded per node
  - Node headers with indented stage rows
  - Total pipeline time at bottom
  - Bottom controls: Stat type combo (Avg/Min/Max/Median) + Time window combo
  - Auto-refresh at 5 Hz

### Phase 3 — Profiling & UI Polish (2026-04-19)
- `PipelineProfiler` singleton: thread-safe (QMutex) stage timing, per-node FPS, frame-based time windows, RAII `ScopeStageTimer`.
  - Copy/move constructors deleted (singleton enforcement).
  - `updated()` signal emitted outside mutex lock to avoid deadlock with UI slot handlers.
  - `markFrame()` validity guard: early return if globalTimer not yet started by `record()`.
- `NoddleConnectionPainter`: custom `AbstractConnectionPainter` showing data type, FPS, and per-stage timing on connections. Compact label on idle, detailed (with metadata) on hover/selection.
- `TimelineView`: dock widget with two modes — bar chart (stat/window selectable) and stacked single-frame timeline. Dual ruler (ms + fps). Mouse-wheel zoom. Tooltip on hover.
- Floating dock windows: `Qt::Window` flags on detach, movable to other screens. "Re-dock All Panels" in View menu (`Ctrl+Shift+D`).
- Recent files: `QSettings`-based recent pipeline menu, up to 8 entries, with Clear action.
- View transform persistence: scale + center saved/restored in pipeline JSON (`viewTransform` key).
- Node position sync: QGraphicsObject positions synced to model JSON before save (QtNodes bug workaround — dragged positions not always written back to model).
- Dark theme: global QSS scoped to `QGraphicsView` descendants only. `#nodePreview` exception for preview label background.
- `PreviewPanel`: supports sink nodes (0 output ports) via `outData(0)` fallback returning internally stored received data.
- Image format extension changed from `.noddle` to `.ndl`. Open dialog accepts both.
- File extension detection: `QFileInfo::suffix()` (not `contains('.')`).
- `ImageDisplayModel`: uses `Qt::FastTransformation` for live preview scaling.
- `ProcessingTimer`: deprecated in favor of `ScopeStageTimer`. Kept for reference.
- 56 tests passing (37 core + 19 widget tests).

### Phase 3 — Code Review Fixes (2026-04-19)
- CRITICAL-1: CameraSource use-after-free fixed — `QFuture::waitForFinished()` in `stopCamera()` before destroying camera objects.
- CRITICAL-2: PipelineProfiler `updated()` signal emitted outside mutex lock (was inside, risking deadlock).
- CRITICAL-3: `markFrame()` validity guard — early return if `m_globalTimer` not valid (no `record()` called yet).
- `ImageDisplayModel`: `FastTransformation` for live preview (was `SmoothTransformation`).
- PipelineProfiler: copy/move constructors deleted (singleton enforcement).
- `m_processing.store(false)` released BEFORE `QMetaObject::invokeMethod` to decouple processing throughput from UI latency.
  - Toggleable via View > Pipeline Timeline menu
- Status bar: permanent "Pipeline: X.XX ms" label, updated at 5 Hz
- `NoddleConnectionPainter` enhanced:
  - Compact label: shows data type + FPS if available (e.g., "Image 30.0fps")
  - Detailed label (hover/select): type + resolution + FPS (e.g., "Image 640×480 @ 30.0fps")
  - FPS queried from `PipelineProfiler::instance().fps(nodeCaption)`
  - Node caption obtained via `model.nodeData(nodeId, NodeRole::Caption)`

### Phase 3 — Timeline View v2 + UX Polish (2026-05-19)
- `TimelineView` rewritten with two modes via QStackedWidget:
  - **Bar chart mode** (`TimelineWidget`): horizontal bars with proper margins (kLeftMargin=10, kRightMargin=10, kDurTextWidth=70), duration text right-aligned. Auto-scale on first data then locked; user-controlled zoom via mouse wheel.
  - **Stacked timeline mode** (`StackedTimelineWidget`): gantt-like chronological lanes per node, time-sorted bars, auto-scroll to latest, wheel zoom around mouse position, hover tooltips (node/stage/duration) via QToolTip hit-test rects.
  - v2: Redesigned as single-lane sequential frame view — all pipeline stages side-by-side on one line, each node has its own color (same palette as bar chart). Zoom out capped at max frame processing time × 1.3. Color legend at bottom shows node-to-color mapping.
  - v3: Uses snapshot (avg durations) starting at t=0 instead of raw timeline events. Single frame display, no frame grouping.
  - Mode switch: "Stacked Timeline" QCheckBox in bottom controls.
- **Dual ruler** (`drawDualRuler()`): top half graduated in ms, bottom half in fps, nice tick spacing via log10/pow rounding, separator line.
- `PipelineProfiler` enhanced:
  - `TimelineEvent` struct: `nodeCaption`, `stageName`, `timestampMs`, `durationMs`
  - `timelineEvents(qint64 windowMs)` method: returns all entries from last windowMs, sorted by timestamp
- **Node position fix on load**: view transform (scale + center point) now saved in `.noddle` JSON under `"viewTransform"` key, restored on load instead of calling `centerScene()`. Falls back to `centerScene()` for legacy files.
- **Status bar FPS**: displays "Pipeline: X.XX ms (Y.Y fps)" instead of just ms.
- **Dark theme stylesheet**: global QSS scoped to `QGraphicsView` descendants in `main.cpp` — transparent background for container QWidget, QComboBox, QPushButton with semi-transparent backgrounds and light text. Does not affect docks or main window controls.
- **Dock floating**: Both PreviewPanel and TimelineView have `DockWidgetFloatable` feature + `AllDockWidgetAreas`, allowing detaching as independent windows and docking anywhere.
- **Profiling control bar styling**: QSS on control bar sets white text for QLabel/QComboBox/QCheckBox, visible checkbox border (#888), checked state highlight (#5080c0).
- Build: 100%, Tests: 37/37

### Phase 3 — Completion Features (2026-04-19)
- **Bar chart dezoom limit**: `TimelineWidget` (bar chart mode) zoom-out now capped at `m_totalMs * 1.3` (total pipeline time), matching stacked view behavior. Previously unlimited.
- **Edit menu actions**: Cut, Copy, Paste, Duplicate, Delete wired to QtNodes v3 `GraphicsView` built-in slots (`onCopySelectedObjects()`, `onPasteObjects()`, `onDuplicateSelectedObjects()`, `onDeleteSelectedObjects()`). No custom clipboard logic — QtNodes handles serialization, ID remapping, position offset, and undo/redo. Clipboard format: `application/qt-nodes-graph`. Shortcuts: Ctrl+C/V/X/D, Delete.
- **Node state persistence (save/load)**: All 11 `NodeDelegateModel` subclasses override `save()/load()` for configuration persistence:
  - ImageSourceModel: file path
  - CsvSourceModel: file path
  - PointCloudSourceModel: file path
  - CameraSourceModel: camera index + format index
  - GaussianBlurModel: kernel size
  - ColorConvertModel: conversion mode index
  - ResizeModel: width + height
  - ThresholdModel: threshold value + type index
  - CannyModel: low/high thresholds
  - MorphologyModel: operation index + kernel size
  - `load()` called before `embeddedWidget()` — backing member fields store values, widget init reads them
  - Source nodes (Image, CSV, PointCloud) reload data from path on `load()`
- **Visual error indicators**: All processing nodes set `NodeValidationState` in `setInData()`:
  - `Warning` when input is null/disconnected
  - `Error` on type mismatch (safety net)
  - `Valid` on successful input receipt
  - Source nodes: `Warning` initially ("No file/image loaded"), `Valid` after load, `Error` on failure
  - CameraSourceModel: warns when no cameras detected
  - Rendering by QtNodes' `DefaultNodePainter` (red/orange border + tooltip icon)
- **Code review fixes**:
  - ImageDisplayModel: `dynamic_pointer_cast` moved after null check
  - All OpenCV `process()` methods: widget null guards with backing field fallback
  - Pattern: `m_spin ? m_spin->value() : m_backingField` for all widget reads in `process()`
