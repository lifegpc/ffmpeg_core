cmake_minimum_required(VERSION 3.11)
find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_AVUTIL QUIET IMPORTED_TARGET GLOBAL libavutil)
endif()

if (PC_AVUTIL_FOUND)
    set(AVUTIL_FOUND TRUE)
    set(AVUTIL_VERSION ${PC_AVUTIL_VERSION})
    set(AVUTIL_VERSION_STRING ${PC_AVUTIL_STRING})
    set(AVUTIL_LIBRARYS ${PC_AVUTIL_LIBRARIES})
    if (USE_STATIC_LIBS)
        set(AVUTIL_INCLUDE_DIRS ${PC_AVUTIL_STATIC_INCLUDE_DIRS})
    else()
        set(AVUTIL_INCLUDE_DIRS ${PC_AVUTIL_INCLUDE_DIRS})
    endif()
    if (NOT AVUTIL_INCLUDE_DIRS)
        find_path(AVUTIL_INCLUDE_DIRS NAMES libavutil/avutil.h)
        if (AVUTIL_INCLUDE_DIRS)
            target_include_directories(PkgConfig::PC_AVUTIL INTERFACE ${AVUTIL_INCLUDE_DIRS})
        endif()
    endif()
    if (NOT TARGET AVUTIL::AVUTIL)
        add_library(AVUTIL::AVUTIL ALIAS PkgConfig::PC_AVUTIL)
    endif()
else()
    message(FATAL_ERROR "failed.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVUTIL
    FOUND_VAR AVUTIL_FOUND
    REQUIRED_VARS
        AVUTIL_LIBRARYS
        AVUTIL_INCLUDE_DIRS
    VERSION_VAR AVUTIL_VERSION
)
