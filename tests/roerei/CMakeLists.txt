enable_testing()

find_package(check REQUIRED)

add_executable(roerei-test roerei-test.cpp)
target_link_libraries(roerei-test
	${Roerei_LIBRARIES}
	${check_LIBRARIES})

include_directories(SYSTEM ${Roerei_INCLUDE_DIRS} ${check_INCLUDE_DIRS})
