# Set location of files to use
list(APPEND DOC_SRC ${open_sea_HEADERS})
list(APPEND DOC_SRC ${open_sea_SOURCES})

# Set Doxygen options
set(DOXYGEN_PROJECT_NAME "Open Sea")
set(DOXYGEN_PROJECT_NUMBER ${open_sea_VERSION_FULL})

doxygen_add_docs(
    doc
    ${DOC_SRC}
    COMMENT "Generate doc"
)
