cmake_minimum_required(VERSION 3.9)
project(open_sea)

set(CMAKE_CXX_STANDARD 17)

# Set project version
set(open_sea_VERSION_MAJOR "0")
set(open_sea_VERSION_MINOR "1")
set(open_sea_VERSION_PATCH "0")
set(open_sea_VERSION_EXTRA "")
set(open_sea_VERSION "${open_sea_VERSION_MAJOR}.${open_sea_VERSION_MINOR}")
set(open_sea_VERSION_FULL "${open_sea_VERSION_MAJOR}.${open_sea_VERSION_MINOR}.${open_sea_VERSION_PATCH}${open_sea_VERSION_EXTRA}")

# Declare options
option(open_sea_BUILD_EXAMPLES "Build example programs" ON)
option(open_sea_DEBUG "Debug mode" OFF)
option(open_sea_BUILD_DOC "Build documentation" ON)

# Set some global locations
set(SRC_DIR "${open_sea_SOURCE_DIR}/src")
set(INCL_DIR "${open_sea_SOURCE_DIR}/include")
set(DEP_DIR "${open_sea_SOURCE_DIR}/deps")

# Boost
set(BOOST_ROOT /opt/boost)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package (Boost 1.66.0 REQUIRED COMPONENTS log filesystem)

# GLFW
set(GLFW_DIR "${DEP_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${GLFW_DIR}")

# Glad
set(GLAD_DIR "${DEP_DIR}/glad")
add_library(glad "${GLAD_DIR}/src/glad.c")
target_include_directories("glad" PRIVATE "${GLAD_DIR}/include")
set(GLAD "${open_sea_SOURCE_DIR}/deps/glad/include/glad/glad.h"
         "${open_sea_SOURCE_DIR}/deps/glad/src/glad.c")

# GLM
set(GLM_DIR "${DEP_DIR}/glm")

# ImGUI
set(ImGui_DIR "${DEP_DIR}/imgui")
set(ImGui "${ImGui_DIR}/imgui.h"
        "${ImGui_DIR}/stb_rect_pack.h"
        "${ImGui_DIR}/stb_textedit.h"
        "${ImGui_DIR}/stb_truetype.h"
        "${ImGui_DIR}/imgui.cpp"
        "${ImGui_DIR}/imgui_draw.cpp"
        "${ImGui_DIR}/imgui_demo.cpp")

# Doxygen
if (open_sea_BUILD_DOC)
    find_package(Doxygen)
endif()

# Configure header
set(OPEN_SEA_DEBUG ${open_sea_DEBUG})
configure_file(
        "${INCL_DIR}/open-sea/config.h.in"
        "${INCL_DIR}/open-sea/config.h"
)

# Include own public interface
include_directories(${INCL_DIR})        # This feels wrong but means not having to navigate to the directory when including

# Copy data
file(COPY data DESTINATION ./)

# Add subdirectories
add_subdirectory(src)

if (open_sea_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (open_sea_BUILD_DOC)
    add_subdirectory(doc)
endif()
