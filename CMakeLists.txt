project (lvfs-arc)

# Project header
project_header_default ("POSITION_INDEPENDENT_CODE:YES")

# 3rdparty
list (APPEND ${PROJECT_NAME}_LIBS ${EFC_LIB})
list (APPEND ${PROJECT_NAME}_LIBS ${LVFS_LIB})

find_package (LibArchive REQUIRED)
include_directories (${LIBARCHIVE_INCLUDE_DIR})
list (APPEND ${PROJECT_NAME}_LIBS ${LIBARCHIVE_LIBRARY})

find_package (LibUnrar REQUIRED)
include_directories (${LIBUNRAR_INCLUDE})
list (APPEND ${PROJECT_NAME}_LIBS ${LIBUNRAR_LIBRARY})

# Sources
add_subdirectory (src)

# Target - lvfs-arc
add_library (lvfs-arc SHARED ${${PROJECT_NAME}_SOURCES})
target_link_libraries (lvfs-arc ${${PROJECT_NAME}_LIBS})

# Documentation
add_documentation (lvfs-arc 0.0.1 "LVFS Plugin for reading archive files")

# Install rules
install_header_files (lvfs-arc "src/lvfs_arc_IArchive.h:IArchive")
install_cmake_files ("cmake/FindLvfsArc.cmake")
install_target (lvfs-arc)
