cmake_minimum_required(VERSION 3.9)
project(open_sea)

set(CMAKE_CXX_STANDARD 11)

# Set project version
set(open_sea_VERSION_MAJOR "0")
set(open_sea_VERSION_MINOR "1")
set(open_sea_VERSION_PATCH "0")
set(open_sea_VERSION_EXTRA "")
set(open_sea_VERSION "${open_sea_VERSION_MAJOR}.${open_sea_VERSION_MINOR}")
set(open_sea_VERSION_FULL "${open_sea_VERSION_MAJOR}.${open_sea_VERSION_MINOR}.${open_sea_VERSION_PATCH}${open_sea_VERSION_EXTRA}")

# Declare options
option(open_sea_BUILD_EXAMPLES "Build example programs" ON)

# Set some global locations
set(SRC_DIR "${open_sea_SOURCE_DIR}/src")
set(INCL_DIR "${open_sea_SOURCE_DIR}/include")
set(DEP_DIR "${open_sea_SOURCE_DIR}/deps")

# GLFW
set(GLFW_DIR "${DEP_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${GLFW_DIR}")
include_directories(deps/glfw/include)

# Glad
set(GLAD_DIR "${DEP_DIR}/glad")
add_library(glad "${GLAD_DIR}/src/glad.c")
target_include_directories("glad" PRIVATE "${GLAD_DIR}/include")
set(GLAD "${open_sea_SOURCE_DIR}/deps/glad/include/glad/glad.h"
         "${open_sea_SOURCE_DIR}/deps/glad/src/glad.c")

# Include own public interface
include_directories(include)        # This feels wrong but means not having to navigate to the directory when including

# Add subdirectories
add_subdirectory(src)

if (open_sea_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()