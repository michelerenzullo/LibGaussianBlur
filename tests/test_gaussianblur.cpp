#include <gtest/gtest.h>
#include <gaussianblur/gaussianblur.h>
#include <gaussianblur/helpers.hpp>
#include "test_helpers.hpp"
#include <cmath>
#include <random>
#include <chrono>

// Test case for prepare_kernel_DFT
TEST(GaussianBlurTest, PrepareKernelDFT) {
    ImgGeom image_geom = {4, 4, 3}; // 4x4 image with 3 channels (RGB)
    float sigma = 2.0f;

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
TEST(GaussianBlurTest, BasicTestWithoutAlpha) {
    // Create a 3x3 RGBA image with sharp contrasts
    std::vector<uint8_t> image_data = {
        // Row 1
        255, 0, 0, 128,    0, 255, 0, 128,       0, 0, 255, 128,
        // Row 2
        0, 0, 0, 128,      255, 255, 255, 128,   128, 128, 128, 128,
        // Row 3
        128, 0, 0, 128,    0, 128, 0, 128,       0, 0, 128, 128
    };

    ImgGeom image_geom = {2, 2, 4}; // 2x2 image with 4 channels (RGBA)
    Image image = {image_data, image_geom};

    float sigma = 3.0f;
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

    // Check that the alpha channel is unaltered
    for (size_t i = 3; i < image.data.size(); i += 4) {
        ASSERT_EQ(image.data[i], 128);
    }
}

// Stress test for Gaussian blur with a large image
TEST(GaussianBlurTest, StressTest) {
    // Create a large image (e.g., 10240x10240 RGBA) with random data
    const int width = 10240;
    const int height = 10240;
    const int channels = 4;
    std::vector<uint8_t> image_data(width * height * channels);

    // Fill the image with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto& pixel : image_data) {
        pixel = dis(gen);
    }

    ImgGeom image_geom = {width, height, channels};
    Image image = {image_data, image_geom};

    float sigma = 5.0f;
    bool apply_to_alpha = true;

    // Measure the time taken to apply Gaussian blur
    auto start = std::chrono::high_resolution_clock::now();
    gaussianblur::gaussianblur(image, sigma, apply_to_alpha);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    // Print the duration
    std::cout << "Gaussian blur applied in " << duration.count() << " seconds." << std::endl;

    // This is quite arbitrary, but since the purpose of this library is also to not
    // fear large images, we can assert that the duration is within an acceptable range
    // like < 10 seconds
    ASSERT_LT(duration.count(), 10.0);

    // Check that the function completes (no specific assertions needed for stress test)
    SUCCEED();
}

// Test case for Gaussian blur with applying to alpha channel
TEST(GaussianBlurTest, BasicTestWithAlpha) {
    // Create a 3x3 RGBA image with sharp contrasts
    std::vector<uint8_t> image_data = {
        // Row 1
        255, 0, 0, 128,    0, 255, 0, 128,       0, 0, 255, 128,
        // Row 2
        0, 0, 0, 128,      255, 255, 255, 128,   128, 128, 128, 128,
        // Row 3
        128, 0, 0, 128,    0, 128, 0, 128,       0, 0, 128, 128
    };

    ImgGeom image_geom = {3, 3, 4}; // 2x2 image with 4 channels (RGBA)
    Image image = {image_data, image_geom};

    float sigma = 3.0f;
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

    // Check that the alpha channel is altered
    bool alpha_altered = false;
    for (size_t i = 3; i < image.data.size(); i += 4) {
        if (image.data[i] != 128) {
            alpha_altered = true;
            break;
        }
    }
    ASSERT_TRUE(alpha_altered);
}

// Test case for flip_block
TEST(HelpersTest, FlipBlock) {
    // Create a simple 2x2 block
    std::vector<float> input = {
        1.0f, 2.0f,
        3.0f, 4.0f
    };
    std::vector<float> output(4);

    // Flip the block
    flip_block<1>(input.data(), output.data(), 2, 2);

    // Check the results
    std::vector<float> expected_output = {
        1.0f, 3.0f,
        2.0f, 4.0f
    };
    ASSERT_EQ(output, expected_output);
}

// Test case for deinterleave_channels
TEST(HelpersTest, DeinterleaveChannels) {
    // Create a simple 2x2 RGB image with known values
    std::vector<uint8_t> interleaved = {
        255, 0, 0,    // Red
        0, 255, 0,    // Green
        0, 0, 255,    // Blue
        255, 255, 255 // White
    };

    std::vector<float> red(4), green(4), blue(4);
    std::array<float*, 3> channels = { red.data(), green.data(), blue.data() };

    // Deinterleave the channels
    deinterleave_channels<3>(interleaved.data(), channels.data(), 4);

    // Check the results
    std::vector<float> expected_red = { 255, 0, 0, 255 };
    std::vector<float> expected_green = { 0, 255, 0, 255 };
    std::vector<float> expected_blue = { 0, 0, 255, 255 };

    ASSERT_EQ(red, expected_red);
    ASSERT_EQ(green, expected_green);
    ASSERT_EQ(blue, expected_blue);
}

// Test case for interleave_channels
TEST(HelpersTest, InterleaveChannels) {
    // Create separate channels for a 2x2 RGB image
    std::vector<float> red = { 255, 0, 0, 255 };
    std::vector<float> green = { 0, 255, 0, 255 };
    std::vector<float> blue = { 0, 0, 255, 255 };
    std::array<const float*, 3> channels = { red.data(), green.data(), blue.data() };

    std::vector<uint8_t> interleaved(12);

    // Interleave the channels
    interleave_channels<3>(channels.data(), interleaved.data(), 4);

    // Check the results
    std::vector<uint8_t> expected_interleaved = {
        255, 0, 0,    // Red
        0, 255, 0,    // Green
        0, 0, 255,    // Blue
        255, 255, 255 // White
    };

    ASSERT_EQ(interleaved, expected_interleaved);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}