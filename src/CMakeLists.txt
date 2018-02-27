# Link common libraries and include relevant directories
link_libraries(glfw ${GLFW_LIBRARIES} glad)
include_directories("${GLFW_DIR}/include" "${GLAD_DIR}/include" ${GLM_DIR})

# Set up source and header lists
set(open_sea_HEADERS
        "${open_sea_SOURCE_DIR}/include/open-sea/config.h"
        "${open_sea_SOURCE_DIR}/include/open-sea/Test.h"
        "${open_sea_SOURCE_DIR}/include/open-sea/Log.h"
        "${open_sea_SOURCE_DIR}/include/open-sea/Window.h"
        "${open_sea_SOURCE_DIR}/include/open-sea/Input.h")
set(open_sea_SOURCES
        Test.cpp
        Log.cpp
        Window.cpp
        Input.cpp)

# Add the library
add_library(open_sea ${open_sea_SOURCES} ${open_sea_HEADERS})
