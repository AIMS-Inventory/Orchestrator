cmake_minimum_required(VERSION 4.1)
project(AimsOrchestrator)

set(CMAKE_CXX_STANDARD 23)

set(SOURCES
    src/main.cpp
    src/graphics/GraphicsInitialize.cpp
    src/graphics/GraphicsInitialize.hpp
    src/Orchestrator.cpp
    src/Orchestrator.hpp
    src/graphics/View.cpp
    src/graphics/View.hpp
    src/io/FileIo.cpp
    src/io/FileIo.hpp
    src/io/CameraInput.cpp
    src/io/CameraInput.hpp
    src/events/Event.hpp
    src/models/KnownBox.cpp
    src/models/KnownBox.hpp
    src/events/PythonEvents.cpp
    src/events/PythonEvents.hpp
    src/events/PythonEventRegistrar.cpp
    src/events/PythonEventRegistrar.hpp
    src/events/DebugEventUI.cpp
    src/events/DebugEventUI.hpp
    src/facial-recognition/FacialRecognition.cpp
    src/facial-recognition/FacialRecognition.hpp
    src/facial-recognition/PersonEntry.hpp
    src/facial-recognition/MedicationCheckoutEvent.hpp
    src/models/Code.hpp
    src/code-detection/CodeDetector.cpp
    src/code-detection/CodeDetector.hpp
    src/pills/PillRecognition.cpp
    src/pills/PillRecognition.hpp
    src/pills/Pill.hpp
)

include_directories(extern/glad)
include_directories(extern/)
include_directories(extern/imgui)

# SDL3
find_package(SDL3 REQUIRED)
# CV
find_package(OpenCV REQUIRED)
# rapidYaml
find_package(ryml REQUIRED)

set(PYBIND11_FINDPYTHON ON)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

#IMGui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui")
set(IMGUI_BACKEND_DIR "${IMGUI_DIR}/backends")

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMGUI_BACKEND_DIR}/imgui_impl_sdl3.cpp
    ${IMGUI_BACKEND_DIR}/imgui_impl_opengl3.cpp
)

# compile imgui as a lib
add_library(imgui STATIC ${IMGUI_SOURCES})

# add sdl to imgui
target_link_libraries(imgui PRIVATE sdl::sdl)

add_executable(AimsOrchestrator ${SOURCES} extern/gl.c)

# copy runtime resources
set(AIMS_ORCHESTRATOR_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(AIMS_ORCHESTRATOR_RUNTIME_RESOURCES_DIR "$<TARGET_FILE_DIR:AimsOrchestrator>/resources")

file(GLOB_RECURSE AIMS_ORCHESTRATOR_RESOURCE_FILES
    CONFIGURE_DEPENDS
    "${AIMS_ORCHESTRATOR_RESOURCES_DIR}/*"
)

set(AIMS_ORCHESTRATOR_RESOURCE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/resources-sync.stamp")

add_custom_command(
    OUTPUT "${AIMS_ORCHESTRATOR_RESOURCE_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${AIMS_ORCHESTRATOR_RESOURCES_DIR}"
            "${AIMS_ORCHESTRATOR_RUNTIME_RESOURCES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${AIMS_ORCHESTRATOR_RESOURCE_STAMP}"
    DEPENDS ${AIMS_ORCHESTRATOR_RESOURCE_FILES}
    COMMENT "Syncing resources to the runtime output directory"
    VERBATIM
)

add_custom_target(sync_resources DEPENDS "${AIMS_ORCHESTRATOR_RESOURCE_STAMP}")
add_dependencies(AimsOrchestrator sync_resources)

# link imgui
target_link_libraries(AimsOrchestrator PRIVATE imgui)

# link cv
#target_include_directories(AimsOrchestrator PRIVATE opencv::opencv)
target_link_libraries(AimsOrchestrator PRIVATE opencv::opencv)

#SDL3 link
#target_include_directories(AimsOrchestrator PRIVATE sdl::sdl)
target_link_libraries(AimsOrchestrator PRIVATE sdl::sdl)

# rapidYaml link
target_link_libraries(AimsOrchestrator PRIVATE ryml::ryml)
target_link_libraries(AimsOrchestrator PRIVATE pybind11::embed)
target_compile_definitions(AimsOrchestrator PRIVATE AIMS_ENABLE_PYTHON_EMBED=1)

set_target_properties(AimsOrchestrator PROPERTIES COMPILE_WARNING_AS_ERROR ON)

if(MSVC)
    target_compile_options(AimsOrchestrator PRIVATE /W4 /permissive-)
else()
    target_compile_options(AimsOrchestrator PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

if (NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(AimsOrchestrator PUBLIC -fsanitize=address -fno-omit-frame-pointer) # catch warns
    target_link_options(AimsOrchestrator PUBLIC -fsanitize=address)
endif()

if (NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(AimsOrchestrator PRIVATE -O3) # optimize for speed
endif()