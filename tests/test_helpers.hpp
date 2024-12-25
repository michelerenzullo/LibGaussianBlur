#pragma once

#include <cstdint>
#include <vector>

// Helper function to calculate the average color value of an image
float calculate_average_color(const std::vector<uint8_t>& data) {
  float sum = 0;
  for (uint8_t value : data) {
    sum += value;
  }
  return sum / data.size();
}

// Helper function to calculate the variance of pixel values in an image
float calculate_variance(const std::vector<uint8_t>& data, float mean) {
  float variance = 0;
  for (uint8_t value : data) {
    variance += (value - mean) * (value - mean);
  }
  return variance / data.size();
}