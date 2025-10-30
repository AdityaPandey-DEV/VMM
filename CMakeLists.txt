# CMakeLists.txt - Alternative build system for VMM
# For users who prefer CMake over Make

cmake_minimum_required(VERSION 3.10)
project(VMM C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# Source files
set(VMM_SOURCES
    src/main.c
    src/vmm.c
    src/pagetable.c
    src/frame.c
    src/tlb.c
    src/swap.c
    src/replacement.c
    src/trace.c
    src/metrics.c
    src/util.c
)

set(TRACE_GEN_SOURCES
    src/trace_gen.c
    src/trace.c
    src/util.c
)

# Executable targets
add_executable(vmm ${VMM_SOURCES})
add_executable(trace_gen ${TRACE_GEN_SOURCES})

# Link math library
target_link_libraries(vmm m)
target_link_libraries(trace_gen m)

# Installation
install(TARGETS vmm trace_gen DESTINATION bin)

# Tests
enable_testing()
add_test(NAME run_tests COMMAND ${CMAKE_SOURCE_DIR}/tests/run_tests.sh)

# Custom targets
add_custom_target(traces
    COMMAND ${CMAKE_BINARY_DIR}/trace_gen -t sequential -n 1000 -o ${CMAKE_SOURCE_DIR}/traces/sequential.trace
    COMMAND ${CMAKE_BINARY_DIR}/trace_gen -t random -n 5000 -o ${CMAKE_SOURCE_DIR}/traces/random.trace
    COMMAND ${CMAKE_BINARY_DIR}/trace_gen -t working_set -n 10000 -o ${CMAKE_SOURCE_DIR}/traces/working_set.trace
    COMMAND ${CMAKE_BINARY_DIR}/trace_gen -t locality -n 8000 -o ${CMAKE_SOURCE_DIR}/traces/locality.trace
    COMMAND ${CMAKE_BINARY_DIR}/trace_gen -t thrashing -n 15000 -o ${CMAKE_SOURCE_DIR}/traces/thrashing.trace
    DEPENDS trace_gen
)

