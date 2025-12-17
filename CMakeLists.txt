cmake_minimum_required(VERSION 3.20)
project(mirror_hash VERSION 0.1.0 LANGUAGES CXX)

# Use C++2c (C++26) with reflection - set flags directly since CMake doesn't know CXX26 yet
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2c -freflection -freflection-latest -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Optimization flags - aggressive optimization for benchmarks
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Header-only library
add_library(mirror_hash INTERFACE)
target_include_directories(mirror_hash INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_mirror_hash tests/test_mirror_hash.cpp)
    target_link_libraries(test_mirror_hash PRIVATE mirror_hash)
    add_test(NAME test_mirror_hash COMMAND test_mirror_hash)
endif()

# Benchmarks
option(BUILD_BENCHMARKS "Build benchmarks" ON)
if(BUILD_BENCHMARKS)
    # Official comparison against wyhash, rapidhash, xxHash
    add_executable(official_comparison benchmarks/official_comparison.cpp)
    target_link_libraries(official_comparison PRIVATE mirror_hash)
    target_include_directories(official_comparison PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

    # Policy comparison between different hash algorithms
    add_executable(policy_comparison benchmarks/policy_comparison.cpp)
    target_link_libraries(policy_comparison PRIVATE mirror_hash)

    # Quality verification tests
    add_executable(quality_verification benchmarks/quality_verification.cpp)
    target_link_libraries(quality_verification PRIVATE mirror_hash)

endif()

# Quality Analysis
option(BUILD_ANALYSIS "Build hash quality analysis" ON)
if(BUILD_ANALYSIS)
    add_executable(hash_quality_analysis analysis/run_analysis.cpp)
    target_include_directories(hash_quality_analysis PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis
    )
endif()

# Examples
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_executable(example_basic examples/basic_usage.cpp)
    target_link_libraries(example_basic PRIVATE mirror_hash)

    add_executable(example_custom_algo examples/custom_algorithm.cpp)
    target_link_libraries(example_custom_algo PRIVATE mirror_hash)

    add_executable(example_hashtable examples/hashtable_integration.cpp)
    target_link_libraries(example_hashtable PRIVATE mirror_hash)
endif()

# Installation
install(DIRECTORY include/ DESTINATION include)
install(TARGETS mirror_hash EXPORT mirror_hash-targets)
install(EXPORT mirror_hash-targets
    FILE mirror_hash-config.cmake
    NAMESPACE mirror_hash::
    DESTINATION lib/cmake/mirror_hash
)
