#include <array>
#include <chrono>
#include <cmath>
#include <gaussianblur/gaussianblur.h>
#include <gaussianblur/helpers.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include "test_helpers.hpp"

// Test case for prepare_kernel_DFT
TEST(GaussianBlurTest, PrepareKernelDFT) {
  const ImgGeom image_geom = {4, 4, 3};  // 4x4 image with 3 channels (RGB)
  float sigma = 2.0F;

  // Prepare the kernel DFT
  KernelDFT kernel_dft = gaussianblur::prepare_kernel_DFT(image_geom, sigma);

  // Check that the DFT arrays are not empty
  ASSERT_FALSE(kernel_dft.kerf_1D_row.empty());
  ASSERT_FALSE(kernel_dft.kerf_1D_col.empty());

  // Check the sizes of the DFT arrays
  ASSERT_EQ(kernel_dft.kerf_1D_row.size(), kernel_dft.kerf_1D_col.size());
  // Iterate through the elements and compare them
  for (size_t i = 0; i < kernel_dft.kerf_1D_row.size(); ++i)
    ASSERT_EQ(kernel_dft.kerf_1D_row.at(i), kernel_dft.kerf_1D_col.at(i));
}

// Test case for Gaussian blur without applying to alpha channel
TEST(GaussianBlurTest, BasicTestRGB) {
  // Create a 3x3 RGB image with sharp contrasts
  std::vector<uint8_t> image_data = {// Row 1
                                     255, 0, 0, 0, 255, 0, 0, 0, 255,
                                     // Row 2
                                     0, 0, 0, 255, 255, 255, 128, 128, 128,
                                     // Row 3
                                     128, 0, 0, 0, 128, 0, 0, 0, 128};

  const ImgGeom image_geom = {3, 3, 3};  // 3x3 image with 3 channels (RGB)
  Image image = {image_data, image_geom};

  float sigma = 3.0F;
  bool apply_to_alpha = false;

  // Calculate the average color and variance of the original image
  float original_mean = calculate_average_color(image.data);
  float original_variance = calculate_variance(image.data, original_mean);

  // Apply Gaussian blur
  gaussianblur::gaussianblur(image, sigma, apply_to_alpha);

  // Calculate the average color and variance of the blurred image
  float blurred_mean = calculate_average_color(image.data);
  float blurred_variance = calculate_variance(image.data, blurred_mean);

  // Check that the variance is lower in the blurred image
  ASSERT_LT(blurred_variance, original_variance);
}

// Test case that
TEST(GaussianBlurTest, InvalidChannelCount) {
  // Create a 3x3 image with 2 channels
  std::vector<uint8_t> image_data = {// Row 1
                                     255, 0, 0, 255, 0, 0,
                                     // Row 2
                                     0, 0, 255, 255, 128, 128,
                                     // Row 3
                                     128, 0, 0, 128, 0, 0};
  std::vector<uint8_t> original_image_data = image_data;

  ImgGeom image_geom = {3, 3, 2};  // 3x3 image with 2 channels
  Image image = {image_data, image_geom};

  float sigma = 3.0F;
  bool apply_to_alpha = false;

  gaussianblur::gaussianblur(image, sigma, apply_to_alpha);

  // Assert that no processing was done and the data is unchanged
  ASSERT_EQ(image.data, original_image_data);
}

// Test case for Gaussian blur without applying to alpha channel
TEST(GaussianBlurTest, BasicTestRGBAWithoutAlpha) {
  // Create a 3x3 RGBA image with sharp contrasts
  const std::vector<uint8_t> image_data = {
      // Row 1
      255, 0, 0, 128, 0, 255, 0, 128, 0, 0, 255, 128,
      // Row 2
      0, 0, 0, 128, 255, 255, 255, 128, 128, 128, 128, 128,
      // Row 3
      128, 0, 0, 128, 0, 128, 0, 128, 0, 0, 128, 128};

  const ImgGeom image_geom = {3, 3, 4};  // 3x3 image with 4 channels (RGBA)
  Image image = {image_data, image_geom};

  float sigma = 3.0F;
  bool apply_to_alpha = false;

  // Calculate the average color and variance of the original image
  const float original_mean = calculate_average_color(image.data);
  const float original_variance = calculate_variance(image.data, original_mean);

  // Apply Gaussian blur
  gaussianblur::gaussianblur(image, sigma, apply_to_alpha);

  // Calculate the average color and variance of the blurred image
  const float blurred_mean = calculate_average_color(image.data);
  const float blurred_variance = calculate_variance(image.data, blurred_mean);

  // Check that the variance is lower in the blurred image
  ASSERT_LT(blurred_variance, original_variance);

  // Check that the alpha channel is unaltered
  for (size_t i = 3; i < image.data.size(); i += 4) {
    ASSERT_EQ(image.data[i], 128);
  }
}

// Stress test for Gaussian blur with a large image. Skip if coverage is
// enabled.
#ifndef WITH_COVERAGE
TEST(GaussianBlurTest, StressTest) {
  // Create a large image (e.g., 10240x10240 RGBA) with random data
  const size_t width = 10240;
  const size_t height = 10240;
  const size_t channels = 4;
  std::vector<uint8_t> image_data(width * height * channels);

  // Fill the image with random data
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);
  for (auto& pixel : image_data) {
    pixel = dis(gen);
  }

  const ImgGeom image_geom = {width, height, channels};
  Image image = {image_data, image_geom};

  const float sigma = 5.0F;
  const bool apply_to_alpha = true;

  // Measure the time taken to apply Gaussian blur
  auto start = std::chrono::high_resolution_clock::now();
  gaussianblur::gaussianblur(image, sigma, apply_to_alpha);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;

  // Print the duration
  std::cout << "Gaussian blur applied in " << duration.count() << " seconds."
            << std::endl;

  // This is quite arbitrary, but since the purpose of this library is also to
  // not fear large images, we can assert that the duration is within an
  // acceptable range like < 10 seconds
  ASSERT_LT(duration.count(), 10.0);

  // Check that the function completes (no specific assertions needed for stress
  // test)
  SUCCEED();
}
#endif

// Test case for Gaussian blur with applying to alpha channel
TEST(GaussianBlurTest, BasicTestRGBAWithAlpha) {
  // Create a 3x3 RGBA image with sharp contrasts
  std::vector<uint8_t> image_data = {
      // Row 1
      255, 0, 0, 128, 0, 255, 0, 128, 0, 0, 255, 128,
      // Row 2
      0, 0, 0, 128, 255, 255, 255, 128, 128, 128, 128, 128,
      // Row 3
      128, 0, 0, 128, 0, 128, 0, 128, 0, 0, 128, 128};

  ImgGeom image_geom = {3, 3, 4};  // 3x3 image with 4 channels (RGBA)
  Image image = {image_data, image_geom};

  float sigma = 3.0F;
  bool apply_to_alpha = true;

  // Calculate the average color and variance of the original image
  float original_mean = calculate_average_color(image.data);
  float original_variance = calculate_variance(image.data, original_mean);

  // Apply Gaussian blur
  gaussianblur::gaussianblur(image, sigma, apply_to_alpha);

  // Calculate the average color and variance of the blurred image
  float blurred_mean = calculate_average_color(image.data);
  float blurred_variance = calculate_variance(image.data, blurred_mean);

  // Check that the variance is lower in the blurred image
  ASSERT_LT(blurred_variance, original_variance);

  // Check that the alpha channel has been altered
  bool alpha_altered = false;
  for (size_t i = 3; i < image.data.size(); i += 4)
    if (image.data[i] != 128) {
      alpha_altered = true;
      break;
    }
  ASSERT_TRUE(alpha_altered);
}

// Test case for flip_block
TEST(HelpersTest, FlipBlock) {
  // Create a simple 2x2 block
  std::vector<float> input = {1.0F, 2.0F, 3.0F, 4.0F};
  std::vector<float> output(4);

  // Flip the block
  flip_block<1>(input.data(), output.data(), 2, 2);

  // Check the results
  const std::vector<float> expected_output = {1.0F, 3.0F, 2.0F, 4.0F};
  ASSERT_EQ(output, expected_output);
}

// Test case for deinterleave_channels
TEST(HelpersTest, DeinterleaveChannels) {
  // Create a 3x3 RGB image
  std::vector<uint8_t> interleaved = {// Row 1
                                      255, 0, 0, 0, 255, 0, 0, 0, 255,
                                      // Row 2
                                      0, 0, 0, 255, 255, 255, 128, 128, 128,
                                      // Row 3
                                      128, 0, 0, 0, 128, 0, 0, 0, 128};

  std::vector<float> red(9), green(9), blue(9);
  std::array<float*, 3> channels = {red.data(), green.data(), blue.data()};

  // Deinterleave the channels
  deinterleave_channels<3>(interleaved.data(), channels.data(), 9);

  // Check the results
  const std::vector<float> expected_red = {255, 0, 0, 0, 255, 128, 128, 0, 0};
  const std::vector<float> expected_green = {0, 255, 0, 0, 255, 128, 0, 128, 0};
  const std::vector<float> expected_blue = {0, 0, 255, 0, 255, 128, 0, 0, 128};

  ASSERT_EQ(red, expected_red);
  ASSERT_EQ(green, expected_green);
  ASSERT_EQ(blue, expected_blue);
}

// Test case for interleave_channels
TEST(HelpersTest, InterleaveChannels) {
  // Create separate channels for a 3x3 RGB image
  std::vector<float> red = {255, 0, 0, 0, 255, 128, 128, 0, 0};
  std::vector<float> green = {0, 255, 0, 0, 255, 128, 0, 128, 0};
  std::vector<float> blue = {0, 0, 255, 0, 255, 128, 0, 0, 128};
  std::array<const float*, 3> channels = {red.data(), green.data(),
                                          blue.data()};

  std::vector<uint8_t> interleaved(27);

  // Interleave the channels
  interleave_channels<3>(channels.data(), interleaved.data(), 9);

  // Check the results
  const std::vector<uint8_t> expected_interleaved = {// Row 1
                                               255, 0, 0, 0, 255, 0, 0, 0, 255,
                                               // Row 2
                                               0, 0, 0, 255, 255, 255, 128, 128,
                                               128,
                                               // Row 3
                                               128, 0, 0, 0, 128, 0, 0, 0, 128};

  ASSERT_EQ(interleaved, expected_interleaved);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}