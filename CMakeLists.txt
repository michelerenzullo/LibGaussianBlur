# Minimum required version of CMake
cmake_minimum_required(VERSION 3.14)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Multithreading
option(ENABLE_MULTITHREADING "Enable multithreading support" OFF)
# WASM
option(WASM "Compile to WebAssembly" OFF)
# Examples
option(WITH_EXAMPLES "Include examples" OFF)
# Tests
option(WITH_TESTS "Include tests" OFF)
# Coverage
option(WITH_COVERAGE "Include coverage" OFF)

if(WITH_COVERAGE AND WITH_EXAMPLES)
    message(FATAL_ERROR "Coverage can affect the performance of the examples. Pick one or the other.")
endif()

# Ensure WITH_TESTS is enabled if WITH_COVERAGE is enabled
if(WITH_COVERAGE)
    message(STATUS "Including coverage")
    set(WITH_TESTS ON CACHE BOOL "Include tests" FORCE)
endif()

# If compiling to WebAssembly
if(WASM)
    message(STATUS "Compiling to WebAssembly")
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
    endif()
    set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
endif()

# Set common compiler flags
if(WITH_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3 -coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -coverage -g3 -O0 -fprofile-arcs -ftest-coverage")
    add_compile_definitions(WITH_COVERAGE)
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3")
endif()

# Append additional flags for WebAssembly
if(WASM)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msimd128")
endif()

if(ENABLE_MULTITHREADING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_MULTITHREADING -pthread")
endif()

# Add Framework Accelerate for macOS (SIMD)
if(APPLE AND NOT WASM)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Accelerate")
endif()

# Project name
project(GaussianBlur)
include_directories(include)

# Add library
if(WASM)
    add_library(GaussianBlurLib STATIC "include/pffft_pommier/pffft.c" "src/gaussianblur.cpp")
else()
    add_library(GaussianBlurLib "include/pffft_pommier/pffft.c" "src/gaussianblur.cpp")
endif()

# Set properties for the library
set_target_properties(GaussianBlurLib PROPERTIES OUTPUT_NAME "Gaussianblur")

if(WITH_EXAMPLES)
    message(STATUS "Building examples")
    if(WASM)
        add_executable(GaussianBlur "examples/wasm/main.cpp")
        set(WASM_COMMON_LINK_OPTIONS 
            "SHELL: --closure 1"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sMAXIMUM_MEMORY=4GB"
            "-sINITIAL_MEMORY=314572800"
            "-O3"
            "--bind"
            "-sWASM_BIGINT"
            "-sEXPORTED_FUNCTIONS=['_malloc']"
            "-msimd128"
        )

        # Append specific options based on SINGLE or ENABLE_MULTITHREADING
        if(ENABLE_MULTITHREADING)
            list(APPEND WASM_COMMON_LINK_OPTIONS 
                "-pthread"
                "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency"
            )
        endif()

        # Apply the options to the target
        target_link_options(GaussianBlur PRIVATE ${WASM_COMMON_LINK_OPTIONS})

        # Install the WASM file
        install(FILES "${CMAKE_BINARY_DIR}/GaussianBlur.wasm" DESTINATION bin)
    else()
        add_executable(GaussianBlur "examples/desktop/main.cpp")
    endif()

    target_link_libraries(GaussianBlur GaussianBlurLib)
    install(TARGETS GaussianBlur
        BUNDLE DESTINATION Applications
        RUNTIME DESTINATION bin
    )
endif()

install(TARGETS GaussianBlurLib ARCHIVE DESTINATION lib)
install(DIRECTORY include/gaussianblur/ DESTINATION include/gaussianblur FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

# Add Google Test
if(WITH_TESTS)
    message(STATUS "Building google tests")
    enable_testing()
    set(BUILD_GMOCK OFF)

    # Temporarily set BUILD_SHARED_LIBS to OFF for Google Test
    set(BUILD_SHARED_LIBS_SAVE ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(external/googletest)
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVE})

    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    # Add test executable
    add_executable(GaussianBlurTests tests/test_gaussianblur.cpp)
    target_link_libraries(GaussianBlurTests GaussianBlurLib gtest_main)
    add_test(NAME GaussianBlurTests COMMAND GaussianBlurTests)
    install(TARGETS GaussianBlurTests DESTINATION bin)
endif()