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
)

include_directories(extern/glad)
include_directories(extern/)
include_directories(extern/imgui)

# SDL3
find_package(SDL3 REQUIRED)
# CV
find_package(OpenCV REQUIRED)

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

add_executable(AimsOrchestrator ${SOURCES} extern/gl.c)

# link imgui
target_link_libraries(AimsOrchestrator PRIVATE imgui)

# link cv
target_include_directories(AimsOrchestrator PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(AimsOrchestrator PRIVATE ${OpenCV_LIBS})

#SDL3 link
target_include_directories(AimsOrchestrator PRIVATE ${SDL3_INCLUDE_DIRS})
target_link_libraries(AimsOrchestrator PRIVATE ${SDL3_LIBRARIES})

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