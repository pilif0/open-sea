# Link common libraries and include relevant directories
link_libraries(glfw ${GLFW_LIBRARIES} glad)
include_directories("${GLFW_DIR}/include" "${GLAD_DIR}/include" ${GLM_DIR} ${ImGui_DIR})

# Set up source and header lists
set(open_sea_HEADERS
        "${INCL_DIR}/open-sea/config.h"
        "${INCL_DIR}/open-sea/Test.h"
        "${INCL_DIR}/open-sea/Log.h"
        "${INCL_DIR}/open-sea/Window.h"
        "${INCL_DIR}/open-sea/Input.h"
        "${INCL_DIR}/open-sea/ImGUI.h")
set(open_sea_SOURCES
        Test.cpp
        Log.cpp
        Window.cpp
        Input.cpp
        ImGui.cpp)

# Add the library
add_library(open_sea ${open_sea_SOURCES} ${open_sea_HEADERS} ${ImGui})
