  
# LibGaussianBlur

![coverage](https://raw.githubusercontent.com/michelerenzullo/LibGaussianBlur/refs/heads/main/coverage.svg)

LibGaussianBlur is a C++ library designed to apply Gaussian blur to images using the Fast Fourier Transform (FFT). It features optimizations for parallel tile processing and leverages the property that a centered kernel has an imaginary part of zero. The library supports multiple platforms, including Android, iOS, WebAssembly (Emscripten), macOS, and Linux, and provides examples for both desktop and web environments. This project is a refined version of [Blur_algorithms](https://github.com/michelerenzullo/Blur_algorithms).

The primary objective of LibGaussianBlur is to offer a portable, lightweight, and efficient solution for applying true Gaussian blur to large images. It is intended for academic purposes and can be extended for video processing, as the kernel data can be stored and applied to frames.

## Key Concepts

### Decomposable Gaussian Kernel
- A 2D Gaussian kernel can be decomposed into two 1D kernels (horizontal and vertical). This means applying a 2D convolution is equivalent to applying two 1D convolutions:
	1. One across the rows (horizontal processing).
	2. One across the columns (vertical processing).
- Advantages:
	- Significant reduction in computational complexity, transforming ￼operations per pixel to operations.
	- Perfect for tile-based image processing, where large images are divided into smaller tiles for efficient memory management and parallelization.
	- Supports processing in a streaming manner, ideal for devices with limited memory or for real-time applications.

### Centered Kernel
- The Gaussian kernel generated by the library is centered and shifted to ensure:
  - The kernel is symmetric around its center.
  -	Its **imaginary part is zero** in the frequency domain.
- Why Shift the Kernel?
	-	In FFT-based convolution, the operation is naturally cyclic, leading to circular convolution artifacts at the edges of the image.
	-	Shifting the Gaussian kernel aligns it to the center of the image or tile, effectively avoiding these artifacts.
	-	This ensures a correct spatial-domain convolution that is indistinguishable from direct convolution, preserving the intended blurring effect.
- Why This Matters:
  - The kernel’s shift ensures smooth edges and prevents unintended overlaps caused by cyclic behavior.
  - The zero imaginary part in the frequency domain reduces computational overhead during FFT-based convolutions:
    - Only the **real components** are used, leading to faster vector multiplications.
  - Enhanced numerical stability during computation.

#### Example of Padding and Centering

##### 1D Padding and Centering
```
Box car kernel 3
1/3 1/3 1/3

FFT Length (for tile) = 8
Extra padd = 5
1/3 1/3 1/3 0 0 0 0 0

Shifting and centering
1/3 1/3 0 0 0 0 0 1/3
The above can be achieved easily with std::rotate
```

##### 2D Padding and Centering
```
Box car kernel 3x3
1/9 1/9 1/9
1/9 1/9 1/9
1/9 1/9 1/9

FFT Length 64
Extra padd = 55
1/9 1/9 1/9 0 0 0 0 0
1/9 1/9 1/9 0 0 0 0 0
1/9 1/9 1/9 0 0 0 0 0
0    0   0  0 0 0 0 0
0    0   0  0 0 0 0 0
0    0   0  0 0 0 0 0
0    0   0  0 0 0 0 0
0    0   0  0 0 0 0 0

Shifting and centering
1/9 1/9 0 0 0 0 0 1/9
1/9 1/9 0 0 0 0 0 1/9
0    0  0 0 0 0 0  0
0    0  0 0 0 0 0  0
0    0  0 0 0 0 0  0
0    0  0 0 0 0 0  0
0    0  0 0 0 0 0  0
1/9 1/9 0 0 0 0 0 1/9
```

### Image Border Handling
- The image border is handled using reflect_101 padding:
	- Reflects the image at its edges, avoiding boundary artifacts caused by abrupt changes.
	- For example, the pixel sequence [a, b, c] would be padded as [b, a, b, c, b].
- Additionally, 0-padding is applied as needed to ensure the image tile dimensions match the length of the FFT.
	- The FFT size is determined using the nearestTransformSize, chosen to be decomposable into primes for computational efficiency.

#### Advantages of Border Handling
1.	Edge Artifacts Eliminated: The reflect_101 padding ensures smooth transitions at the edges of the image.
2.	FFT Compatibility: Padding ensures that the image size is compatible with the FFT, optimizing performance for frequency-domain operations.
3.	Seamless Tiling: Reflective and zero padding ensure adjacent tiles blend seamlessly, making this approach suitable for large-scale and real-time image processing.

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

### Download already built library

The current CI/CD pipeline automatically uploads the artifacts upon completion. You can download these artifacts from the "Actions" page by navigating to the latest workflow run and scrolling to the bottom of the page. 

See below in the [Artifact Management](#artifact-management) section for more details.

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
cmake -DENABLE_MULTITHREADING=ON -DWITH_EXAMPLES=ON -DWITH_TESTS=ON ..
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
./GaussianBlur <smoothing_factor> <input_file> [apply_to_alpha]
```
-  <smoothing_factor>: The smoothing factor for the Gaussian blur (must be > 0).
-  <input_file>: The input image file.

- [apply_to_alpha]: Optional. If set to 1, the convolution is done on the 4th channel (alpha channel). If not provided or set to 0, the convolution is done on the first 3 channels only.

If compiled with `WITH_TESTS=ON` (GoogleTest), you can run the tests using:
```sh
./GaussianBlurTests
```

### WebAssembly

The WebAssembly version is compiled by default with multi-threading using web workers and SIMD instructions (required by the pffft library). A script is provided to run a local web server with lighttpd, which also sets up the necessary CORS and COEP headers (required when using SharedArrayBuffers):

```sh
cd examples/wasm
./scripts/local_deploy.sh
```
Then navigate to `http://localhost:8080/gaussianblur.html`

#### Note for WebAssembly on macOS:

If executing the bootstrap from a macOS machine, ensure GNU Grep is available, as the script uses a regex command (grep -oP) unavailable on macOS. This command sets the “closure compiled” variable name of `Module`, which is important for the worker to call the entry point correctly:

```sh
brew install ggrep
```

## CI/CD Integration
LibGaussianBlur implements a robust Continuous Integration and Continuous Deployment (CI/CD) pipeline using GitHub Actions to ensure reliable and efficient development workflows. The pipeline is designed with the following features:

### Multi-Platform Build
- The CI/CD pipeline supports automated builds for multiple platforms:
- Linux
- Android
- WebAssembly (WASM)

### Artifact Management
- Artifacts generated during builds are compressed and stored:
  - Each workflow run produces downloadable build artifacts like coverage report, binaries, shared / static library based on the environment.
  - Artifacts are retained for up to 90 days, available under the “Artifacts” section on the bottom page of the corresponding GitHub workflow run.


### Caching Strategies
- The pipeline uses advanced caching strategies to speed up build times:
  - Docker base dependencies are cached across workflow runs and stored in GitHub.
  - Cache mechanisms prevent redundant builds by restoring previously computed layers as long as the SHA of relevant files for the layer is unchanged.
  - A mechanism is in place to manage GitHub’s cache storage limits, releasing cache of older runs, not useful anymore.

### Testing
- The pipeline is configured to:
  - Build and run the test suite on the Linux target.
  - A coverage report is generated after running tests, the .html report is available in the artifact zip
  - Tests must pass successfully before generating the coverage report and build the Android and WASM libraries.

## Roadmap
- Code quality and Doxygen
- Flutter plugin with native bindings

## Contributing
Contributions are welcome! Please open an issue or submit a pull request on GitHub.


## License
This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgements

- [pffft by Julien Pommier](https://bitbucket.org/jpommier/pffft/src/master) for FFT processing and such lightweight and performant FFT library

- [stb_image](https://github.com/nothings/stb) for image loading and writing in the examples