project(roerei)

find_package(Threads)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS system program_options regex chrono date_time filesystem REQUIRED)

add_executable(roerei main.cpp cli_read.cpp cli_exec.cpp storage.cpp exporter.cpp structure_exporter.cpp)
target_link_libraries(roerei
	${Boost_LIBRARIES}
	${msgpack_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT})

list(APPEND Roerei_INCLUDE_DIRS ${msgpack_INCLUDE_DIRS})

include_directories("${CMAKE_CURRENT_LIST_DIR}/..")
include_directories(SYSTEM ${Roerei_INCLUDE_DIRS})
set(Roerei_INCLUDE_DIRS ${Roerei_INCLUDE_DIRS} PARENT_SCOPE)
