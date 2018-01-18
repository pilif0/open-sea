# Link common libraries and include relevant directories
link_libraries(glfw ${GLFW_LIBRARIES} open_sea glad)
include_directories("${GLFW_DIR}/include" "${GLAD_DIR}/include" "${INCL_DIR}/open-sea")

# Add the sample game as executable
add_executable("sample-game" "sample-game/SampleGame.cpp")