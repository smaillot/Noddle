# Noddle

Noddle is a Linux-first modular flow-programming platform for computer vision.

## Current Direction
- UI baseline: Ryven (Python/Qt) for rapid visual graph iteration.
- Runtime: mixed C++ backend + Python workers.
- Internal graph data model: raw tensor.
- Compute strategy: abstraction first, hardware acceleration backends added incrementally.

## Repository Layout
- `docs/`: architecture notes and roadmap.
- `backend/cpp/`: C++ core runtime and compute abstractions.
- `backend/python/`: Python bridge and worker-side adapters.
- `frontend/ryven/`: Ryven integration files.
- `pipelines/examples/`: example graph pipelines.

## First Milestone (MVP)
Multimodal pipeline prototype with continuous real-time visualization when possible:
- image source
- point cloud source
- CSV source
- visualization-enabled processing loop

Implemented in this scaffold:
- C++ tensor-first runtime skeleton with source-node interface.
- MVP source nodes: image, csv, pointcloud.
- Visualization overload policy with selectable modes:
	- drop frame
	- downsample
	- pause
- Default overload behavior:
	- stream: drop frame
	- single frame: pause
- Baseline OpenCV node classes:
	- color convert
	- resize
	- gaussian blur
	- threshold
	- canny
	- morphology

See `docs/architecture.md` and `docs/roadmap.md` for details.

## Frontend Quick Start

From project root:

```bash
bash start_frontend.sh
```

Notes:
- Default venv path is `~/.venv/noddle`.
- Override with `NODDLE_VENV=/path/to/venv bash start_frontend.sh`.
