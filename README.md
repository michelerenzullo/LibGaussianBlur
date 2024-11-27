
# GaussianBlur

  DOCUMENTATION AND README IN PROGRESS
GaussianBlur is a C++ library for applying Gaussian blur to images using Fast Fourier Transform plus many optimizations to parallelize tiles processing of the image. This project includes support for multiple platforms including Android, iOS, WebAssembly (Emscripten), macOS, and Linux. It also provides examples for both desktop and WebAssembly. This library is a cleanup version of "Blur_algorithms", another repository of mine.

The main goal of this repository is to create a portable, light and fast library to apply true gaussian blur without fear of large images.
  

## Features

  

- Apply Gaussian blur to images

- Support for multi-threaded processing and SIMD

- WebAssembly (WASM) support

- Examples for desktop and web environments

- Cross-platform support: Android, iOS, macOS, Linux, soon Flutter WebAssembly

  

## Requirements

  

- CMake 3.14 or higher

- A C++ compiler that supports C++20

- Emscripten (for WebAssembly support)

- Android NDK (for Android support)

- Xcode (for iOS and macOS support)

  

## Installation

  


### Building the Library

#### RECOMMENDED: Using the Bootstrap Script 

It is recommended to use the bootstrap shell script 

    ./bootstrap/bootstrap.sh "PLATFORM" 

that performs some additional configuration and build all the files ready to be shipped under the external/ folder. The PLATFORM argument can be one of the following:

  

    android
    
    linux
    
    macos_arm
    
    macos_x86
    
    wasm
    
    ios
    
    all_darwin
    
    all_linux

  

#### iOS Specific Instructions

Since this build generate an iOS framework, it needs to be signed to be implemented in your app, therefore you need to set the DEV_TEAM environment variable to sign the iOS framework before running the bootstrap script.

  

```sh

export  DEV_TEAM="your_dev_team_id"
./bootstrap/bootstrap.sh ios

```

  

#### Manual Build

##### Desktop (Linux, macOS)

  

```sh

mkdir  build

cd  build

cmake  -DENABLE_MULTITHREADING=ON -DWITH_EXAMPLES=ON ..

make

```

  

##### WebAssembly
Please ensure to have emscripten installed and available, you might need to define the TOOLCHAIN_FILE of Emscripten.

```sh

mkdir  build

cd  build

cmake  -DWASM=ON  -DENABLE_MULTITHREADING=ON  -DWITH_EXAMPLES=ON  -DCMAKE_TOOLCHAIN_FILE="$HOME/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"  ..

make

```

  

## Usage

### Command Line

  

```sh

./GaussianBlur <smoothing_factor> <input_file> [alpha]

```

-  <smoothing_factor>: The smoothing factor for the Gaussian blur (must be > 0).

-  <input_file>: The input image file.

- [alpha]: Optional. If set to 1, the convolution is done on the 4th channel (alpha channel). If not provided or set to 0, the convolution is done on the first 3 channels only.

  

### WebAssembly

This version is compiled by defualt with multi-threading using web workers and also SIMD instructions (needed by pffft library).

An handy script that run a local webserver with lighttpd is provided, which also set-up the CORS and COEP headers.

```sh

cd  examples/wasm

./scripts/local_deploy.sh

```

Then simply navigate to `http://localhost:8080/gaussianblur.html`

Please note that if you are executing the bootstrap from a macOS machine, you must have GNU Grep available because there is a REGEX command in the shell script and ```grep -oP``` is unavailable on macOS. This command set the "closure compiled" variable name of "Module", and it is important to set correctly or the worker wouldn't be able to call the entry point
```sh
brew install ggrep
```

  

### Contributing

Contributions are welcome! Please open an issue or submit a pull request on GitHub.

  

### License

This project is licensed under the MIT License. See the LICENSE file for details.

  

### Acknowledgements

- stb_image for image loading and writing in the examples

- pffft by Julien Pommier for FFT processing and such lightweight and performant FFT library
