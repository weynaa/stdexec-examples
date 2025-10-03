cmake_minimum_required(VERSION 3.27)

project(stdexec_examples LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
	stdexec
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/stdexec
	SYSTEM
)
FetchContent_MakeAvailable(stdexec)

FetchContent_Declare(
	GTest
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/googleTest
	SYSTEM
)
FetchContent_MakeAvailable(GTest)

function(add_example file)
	get_filename_component(basename ${file} NAME_WLE)
	add_executable(${basename} ${file})
	target_link_libraries(${basename} PRIVATE STDEXEC::stdexec)
endfunction()

add_example(examples/example_1_continue_on.cpp)
add_example(examples/example_2_basic_coroutine.cpp)
add_example(examples/example_3_async_scope.cpp)
add_example(examples/example_4_capturing_lambdas.cpp)
add_example(examples/example_5_continuation_scheduler.cpp)
add_example(examples/example_6_lock.cpp)
add_example(examples/example_7_accidental_async.cpp)
add_example(examples/example_8_synchronization_cycle.cpp)


add_executable(hardware_manager exercise/hardware_manager.cpp)
target_link_libraries(hardware_manager STDEXEC::stdexec GTest::gtest)
