# Declare shaders
set(SHADERS shaders/ImGui.vshader
        shaders/ImGui.fshader
        shaders/Test.vshader
        shaders/Test.fshader)
list(APPEND DATA_FILES ${SHADERS})

# Declare models
set(MODELS )
list(APPEND DATA_FILES ${MODELS})

# Copy all data files
foreach(FILE ${DATA_FILES})
    configure_file(${FILE} ${FILE} COPYONLY)
endforeach()

# Unset local variables
unset(SHADERS)
unset(MODELS)
unset(DATA_FILES)
