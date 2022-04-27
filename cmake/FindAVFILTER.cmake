cmake_minimum_required(VERSION 3.11)
find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_AVFILTER QUIET IMPORTED_TARGET GLOBAL libavfilter)
endif()

if (PC_AVFILTER_FOUND)
    set(AVFILTER_FOUND TRUE)
    set(AVFILTER_VERSION ${PC_AVFILTER_VERSION})
    set(AVFILTER_VERSION_STRING ${PC_AVFILTER_STRING})
    set(AVFILTER_LIBRARYS ${PC_AVFILTER_LIBRARIES})
    if (USE_STATIC_LIBS)
        set(AVFILTER_INCLUDE_DIRS ${PC_AVFILTER_STATIC_INCLUDE_DIRS})
    else()
        set(AVFILTER_INCLUDE_DIRS ${PC_AVFILTER_INCLUDE_DIRS})
    endif()
    if (NOT AVFILTER_INCLUDE_DIRS)
        find_path(AVFILTER_INCLUDE_DIRS NAMES libavfilter/avfilter.h)
        if (AVFILTER_INCLUDE_DIRS)
            target_include_directories(PkgConfig::PC_AVFILTER INTERFACE ${AVFILTER_INCLUDE_DIRS})
        endif()
    endif()
    if (NOT TARGET AVFILTER::AVFILTER)
        add_library(AVFILTER::AVFILTER ALIAS PkgConfig::PC_AVFILTER)
    endif()
else()
    message(FATAL_ERROR "failed.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVFILTER
    FOUND_VAR AVFILTER_FOUND
    REQUIRED_VARS
        AVFILTER_LIBRARYS
        AVFILTER_INCLUDE_DIRS
    VERSION_VAR AVFILTER_VERSION
)
