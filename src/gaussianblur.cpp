
#include <gaussianblur/gaussianblur.h>
#include <gaussianblur/helpers.hpp>
#include <numbers>
extern "C" {
  #include <pffft_pommier/pffft.h>
}

namespace gaussianblur {

int gaussian_window(const float sigma, const int max_width = 0) {
  // calculate the width necessary for the provided sigma
  // return an odd width for the kernel, if max is passed check that is not
  // bigger than it

  const float radius = std::max(sigma * sqrt(2 * log(255)) - 1, 0.0);
  int width = radius * 2 + 0.5F;
  if (max_width) width = std::min(width, max_width);

  if (width % 2 == 0) ++width;

  // printf("sigma %f radius %f - width %d - max_width %d\n", sigma, radius,
  // width, max_width);

  return width;
}

template <typename T>
void get_gaussian(T &kernel, const float sigma, int width = 0,
                  int FFT_length = 0) {
  // create a 1D and zero padded gaussian kernel
  if (!width) width = gaussian_window(sigma);

  kernel.resize(FFT_length ? FFT_length : width);

  const float mid_w = (width - 1) / 2.0F;
  const float s = 2.0F * sigma * sigma;

  int i = 0;

  for (float y = -mid_w; y <= mid_w; ++y, ++i)
    kernel.at(i) = (exp(-(y * y) / s)) / (std::numbers::pi_v<float> * s);

  const float sum =
      1.0F / std::accumulate(kernel.begin(), kernel.begin() + width, 0.0F);

  std::transform(kernel.begin(), kernel.begin() + width, kernel.begin(),
                 [&sum](const auto &i) { return i * sum; });

  // fit the kernel in the FFT_length and shift the center to avoid circular
  // convolution
  if (FFT_length) {
    // same of with raw pointers
    // std::rotate(std::reverse_iterator(&kernel[0] + FFT_length),
    // std::reverse_iterator(&kernel[0] + width / 2),
    // std::reverse_iterator(&kernel[0]));
    std::rotate(kernel.rbegin(), kernel.rbegin() + (kernel.size() - width / 2),
                kernel.rend());
  }
}

template <typename T, typename N>
void pffft_sorted_optimized_convolution(std::vector<T, N> &tile_dft,
                                        const std::vector<T, N> &kernel_dft,
                                        float scaler) {
  // Do the convolution in the frequency domain without accumulation.
  // Assuming that:
  //   - the DFT obtained from pffft is **sorted** in the conventional way
  //   - imaginary part of the centered kernel is 0, which is our case (skip
  //   multiplication for imaginay part of the kernel)
  for (int i = 0; i < tile_dft.size() / 2; i++) {
    const int real_part_idx = 2 * i;
    const int imag_part_idx = 2 * i + 1;
    const float real_part_kernel_multiplier =
        kernel_dft.at(real_part_idx) * scaler;
    tile_dft.at(real_part_idx) *= real_part_kernel_multiplier;
    tile_dft.at(imag_part_idx) *= real_part_kernel_multiplier;
  }
}

// Utils from pffft to check the nearest efficient transform size of FFT
int is_valid_size(int N) {
  const int N_min = 32;
  int R = N;
  while (R >= 5 * N_min && (R % 5) == 0) R /= 5;
  while (R >= 3 * N_min && (R % 3) == 0) R /= 3;
  while (R >= 2 * N_min && (R % 2) == 0) R /= 2;
  return (R == N_min) ? 1 : 0;
}

int nearest_transform_size(int N) {
  const int N_min = 32;
  if (N < N_min) N = N_min;
  N = N_min * ((N + N_min - 1) / N_min);

  while (!is_valid_size(N)) N += N_min;
  return N;
}

KernelDFT prepare_kernel_DFT(const ImgGeom image_geometry, const float sigma) {
  std::chrono::time_point<std::chrono::steady_clock> start_0 =
      std::chrono::steady_clock::now();
  // calculate a good width of the kernel for our sigma
  int kSize = gaussian_window(
      sigma, std::max(image_geometry.rows, image_geometry.cols));

  int pad = (kSize - 1) / 2;

  // absolute min padd
  std::array<int, 2> sizes = {image_geometry.rows + pad * 2,
                              image_geometry.cols + pad * 2};

  // if the length of the data is not decomposable in small prime numbers 2 - 3
  // - 5, is necessary to update the size adding more pad as trailing zeros
  std::array<int, 2> trailing_zeros = {0, 0};

  for (int i = 0; i < 2; ++i) {
    if (!is_valid_size(sizes.at(i))) {
      int new_size = nearest_transform_size(sizes.at(i));
      trailing_zeros.at(i) = (new_size - sizes.at(i));
      sizes.at(i) = new_size;
    }
  }

  // fast convolve by pffft, without reordering the z-domain. Thus, we perform a
  // row by row, col by col FFT and convolution with 2x1D kernel

  AlignedVector<float> kernel_aligned_1D_col(sizes.at(1));

  // create a gaussian 1D kernel with the specified sigma and kernel size, and
  // center it in a length of FFT_length
  get_gaussian(kernel_aligned_1D_col, sigma, kSize, sizes.at(1));

  AlignedVector<float> kerf_1D_col(sizes.at(1));
  AlignedVector<float> kerf_1D_row(sizes.at(0));

  PFFFT_Setup_UniquePtr cols_setup(pffft_new_setup(sizes.at(1), PFFFT_REAL));
  PFFFT_Setup_UniquePtr rows_setup(pffft_new_setup(sizes.at(0), PFFFT_REAL));

  AlignedVector<float> tmp;
  const int maxsize = std::max(sizes.at(0), sizes.at(1));
  tmp.reserve(maxsize);
  tmp.resize(sizes.at(1));

  pffft_transform_ordered(cols_setup.get(), kernel_aligned_1D_col.data(),
                          kerf_1D_col.data(), tmp.data(), PFFFT_FORWARD);

  // calculate the DFT of kernel by col if size of cols is not the same of rows
  if (sizes.at(0) != sizes.at(1)) {
    AlignedVector<float> kernel_aligned_1D_row(sizes.at(0));
    get_gaussian(kernel_aligned_1D_row, sigma, kSize, sizes.at(0));

    tmp.resize(sizes.at(0));
    pffft_transform_ordered(rows_setup.get(), kernel_aligned_1D_row.data(),
                            kerf_1D_row.data(), tmp.data(), PFFFT_FORWARD);

  } else
    std::copy(kerf_1D_col.begin(), kerf_1D_col.end(), kerf_1D_row.begin());

#ifdef TIMING
  printf("Kernel DFT prepared in %f ms\n",
         std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now() - start_0)
             .count());
#endif
  return {std::move(kerf_1D_row),
          std::move(kerf_1D_col),
          std::move(rows_setup),
          std::move(cols_setup),
          pad,
          TrailingZeros{trailing_zeros.at(0), trailing_zeros.at(1)}};
}

std::optional<DeinterleavedChs> deinterleave_image_channels(Image &image) {
  DeinterleavedChs deinterleaved_vector;

  int img_size_per_channel = image.geom.rows * image.geom.cols;
  if (image.geom.channels == 3) {
    deinterleaved_vector =
        DeinterleavedChs(3, std::vector<float>(img_size_per_channel));
    std::array<float *, 3> BGR = {deinterleaved_vector.at(0).data(),
                                  deinterleaved_vector.at(1).data(),
                                  deinterleaved_vector.at(2).data()};
    deinterleave_channels<3>(image.data.data(), BGR.data(),
                             image.geom.rows * image.geom.cols);
  } else if (image.geom.channels == 4) {
    deinterleaved_vector =
        DeinterleavedChs(4, std::vector<float>(img_size_per_channel));
    std::array<float *, 4> BGRA = {
        deinterleaved_vector.at(0).data(), deinterleaved_vector.at(1).data(),
        deinterleaved_vector.at(2).data(), deinterleaved_vector.at(3).data()};
    deinterleave_channels<4>(image.data.data(), BGRA.data(),
                             image.geom.rows * image.geom.cols);
  } else {
    std::cerr << "Unsupported number of channels" << std::endl;
    return std::nullopt;
  }

  return std::optional<DeinterleavedChs>{std::move(deinterleaved_vector)};
}

void process_channel_tiles(
    const int channel, const int tiles, const int tile_size, const int pad,
    const int trailing_zeros, PFFFT_Setup *setup,
    const AlignedVector<float> &kernel, AlignedVector<float> &tmp,
    AlignedVector<float> &tile, AlignedVector<float> &work,
    AlignedVector<float> &resf, DeinterleavedChs &deinterleaved_channels,
    float scaler) {
  tmp.resize(kernel.size());
  tile.resize(kernel.size());
  work.resize(kernel.size());
  hybrid_loop(tiles, [&](auto j) {
    auto tmp_local(tmp), tile_local(tile), work_local(work);

    // copy the tile and pad by reflection in the aligned vector
    // left reflected pad
    std::copy_n(
        std::reverse_iterator(deinterleaved_channels.at(channel).data() +
                              j * tile_size + pad + 1),
        pad, tile_local.begin());
    // middle
    std::copy_n(deinterleaved_channels.at(channel).data() + j * tile_size,
                tile_size, tile_local.begin() + pad);
    // right reflected pad
    std::copy_n(
        std::reverse_iterator(deinterleaved_channels.at(channel).data() +
                              (j + 1) * tile_size - 1),
        pad, tile_local.end() - pad /* fft trailing 0s --> */ - trailing_zeros);

    pffft_transform_ordered(setup, tile_local.data(), work_local.data(),
                            tmp_local.data(), PFFFT_FORWARD);
    pffft_sorted_optimized_convolution(work_local, kernel, scaler);
    pffft_transform_ordered(setup, work_local.data(), tile_local.data(),
                            tmp_local.data(), PFFFT_BACKWARD);

    // save the 1st pass tile per tile in the output vector
    std::copy_n(tile_local.begin() + pad, tile_size,
                resf.begin() + j * tile_size);
  });

  // transpose cache-friendly, took from FastBoxBlur
  flip_block<1>(resf.data(), deinterleaved_channels.at(channel).data(),
                tile_size, tiles);
}

void pffft(const ImgGeom image_geometry, const KernelDFT kernelDFT,
           DeinterleavedChs &deinterleaved_channels, bool apply_to_alpha) {
  std::chrono::time_point<std::chrono::steady_clock> start_1 =
      std::chrono::steady_clock::now();
  const int maxsize =
      std::max(kernelDFT.kerf_1D_row.size(), kernelDFT.kerf_1D_col.size());
  const float divisor_col = 1.0F / kernelDFT.kerf_1D_col.size();
  const float divisor_row = 1.0F / kernelDFT.kerf_1D_row.size();

  AlignedVector<float> tmp;
  tmp.reserve(maxsize);

  int ch_to_process = 3;
  if (image_geometry.channels == 4 && apply_to_alpha) ch_to_process = 4;

  for (int i = 0; i < ch_to_process; ++i) {
    AlignedVector<float> resf(image_geometry.rows * image_geometry.cols);
    AlignedVector<float> tile, work;
    tile.reserve(maxsize);
    work.reserve(maxsize);

    // Process the convolution row per row and transpose the result
    process_channel_tiles(i, image_geometry.rows, image_geometry.cols,
                          kernelDFT.pad, kernelDFT.trailing_zeros.cols,
                          kernelDFT.cols_setup.get(), kernelDFT.kerf_1D_col,
                          tmp, tile, work, resf, deinterleaved_channels,
                          divisor_col);

    // Process the convolution col per col and transpose the result
    process_channel_tiles(i, image_geometry.cols, image_geometry.rows,
                          kernelDFT.pad, kernelDFT.trailing_zeros.rows,
                          kernelDFT.rows_setup.get(), kernelDFT.kerf_1D_row,
                          tmp, tile, work, resf, deinterleaved_channels,
                          divisor_row);
  }
#ifdef TIMING
  printf("Convolution done in %f ms\n",
         std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now() - start_1)
             .count());
#endif
}

void copy_processed_data_to_image(
    Image &image, const DeinterleavedChs deinterleaved_channels) {
  if (image.geom.channels == 3) {
    std::array<const float *, 3> BGR = {deinterleaved_channels.at(0).data(),
                                        deinterleaved_channels.at(1).data(),
                                        deinterleaved_channels.at(2).data()};
    interleave_channels<3>(BGR.data(), image.data.data(),
                           image.geom.rows * image.geom.cols);
  } else if (image.geom.channels == 4) {
    std::array<const float *, 4> BGRA = {deinterleaved_channels.at(0).data(),
                                         deinterleaved_channels.at(1).data(),
                                         deinterleaved_channels.at(2).data(),
                                         deinterleaved_channels.at(3).data()};
    interleave_channels<4>(BGRA.data(), image.data.data(),
                           image.geom.rows * image.geom.cols);
  }
}

void gaussianblur(Image &image, const float sigma, const bool apply_to_alpha) {
  // If the image has the alpha channel, the convolution is done on the 4th
  // channel if alpha is true, otherwise on the first 3 channels only
  if (sigma <= 0) {
    printf("Invalid smoothing factor\n");
    return;
  }
  std::optional<DeinterleavedChs> deinterleaved_channels;
  if (!(deinterleaved_channels = deinterleave_image_channels(image))
           .has_value())
    return;

  KernelDFT kernelDFT = prepare_kernel_DFT(image.geom, sigma);
  pffft(image.geom, std::move(kernelDFT), deinterleaved_channels.value(),
        apply_to_alpha);

  copy_processed_data_to_image(image,
                               std::move(deinterleaved_channels.value()));
}
}  // namespace gaussianblur
