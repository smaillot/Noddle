# Noddle Roadmap (Initial)

## Phase 0 - Foundations
- Define graph contracts: tensor schema, metadata, node input/output compatibility.
- Create C++ runtime skeleton with execution abstraction.
- Prepare Python bridge package structure.
- Prepare Ryven integration folder structure.

## Phase 1 - MVP Multimodal Prototype
- Implement source nodes:
  - image source
  - point cloud source
  - csv source
- Implement baseline transform nodes (OpenCV core set).
- Implement real-time visualization path with user toggle.
- Validate end-to-end pipeline assembly in Ryven.

### Phase 1 Decisions Applied
- Canonical payload metadata is limited to tensor dimensions.
- Overload modes coexist and are user-selectable.
- Defaults are stream=drop-frame and single-frame=pause.
- Baseline OpenCV blocks are: color convert, resize, gaussian blur, threshold, canny, morphology.

## Phase 2 - Reliability and Performance
- Add test harness for node contracts and pipeline execution.
- Add profiler hooks for pipeline-level optimization.
- Improve scheduling behavior for mixed source rates.

## Phase 3 - Export Path
- Introduce graph-to-executable export flow for batch processing.
- Add selective live pipeline export for relevant streaming outputs.

## Decision Rules
- Prefer internal API evolution over early ABI stability.
- Keep frontend integration modular to allow replacing Ryven later.
- Optimize per pipeline, not against a fixed global latency target.
