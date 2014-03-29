project (lvfs-arc)

# Project header
project_cxx_library_header_default ()

# 3rdparty
list (APPEND ${PROJECT_NAME}_LIBS lvfs)

find_package (LibArchive REQUIRED)
include_directories (${LIBARCHIVE_INCLUDE_DIR})
list (APPEND ${PROJECT_NAME}_LIBS ${LIBARCHIVE_LIBRARY})

find_package (LibUnrar REQUIRED)
include_directories (${LIBUNRAR_INCLUDE})
list (APPEND ${PROJECT_NAME}_LIBS ${LIBUNRAR_LIBRARY})

# Sources
add_subdirectory (src)

# Target - lvfs-arc
add_library(lvfs-arc STATIC ${${PROJECT_NAME}_SOURCES})
target_link_libraries (lvfs-arc ${${PROJECT_NAME}_LIBS})

# Version
build_version_file (lvfs-arc 0.0.1)

# Documentation
add_documentation ("LVFS-Arc" 0.0.1 "LVFS Plugin for reading archive files")
