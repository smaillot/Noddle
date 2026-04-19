cmake_minimum_required(VERSION 3.20)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Concurrent)
qt_standard_project_setup()

set(APP_SOURCES
    main.cpp
    NoddleMainWindow.hpp
    NoddleMainWindow.cpp
    PreviewPanel.hpp
    PreviewPanel.cpp
    nodes/ImageSourceModel.hpp
    nodes/CsvSourceModel.hpp
    nodes/PointCloudSourceModel.hpp
    nodes/ImageDisplayModel.hpp
    nodes/opencv/ColorConvertModel.hpp
    nodes/opencv/ResizeModel.hpp
    nodes/opencv/GaussianBlurModel.hpp
    nodes/opencv/ThresholdModel.hpp
    nodes/opencv/CannyModel.hpp
    nodes/CameraSourceModel.hpp
    nodes/CameraSourceModel.cpp
    widgets/FpsCounter.hpp
    widgets/ProcessingTimer.hpp
    widgets/PipelineProfiler.hpp
    nodes/opencv/MorphologyModel.hpp
    NoddleConnectionPainter.hpp
    NoddleConnectionPainter.cpp
    TimelineView.hpp
    TimelineView.cpp
)

add_executable(noddle_app ${APP_SOURCES})

target_include_directories(noddle_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(noddle_app PRIVATE
    NODDLE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(noddle_app
    PRIVATE
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::Concurrent
        QtNodes::QtNodes
        noddle_core
)

if(NODDLE_WITH_OPENCV)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc)
    target_compile_definitions(noddle_app PRIVATE NODDLE_WITH_OPENCV)
    target_link_libraries(noddle_app PRIVATE ${OpenCV_LIBS})
endif()
