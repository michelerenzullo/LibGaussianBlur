# Minimum required version of CMake
cmake_minimum_required(VERSION 3.14)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

set(ENABLE_MULTITHREADING OFF CACHE BOOL "Multi-threaded")

# WASM
option(WASM "Compile to WebAssembly" OFF)
# Examples
option(WITH_EXAMPLES "Include examples" ON)

# If compiling to WebAssembly
if(WASM)
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
    endif()
    set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
endif()

# Set common compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Append additional flags for WebAssembly
if(WASM)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msimd128")
endif()

if(ENABLE_MULTITHREADING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_MULTITHREADING -pthread")
endif()

if(APPLE AND NOT WASM)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -framework Accelerate")
else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3")
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