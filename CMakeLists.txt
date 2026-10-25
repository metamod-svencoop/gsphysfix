cmake_minimum_required(VERSION 3.21)
option(FORMAT_VALIDATION_ONLY "Configure formatting without native dependencies" OFF)
if(FORMAT_VALIDATION_ONLY)
    project(GSPhysFixFormat LANGUAGES NONE)
    include(cmake/FormatValidation.cmake)
    return()
endif()
include(cmake/Metamod.cmake)
project(gsphysfix LANGUAGES C CXX)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration" FORCE)
endif()
include("${METAMOD_SOURCE_PATH}/cmake/SDK.cmake")
mmfg_configure_install_layout()
add_library(gsphysfix SHARED
    src/dllapi.cpp
    src/engine_hook.cpp
    src/gsphysfix.cpp
    src/h_export.cpp
    src/meta_api.cpp
    src/physfix.cpp
)
mmfg_configure_target(gsphysfix)
target_link_libraries(gsphysfix PRIVATE Metamod::SDK)
if(WIN32)
    target_link_options(gsphysfix PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/gsphysfix.def")
else()
    target_compile_options(gsphysfix PRIVATE -mincoming-stack-boundary=2)
endif()
# The existing validator requires every declared platform in every selected
# snapshot, and the hook point differs by platform: split the manifests.
add_custom_target(gsphysfix_windows)
mmfg_add_gamedata(gsphysfix_windows scripts/manifests/gsphysfix_windows.json)
add_custom_target(gsphysfix_linux)
mmfg_add_gamedata(gsphysfix_linux scripts/manifests/gsphysfix_linux.json)
add_dependencies(gsphysfix gsphysfix_windows gsphysfix_linux)
mmfg_assign_source_groups(gsphysfix)
option(MMFG_GSPHYSFIX_BUILD_TESTS "Build GSPhysFix behavior tests" ON)
if(MMFG_GSPHYSFIX_BUILD_TESTS)
    enable_testing()
    add_executable(mmfg_test_gsphysfix tests/test_gsphysfix.cpp src/physfix.cpp)
    target_include_directories(mmfg_test_gsphysfix PRIVATE src)
    target_link_libraries(mmfg_test_gsphysfix PRIVATE Metamod::SDK)
    target_compile_features(mmfg_test_gsphysfix PRIVATE cxx_std_17)
    add_test(NAME mmfg_test_gsphysfix COMMAND mmfg_test_gsphysfix)
endif()
include(cmake/FormatValidation.cmake)
