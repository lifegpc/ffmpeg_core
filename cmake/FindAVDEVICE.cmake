cmake_minimum_required(VERSION 3.11)
find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_AVDEVICE QUIET IMPORTED_TARGET GLOBAL libavdevice)
endif()

if (PC_AVDEVICE_FOUND)
    set(AVDEVICE_FOUND TRUE)
    set(AVDEVICE_VERSION ${PC_AVDEVICE_VERSION})
    set(AVDEVICE_VERSION_STRING ${PC_AVDEVICE_STRING})
    set(AVDEVICE_LIBRARYS ${PC_AVDEVICE_LIBRARIES})
    if (USE_STATIC_LIBS)
        set(AVDEVICE_INCLUDE_DIRS ${PC_AVDEVICE_STATIC_INCLUDE_DIRS})
    else()
        set(AVDEVICE_INCLUDE_DIRS ${PC_AVDEVICE_INCLUDE_DIRS})
    endif()
    if (NOT AVDEVICE_INCLUDE_DIRS)
        find_path(AVDEVICE_INCLUDE_DIRS NAMES libavdevice/avdevice.h)
        if (AVDEVICE_INCLUDE_DIRS)
            target_include_directories(PkgConfig::PC_AVDEVICE INTERFACE ${AVDEVICE_INCLUDE_DIRS})
        endif()
    endif()
    if (NOT TARGET AVDEVICE::AVDEVICE)
        add_library(AVDEVICE::AVDEVICE ALIAS PkgConfig::PC_AVDEVICE)
    endif()
else()
    message(FATAL_ERROR "failed.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVDEVICE
    FOUND_VAR AVDEVICE_FOUND
    REQUIRED_VARS
        AVDEVICE_LIBRARYS
        AVDEVICE_INCLUDE_DIRS
    VERSION_VAR AVDEVICE_VERSION
)
