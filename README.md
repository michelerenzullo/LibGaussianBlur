  
# LibGaussianBlur


LibGaussianBlur is a C++ library designed to apply Gaussian blur to images using the Fast Fourier Transform (FFT). It features optimizations for parallel tile processing and leverages the property that a centered kernel has an imaginary part of zero. The library supports multiple platforms, including Android, iOS, WebAssembly (Emscripten), macOS, and Linux, and provides examples for both desktop and web environments. This project is a refined version of [Blur_algorithms](https://github.com/michelerenzullo/Blur_algorithms).

The primary objective of LibGaussianBlur is to offer a portable, lightweight, and efficient solution for applying true Gaussian blur to large images. It is intended for academic purposes and can be extended for video processing, as the kernel data can be stored and applied to frames.

## Features
  
- Centered Gaussian Kernel Generation: Calculates a centered Gaussian kernel from a given sigma, avoiding circular convolution. The radius is determined by the formula:  
 $$radius = {σ\sqrt{2log(255)} -1} $$  
 $$width = {round(2 radius)} $$  

- FFT of Kernel and Image Tiles: Performs FFT on both kernel and image tiles (rows and columns).
- Frequency Domain Convolution: Applies convolution in the frequency domain using the real part of the kernel.
- Multi-threaded Processing and SIMD Support: Optimizes performance through multi-threading and Single Instruction, Multiple Data (SIMD) instructions.
- WebAssembly (WASM) Support: Enables web-based applications.
- Cross-Platform Compatibility: Supports Android, iOS, macOS, Linux, and soon Flutter.
- Examples Provided: Includes examples for both desktop and web environments.
## Requirements
- CMake 3.14 or higher
- A C++ compiler that supports C++20
- Emscripten (for WebAssembly support)
- Android NDK (for Android support)
- Xcode (for iOS and macOS support)


## Installation

### Building the Library

#### Using the Bootstrap Script

It is recommended to use the bootstrap shell script

```sh
./bootstrap/bootstrap.sh "PLATFORM"
```

This script performs additional configuration and builds all the files ready to be shipped under the ```external/``` folder. The ```PLATFORM``` argument can be one of the following:
- android
- linux
- macos_arm
- macos_x86
- wasm
- iOS
- all_darwin
- all_linux

##### Example:
```sh
./bootstrap/bootstrap.sh macos_arm
```

This will generate the necessary files in the ```external/macos``` folder.

#### iOS Specific Instructions

Since this build generate an iOS framework, it needs to be signed to be implemented in your app, therefore you need to set the `DEV_TEAM` environment variable to sign the iOS framework before running the bootstrap script.

```sh
export DEV_TEAM="your_dev_team_id"
./bootstrap/bootstrap.sh ios
```
This will generate the necessary files in the `external/ios` folder.

#### Manual Build

##### Desktop (Linux, macOS)

```sh
mkdir build
cd build
cmake -DENABLE_MULTITHREADING=ON -DWITH_EXAMPLES=ON ..
make
```

##### WebAssembly

Please ensure you have Emscripten installed. You might need to define the TOOLCHAIN_FILE of Emscripten.

```sh
mkdir build
cd build
cmake -DWASM=ON -DENABLE_MULTITHREADING=ON  -DWITH_EXAMPLES=ON -DCMAKE_TOOLCHAIN_FILE="$HOME/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" ..
make
```

 
## Usage

  When building with `WITH_EXAMPLES=ON`, you can try the library. These examples are intended to provide a simple idea of how the library is used.

### Command Line (desktop)

```sh
./GaussianBlur <smoothing_factor> <input_file> [alpha]
```
-  <smoothing_factor>: The smoothing factor for the Gaussian blur (must be > 0).
-  <input_file>: The input image file.

- [alpha]: Optional. If set to 1, the convolution is done on the 4th channel (alpha channel). If not provided or set to 0, the convolution is done on the first 3 channels only.

### WebAssembly

The WebAssembly version is compiled by default with multi-threading using web workers and SIMD instructions (required by the pffft library). A script is provided to run a local web server with lighttpd, which also sets up the necessary CORS and COEP headers (required when using SharedArrayBuffers):

```sh
cd examples/wasm
./scripts/local_deploy.sh
```
Then navigate to `http://localhost:8080/gaussianblur.html`

#### Note for macOS Users:

If executing the bootstrap from a macOS machine, ensure GNU Grep is available, as the script uses a regex command (grep -oP) unavailable on macOS. This command sets the “closure compiled” variable name of `Module`, which is important for the worker to call the entry point correctly:

```sh
brew install ggrep
```

  

### Contributing
Contributions are welcome! Please open an issue or submit a pull request on GitHub.


### License
This project is licensed under the MIT License. See the LICENSE file for details.

### Acknowledgements

- [stb_image](https://github.com/nothings/stb) for image loading and writing in the examples

- [pffft by Julien Pommier](https://bitbucket.org/jpommier/pffft/src/master) for FFT processing and such lightweight and performant FFT library