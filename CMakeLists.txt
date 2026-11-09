
# CMakeLists.txt written by Noerlol, https://github.com/xNoerPlaysCodes

cmake_minimum_required(VERSION 4.1)
project(Lesset C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 26)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES --define-macro=LESSET_INCLUDE_DEBUG")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -freflection --define-macro=LESSET_INCLUDE_DEBUG")
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
include(FetchContent)

FetchContent_Declare(
    libglfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
)
FetchContent_MakeAvailable(libglfw)

FetchContent_Declare(
    libimgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(libimgui)

FetchContent_Declare(
    libimplot
    GIT_REPOSITORY https://github.com/epezent/implot
    GIT_TAG master
)

FetchContent_MakeAvailable(libimplot)

find_package(Vulkan REQUIRED)
set(LIBRARIES "glfw;Vulkan::Vulkan")

add_library(imgui STATIC
        ${libimgui_SOURCE_DIR}/imgui.cpp
        ${libimgui_SOURCE_DIR}/imgui_draw.cpp
        ${libimgui_SOURCE_DIR}/imgui_demo.cpp
        ${libimgui_SOURCE_DIR}/imgui_tables.cpp
        ${libimgui_SOURCE_DIR}/imgui_widgets.cpp
        ${libimgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        ${libimgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${libimgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

add_library(implot STATIC
    ${libimplot_SOURCE_DIR}/implot.cpp ${libimplot_SOURCE_DIR}/implot_items.cpp
)

target_include_directories(implot PRIVATE ${libimgui_SOURCE_DIR})

target_include_directories(imgui PUBLIC ${libimgui_SOURCE_DIR} ${libglfw_SOURCE_DIR}/include)

# Use vulkan headers from glfw:
include_directories(${GLFW_DIR}/deps)

file(GLOB sources *.cpp)

add_executable(Lesset ${sources})
target_include_directories(Lesset PRIVATE
        ${libimgui_SOURCE_DIR}
        ${libimgui_SOURCE_DIR}/backends
        ${libimgui_SOURCE_DIR}/misc/cpp
        ${libimplot_SOURCE_DIR}
        ${libglfw_SOURCE_DIR}/include
)
target_link_libraries(Lesset ${LIBRARIES} imgui glfw implot nfd)
