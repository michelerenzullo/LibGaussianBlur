#pragma once

#include <vector>
#include <array>
#include <optional>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <gaussianblur/helpers.hpp>

namespace gaussianblur {

void pffft(const ImgGeom image_geometry, const KernelDFT kernelDFT, DeinterleavedChs& temp, bool alpha = false);
KernelDFT prepare_kernel_DFT(const ImgGeom image_geometry, const float nsmooth);
void copy_work_array(Image& image, const DeinterleavedChs temp);
std::optional<DeinterleavedChs> prepare_work_array(const Image& image);
void gaussianblur(Image& image, const float nsmooth, const bool alpha);

} // namespace gaussianblur
