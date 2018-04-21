# Link common libraries and include relevant directories
link_libraries(glfw ${GLFW_LIBRARIES} open_sea glad ${Boost_LIBRARIES})
include_directories("${GLFW_DIR}/include" "${GLAD_DIR}/include" ${GLM_DIR} ${ImGui_DIR})

# Add the sample game as executable
add_executable(sample-game "SampleGame.cpp")

# Declare models
set(MODELS data/models/teapot.obj
        data/models/triangle.obj
        data/models/cube.obj)
list(APPEND DATA_FILES ${MODELS})

# Copy all data files
foreach(FILE ${DATA_FILES})
    configure_file(${FILE} ${FILE} COPYONLY)
endforeach()

# Unset local variables
unset(MODELS)
unset(DATA_FILES)
