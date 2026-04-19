# Noddle Architecture (Draft)

## Goals
- Keep visual feedback real-time by default during pipeline prototyping.
- Keep runtime modular so backends can evolve (CPU first, hardware backends later).
- Keep frontend replaceable while using Ryven now.

## High-Level Components
1. Graph UI Layer (Ryven integration)
2. Orchestration Layer (graph execution scheduling and state)
3. Compute Core (C++ tensor-first operators)
4. Python Bridge (custom blocks, adapters, and rapid extensions)
5. IO Adapters (camera, point cloud, files, csv)

## Canonical Data Model
- In-graph transport type: raw tensor + metadata.
- External formats are converted at graph boundaries by dedicated nodes.
- Minimal metadata contract for MVP: dimensions only.

## Visualization Overload Policy (MVP)
- Three selectable strategies coexist: drop frame, downsample, pause.
- Default strategy for streams: drop frame.
- Default strategy for single-frame workflows: pause.

## OpenCV Baseline Blocks (MVP)
- Color convert
- Resize
- Gaussian blur
- Threshold
- Canny
- Morphology

## Runtime Strategy
- Define abstract execution interfaces first.
- Start with CPU implementation.
- Add hardware-specific implementations behind the same interfaces.

## Export Strategy (Later)
- Optional export to standalone executable for faster batch execution.
- Optional export path for selected live processing scenarios.

## Non-goals for now
- Cross-platform support beyond Linux.
- Stable public plugin ABI.
- Packaging and distribution.
