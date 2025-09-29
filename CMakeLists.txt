cmake_minimum_required(VERSION 3.27)
project(RingBuffer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

# Header-only library target to propagate include paths & warnings
add_library(ringbuffer INTERFACE)
target_include_directories(ringbuffer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(example src/main.cpp)
target_link_libraries(example PRIVATE ringbuffer)

add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE ringbuffer)

add_executable(bench bench/bench.cpp)
target_link_libraries(bench PRIVATE ringbuffer)
