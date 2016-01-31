project(roerei)

find_package(Threads)
find_package(Boost COMPONENTS system program_options regex chrono date_time filesystem REQUIRED)

add_executable(roerei main.cpp)
target_link_libraries(roerei
	${Boost_LIBRARIES}
	${msgpack_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT})

add_executable(roerei-legacy legacy.cpp)
target_link_libraries(roerei-legacy
	${Boost_LIBRARIES}
	${msgpack_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT})

list(APPEND Roerei_INCLUDE_DIRS ${msgpack_INCLUDE_DIRS})

include_directories(SYSTEM ${Roerei_INCLUDE_DIRS})
set(Roerei_INCLUDE_DIRS ${Roerei_INCLUDE_DIRS} PARENT_SCOPE)
