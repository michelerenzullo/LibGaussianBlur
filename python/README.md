# GaussianBlur

![Continuous Integration](https://github.com/michelerenzullo/LibGaussianBlur/actions/workflows/ci.yml/badge.svg?branch=main) 
![coverage](https://raw.githubusercontent.com/michelerenzullo/LibGaussianBlur/refs/heads/main/coverage.svg)
![license](https://img.shields.io/github/license/michelerenzullo/libgaussianblur)  

GaussianBlur provide a high-performance interface to the underlying LibGaussianBlur C++ library, enabling efficient Gaussian blur image processing through a simple Python API.

## Overview

LibGaussianBlur is a C++ library designed to apply Gaussian blur to images using the Fast Fourier Transform (FFT) multi-threaded for tiles. The Python bindings expose this powerful functionality in a user-friendly way, integrating with popular Python imaging libraries such as Pillow and numpy.

Key features include:

- **FFT-based Gaussian Blur:** Apply precise Gaussian blur using FFT techniques.
- **Optimized Performance:** Leverages parallel tile processing and multi-threading.
- **Cross-Platform Support:** Efficiently processes images on multiple operating systems.
- **Flexible API:** Easily integrate with existing Python image processing workflows.

## Example Requirements

*Note: The following packages are only required for running the usage example and are not mandatory dependencies of the module itself.*

- Python >= 3.11
- [Pillow](https://pillow.readthedocs.io/) – for image loading and saving
- [numpy](https://numpy.org/) – for numerical operations

## Installation

Install the module from PyPI:

```bash
pip install gaussianblur
```

Alternatively, you can build it from source if needed. Make sure the underlying C++ library and its dependencies are correctly configured.

## Usage Example

Below is a simple example demonstrating how to use the `gaussianblur` module:

```python
from PIL import Image as PILImage
import numpy as np
import gaussianblur

# This is a simple example of how to use the gaussianblur module in Python.
def main():
    input_file = "input.png"
    output_file = "output.png"
    
    # Load image using Pillow
    pil_img = PILImage.open(input_file)
    
    # Create an instance of the gaussianblur.Image object
    img = gaussianblur.Image()
    img.geom.rows = pil_img.size[1]
    img.geom.cols = pil_img.size[0]
    img.geom.channels = len(pil_img.getbands())
    img.data = np.array(pil_img).flatten().tolist()
    
    # Set the sigma value for the Gaussian blur
    sigma = 7.5
    
    # If True, the alpha channel will be blurred as well (if present)
    apply_to_alpha_channel = True
    
    # Apply Gaussian blur
    gaussianblur.gaussianblur(img, sigma, apply_to_alpha_channel)
    print("Gaussian blur applied.")
    
    # Save the output image
    out_np_img = np.array(img.data, dtype=np.uint8).reshape(img.geom.rows, img.geom.cols, img.geom.channels)
    PILImage.fromarray(out_np_img, pil_img.mode).save(output_file)
    print("Output written to", output_file)

if __name__ == "__main__":
    main()
```

## How It Works

The Python module leverages the robust and efficient algorithms implemented in the underlying C++ library:

- **Centered Kernel Generation:** Automatically generates a centered Gaussian kernel to avoid circular convolution artifacts.
- **FFT-based Convolution:** Performs FFT on both the kernel and image tiles to achieve efficient, high-performance convolution in each dimension and for each tile.
- **Multi-threading & SIMD Optimizations:** Ensures speedy processing without fearing any large images, thanks to FFT and parallel processing.

## Additional Information

For more details about the underlying C++ library and its capabilities, please refer to the [LibGaussianBlur repository](https://github.com/michelerenzullo/LibGaussianBlur).

## Contributing

Contributions are welcome! If you have suggestions or improvements, please open an issue or submit a pull request on GitHub.

## License

This project is licensed under the Apache-2.0 License. See the [LICENSE](https://raw.githubusercontent.com/michelerenzullo/LibGaussianBlur/refs/heads/main/LICENSE) file for details.