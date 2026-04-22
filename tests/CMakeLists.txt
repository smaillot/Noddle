include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# Qt6::Core and Widgets needed for widget and engine tests
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC ON)

add_executable(noddle_tests
    test_tensor.cpp
    test_visualization_policy.cpp
    test_source_nodes.cpp
    test_runtime.cpp
    test_python_plugin_runtime.cpp
    test_python_plugin_node_model.cpp
    test_widgets.cpp
    test_engine.cpp
    # Listed explicitly so AUTOMOC generates moc for Q_OBJECT macros
    ${CMAKE_SOURCE_DIR}/app/widgets/PipelineProfiler.hpp
    ${CMAKE_SOURCE_DIR}/app/nodes/PythonPluginModel.hpp
    DummyModels.hpp
    # Engine sources needed by test_engine.cpp
    ${CMAKE_SOURCE_DIR}/app/engine/GraphSchedule.cpp
    ${CMAKE_SOURCE_DIR}/app/plugins/PythonPluginRuntime.cpp
)

target_include_directories(noddle_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/widgets
        ${CMAKE_SOURCE_DIR}/app
)

target_link_libraries(noddle_tests
    PRIVATE
        Catch2::Catch2WithMain
        noddle_core
        Qt6::Core
        Qt6::Widgets
        QtNodes::QtNodes
)

if(NODDLE_WITH_PYTHON_PLUGIN)
    find_package(Python3 COMPONENTS Interpreter Development QUIET)
    if(Python3_Development_FOUND)
        target_compile_definitions(noddle_tests PRIVATE NODDLE_WITH_PYTHON_PLUGIN)
        target_link_libraries(noddle_tests PRIVATE Python3::Python)
    endif()
endif()

include(CTest)
include(Catch)
catch_discover_tests(noddle_tests)

# ── Integration tests (need QCoreApplication event loop) ─────────

find_package(Qt6 REQUIRED COMPONENTS Test Concurrent)

add_executable(noddle_integration_tests
    test_integration_engine.cpp
    DummyModels.hpp
    ${CMAKE_SOURCE_DIR}/app/engine/GraphSchedule.cpp
    ${CMAKE_SOURCE_DIR}/app/engine/PipelineExecutor.cpp
)

target_include_directories(noddle_integration_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/app/widgets
)

target_link_libraries(noddle_integration_tests
    PRIVATE
        Catch2::Catch2
        noddle_core
        Qt6::Core
        Qt6::Widgets
        Qt6::Test
        Qt6::Concurrent
        QtNodes::QtNodes
)

catch_discover_tests(noddle_integration_tests)

# Run tests automatically after build
add_custom_command(
    TARGET noddle_tests POST_BUILD
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Noddle unit tests..."
)
