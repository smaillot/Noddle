# noddle_core — C++ runtime library
# Can be built standalone or as a subdirectory of the root project.

cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_PROJECT_NAME)
    project(noddle_core LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(NODDLE_WITH_OPENCV)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
elseif(NOT DEFINED NODDLE_WITH_OPENCV)
    find_package(OpenCV QUIET COMPONENTS core imgproc imgcodecs)
endif()

add_library(noddle_core
    src/runtime.cpp
    src/source_nodes.cpp
    src/opencv_nodes.cpp
)

target_include_directories(noddle_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(OpenCV_FOUND)
    target_compile_definitions(noddle_core PUBLIC NODDLE_WITH_OPENCV)
    target_link_libraries(noddle_core PUBLIC ${OpenCV_LIBS})
endif()

add_executable(noddle_core_demo src/main.cpp)
target_link_libraries(noddle_core_demo PRIVATE noddle_core)
