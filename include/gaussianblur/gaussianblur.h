#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <gaussianblur/helpers.hpp>
#include <numeric>
#include <optional>
#include <vector>

namespace gaussianblur {
/**
 * @brief Prepares the DFT of the Gaussian kernel based on the image geometry
 * and smoothing factor.
 *
 * @param image_geometry The geometry of the image (dimensions and channels).
 * @param sigma The smoothing factor for the Gaussian blur.
 * @return KernelDFT The precomputed DFT of the Gaussian kernel.
 */
KernelDFT prepare_kernel_DFT(const ImgGeom image_geometry, const float sigma);

/**
 * @brief Applies Gaussian blur to the image.
 *
 * @param image The image to be blurred.
 * @param sigma The smoothing factor for the Gaussian blur.
 * @param apply_to_alpha If true, applies the blur to the apply_to_alpha
 * channel; otherwise, applies to RGB channels.
 */
void gaussianblur(Image &image, const float sigma, const bool apply_to_alpha);

}  // namespace gaussianblur