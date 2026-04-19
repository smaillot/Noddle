---
applyTo: "backend/cpp/**"
---
# C++ Backend Conventions

Use when editing files in `backend/cpp/`.

## Language & Toolchain
- C++20 standard (concepts, ranges, designated initializers OK)
- GCC 13+ / Clang 17+, Linux-only
- CMake ≥ 3.16

## Naming
- Types: `PascalCase` (e.g., `TensorPacket`, `ImageSourceNode`)
- Functions/methods: `snake_case` (e.g., `make_cpu_backend()`)
- Member variables: `snake_case` with no prefix (e.g., `data`, `dtype`)
- Constants/enums: `PascalCase` values (e.g., `DType::Float32`)
- Namespaces: `noddle::core`, `noddle::cv`
- Files: `snake_case.hpp` / `snake_case.cpp`

## Code Style
- RAII everywhere — no raw `new`/`delete`
- Prefer `std::unique_ptr` / `std::shared_ptr` for ownership
- Use `std::span`, `std::string_view` for non-owning references
- Mark single-argument constructors `explicit`
- Use `[[nodiscard]]` on functions returning values that should not be ignored
- Prefer `enum class` over plain `enum`
- **No exceptions in hot paths** — use `std::optional` or `std::expected` for fallible ops

## Headers
- Use `#pragma once`
- Include order: project headers, Qt headers, third-party, standard library
- Forward-declare where possible to reduce coupling

## OpenCV Guard
- All OpenCV-dependent code must be guarded by `#ifdef NODDLE_WITH_OPENCV`
- CMake option: `option(NODDLE_WITH_OPENCV "Build with OpenCV nodes" OFF)`

## Interfaces
- Node interface: inherit `INode` from `runtime.hpp`
- Source nodes: inherit `ISourceNode`
- Pure virtual `process()` for compute nodes, `next()` for source nodes
- Canonical data type is `TensorPacket` from `tensor.hpp`

## Testing
- Catch2 v3 via FetchContent
- Test files in `tests/test_*.cpp`
- Tags match component: `[tensor]`, `[runtime]`, `[opencv]`, `[source]`
