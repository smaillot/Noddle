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
- Frontend MVP created in `frontend/ryven/app.py` with:
	- block catalog
	- linear pipeline editor view
	- live preview panel
	- overload strategy controls aligned with runtime policy
- Baseline block list in frontend matches MVP source and OpenCV nodes.
- Python syntax validation passed (`python3 -m py_compile app.py`).

## Tooling Updates (2026-04-19)
- Added root launcher script `start_frontend.sh`.
- Script targets venv `~/.venv/noddle` by default and supports override via `NODDLE_VENV`.
- Script syncs frontend requirements then starts `frontend/ryven/app.py`.

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
- **Phase 4**: Plugins Python/C++ custom, file d'exécution async, ONNX Runtime, export exécutable.
