# Link common libraries and include relevant directories
link_libraries(open_sea ${Boost_LIBRARIES})
include_directories(SYSTEM ${INCL_DIR} "${GLFW_DIR}/include" "${GLAD_DIR}/include" ${GLM_DIR} ${ImGui_DIR} ${Boost_INCLUDE_DIRS})

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
