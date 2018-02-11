# Set location of files to use
set(DOC_SRC "${INCL_DIR}/open-sea/Log.h"
        "${SRC_DIR}/Log.cpp")

# Set Doxygen options
set(DOXYGEN_PROJECT_NAME "Open Sea")
set(DOXYGEN_PROJECT_NUMBER ${open_sea_VERSION_FULL})

doxygen_add_docs(
    doc
    ${DOC_SRC}
    COMMENT "Generate doc"
)
