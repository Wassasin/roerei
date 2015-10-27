cmake_minimum_required(VERSION 2.6)

set(MSGPACK_BUILD_TESTS OFF CACHE BOOL "Build msgpack tests")
set(MSGPACK_BUILD_EXAMPLES OFF CACHE BOOL "Build msgpack examples")

add_subdirectory(contrib/msgpack-c)

project(roerei)
list(APPEND msgpack_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/contrib/msgpack-c/include")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/contrib/wubwubcmake/modules")

include("contrib/wubwubcmake/enable_cpp11.cmake")
include("contrib/wubwubcmake/warning_settings.cmake")
add_sane_warning_flags()

list(APPEND Roerei_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/src/")

enable_testing()
add_subdirectory("src/roerei")
add_subdirectory("tests/roerei")
