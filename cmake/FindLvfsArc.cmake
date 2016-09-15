# - Find lvfs-arc
# Find the lvfs-arc library
#
#  This module defines the following variables:
#     LVFSARC_FOUND   - True if LVFSARC_INCLUDE is found
#     LVFSARC_INCLUDE - where to find header files
#     LVFSARC_LIB     - the library files


if (NOT LVFSARC_FOUND)
    find_path (LVFSARC_INCLUDE
               NAMES "lvfs-arc/IArchive"
               PATH_SUFFIXES include
               DOC "Lvfs ARC include directory")

    find_library (LVFSARC_LIB
                  NAMES lvfs-arc
                  DOC "Lvfs Arc library file")

    # handle the QUIETLY and REQUIRED arguments and set LVFSARC_FOUND to TRUE if all listed variables are TRUE
    include (FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS (Lvfs DEFAULT_MSG LVFSARC_INCLUDE LVFSARC_LIB)

    mark_as_advanced (LVFSARC_INCLUDE LVFSARC_LIB)
endif ()
