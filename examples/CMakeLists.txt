# Link common libraries and include relevant directories
link_libraries(glfw ${GLFW_LIBRARIES} open_sea glad ${Boost_LIBRARIES})
include_directories("${GLFW_DIR}/include" "${GLAD_DIR}/include" "${INCL_DIR}/open-sea" ${Boost_INCULDE_DIRS})

# Add the sample game as executable
add_executable(sample-game "sample-game/SampleGame.cpp")