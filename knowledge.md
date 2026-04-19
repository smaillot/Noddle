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
- **Phase 3**: Export code C++, undo/redo, copy/paste, groupes/sous-pipelines, erreurs visuelles.

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

## Phase 3 Status (2026-04-19) — IN PROGRESS
- `CameraSourceModel`: live camera source using Qt6 Multimedia (QCamera + QMediaCaptureSession + QVideoSink)
  - Camera device selection via QComboBox
  - Start/Stop toggle button
  - Frame rate throttle at ~30fps (QElapsedTimer)
  - FPS counter in embedded widget
  - 120x90 preview in node
- `FpsCounter`: utility class for frame rate measurement (averaged over 1 second)
- `PreviewPanel` enhanced:
  - Auto-refresh on `inPortDataWasSet` (downstream nodes) and `dataUpdated` (source nodes)
  - Node-specific delegate model connection/disconnection on selection change
  - FPS display for preview refresh rate
- Qt6 Multimedia added: `qt6-multimedia-dev`, `Qt6::Multimedia` linked
- Build: 100%, Tests: 37/37, App: launches OK
- Branch: `feature/phase3-live-camera`
