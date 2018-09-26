# Link common libraries and include relevant directories
link_libraries(glfw ${GLFW_LIBRARIES} glad)
include_directories(${INCL_DIR} "${GLFW_DIR}/include" "${GLAD_DIR}/include" ${GLM_DIR} ${ImGui_DIR} ${Boost_INCLUDE_DIRS})

# Set up source and header lists
set(open_sea_HEADERS
        "${INCL_DIR}/open-sea/config.h"
        "${INCL_DIR}/open-sea/Log.h"
        "${INCL_DIR}/open-sea/Window.h"
        "${INCL_DIR}/open-sea/Input.h"
        "${INCL_DIR}/open-sea/ImGui.h"
        "${INCL_DIR}/open-sea/Delta.h"
        "${INCL_DIR}/open-sea/GL.h"
        "${INCL_DIR}/open-sea/Model.h"
        "${INCL_DIR}/open-sea/Entity.h"
        "${INCL_DIR}/open-sea/Components.h"
        "${INCL_DIR}/open-sea/Render.h"
        "${INCL_DIR}/open-sea/Systems.h"
        "${INCL_DIR}/open-sea/Controls.h"
        "${INCL_DIR}/open-sea/Debug.h"
        "${INCL_DIR}/open-sea/Debuggable.h"
        "${INCL_DIR}/open-sea/Profiler.h"
        "${INCL_DIR}/open-sea/Track.h")
set(open_sea_SOURCES
        "${SRC_DIR}/Log.cpp"
        "${SRC_DIR}/Window.cpp"
        "${SRC_DIR}/Input.cpp"
        "${SRC_DIR}/ImGui.cpp"
        "${SRC_DIR}/Delta.cpp"
        "${SRC_DIR}/GL.cpp"
        "${SRC_DIR}/Model.cpp"
        "${SRC_DIR}/Entity.cpp"
        "${SRC_DIR}/Components.cpp"
        "${SRC_DIR}/Render.cpp"
        "${SRC_DIR}/Systems.cpp"
        "${SRC_DIR}/Controls.cpp"
        "${SRC_DIR}/Debug.cpp"
        "${SRC_DIR}/Profiler.cpp")

# Copy source and header lists to parent for use in documentation
set(open_sea_HEADERS ${open_sea_HEADERS} PARENT_SCOPE)
set(open_sea_SOURCES ${open_sea_SOURCES} PARENT_SCOPE)

# Add the library
add_library(open_sea ${open_sea_SOURCES} ${open_sea_HEADERS} ${ImGui})
