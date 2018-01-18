# Set up source and header lists
set(open_sea_HEADERS "${open_sea_SOURCE_DIR}/include/open-sea/Test.h")
set(open_sea_SOURCES Test.cpp)

# Add the library
add_library(open_sea ${open_sea_SOURCES} ${open_sea_HEADERS})