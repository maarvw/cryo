cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(cryo VERSION 0.0.1)

include(CMakePackageConfigHelpers)
include(CheckIncludeFileCXX)
include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library(cryo INTERFACE)

target_sources(cryo
    INTERFACE
        FILE_SET headers
        BASE_DIRS include
        TYPE HEADERS
        FILES
            include/cryo/set.h
)

target_include_directories(cryo INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(cryo INTERFACE cxx_std_20)

if(MSVC)
    target_compile_definitions(cryo INTERFACE _CTYPE_DISABLE_MACROS)
    target_compile_options(cryo INTERFACE /utf-8 /wd4146 /wd4245)
endif()

set(targets_export_name "cryo-targets")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cryo-config-version.cmake"
    VERSION ${cryo_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    cmake/cryo-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cryo-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cryo"
)
install(
    TARGETS cryo
    EXPORT ${targets_export_name}
    FILE_SET headers
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/cryo-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/cryo-config-version.cmake"
    DESTINATION
        "${CMAKE_INSTALL_LIBDIR}/cmake/cryo"
)
install(
    EXPORT ${targets_export_name}
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cryo"
    NAMESPACE cryo::
)
install(
    DIRECTORY include/cryo
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

add_subdirectory(external/fe)
