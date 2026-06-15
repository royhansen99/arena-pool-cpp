cmake_minimum_required(VERSION 3.10)

project(ArenaPoolCpp VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(TestsArena tests/tests_arena.cpp)
add_executable(TestsPool tests/tests_pool.cpp)
add_executable(TestsVector tests/tests_vector.cpp)
add_executable(TestsStringArena tests/tests_string_arena.cpp)
add_executable(TestsStringDynamic tests/tests_string_dynamic.cpp)
add_executable(TestsStringStatic tests/tests_string_static.cpp)
add_executable(ArenaExample tests/arena_example.cpp)
add_executable(Benchmarks tests/benchmarks_alloc.cpp)
add_executable(StringExample tests/string_example.cpp)
