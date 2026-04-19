cmake_minimum_required(VERSION 3.20)
project(noddle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Options ──────────────────────────────────────────────────────
option(NODDLE_WITH_OPENCV "Build with OpenCV nodes" OFF)

# ── Core library ─────────────────────────────────────────────────
add_subdirectory(backend/cpp)

# ── QtNodes (node editor framework) ─────────────────────────────
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(USE_QT6 ON CACHE BOOL "" FORCE)
add_subdirectory(external/nodeeditor)

# ── Application ──────────────────────────────────────────────────
add_subdirectory(app)
