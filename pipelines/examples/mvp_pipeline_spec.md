# MVP Pipeline Spec

## Goal
Validate multimodal source ingestion and baseline transform path with real-time visualization policy.

## Source Nodes
1. `source.image` -> tensor (placeholder in current scaffold)
2. `source.csv` -> tensor shape [rows, cols]
3. `source.pointcloud` -> tensor shape [rows, 3]

## Transform Nodes (OpenCV)
1. `opencv.cvt_color`
2. `opencv.resize`
3. `opencv.gaussian_blur`
4. `opencv.threshold`
5. `opencv.canny`
6. `opencv.morphology`

## Visualization Policy
- Strategies available: drop frame, downsample, pause
- Defaults:
  - stream: drop frame
  - single frame: pause

## Execution Notes
- Canonical payload is raw tensor.
- Metadata is dimensions-only.
- External formats should be normalized by import/source nodes.
