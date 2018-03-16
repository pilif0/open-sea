# Declare shaders
set(SHADERS shaders/ImGui.vshader
        shaders/ImGui.fshader)
list(APPEND DATA_FILES ${SHADERS})

# Copy all data files
foreach(FILE ${DATA_FILES})
    configure_file(${FILE} ${FILE} COPYONLY)
endforeach()
