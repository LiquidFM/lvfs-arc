project (lvfs-arc)

# Project header
project_cxx_library_header_default ()

# 3rdparty
list (APPEND ${PROJECT_NAME}_LIBS brolly)
list (APPEND ${PROJECT_NAME}_LIBS efc)
list (APPEND ${PROJECT_NAME}_LIBS lvfs)
list (APPEND ${PROJECT_NAME}_LIBS unrar)

find_package (LibArchive REQUIRED)
include_directories (${LIBARCHIVE_INCLUDE_DIR})
list (APPEND ${PROJECT_NAME}_LIBS ${LIBARCHIVE_LIBRARY})

# Sources
add_subdirectory (src)

# Target - lvfs-arc
add_library (lvfs-arc SHARED ${${PROJECT_NAME}_SOURCES})
target_link_libraries (lvfs-arc ${${PROJECT_NAME}_LIBS})

# Version
build_version_file (lvfs-arc 0.0.1)

# Documentation
add_documentation ("LVFS-Arc" 0.0.1 "LVFS Plugin for reading archive files")

# Install rules
install_header_files (lvfs-arc "src/lvfs_arc_IArchive.h:IArchive")
install_target (lvfs-arc)
