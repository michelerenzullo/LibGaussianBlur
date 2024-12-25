#include <gaussianblur/gaussianblur.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"
#include "../stb_image_write.h"

struct STBImageDeleter {
  void operator()(uint8_t* ptr) const { stbi_image_free(ptr); }
};

void write_image(std::string file, const Image& image) {
  std::string ext = file.substr(file.size() - 3, 3);
  file = file.substr(0, file.size() - 4) + std::string("_pffft.") + ext;
  if (ext == "bmp")
    stbi_write_bmp(file.c_str(), image.geom.cols, image.geom.rows,
                   image.geom.channels, image.data.data());
  else if (ext == "jpg")
    stbi_write_jpg(file.c_str(), image.geom.cols, image.geom.rows,
                   image.geom.channels, image.data.data(), 100);
  else {
    if (ext != "png") {
      printf("Image format '%s' not supported, writing default png\n",
             ext.c_str());
      file = file.substr(0, file.size() - 4) + std::string(".png");
    }
    stbi_write_png(file.c_str(), image.geom.cols, image.geom.rows,
                   image.geom.channels, image.data.data(),
                   image.geom.channels * image.geom.cols);
  }
}

std::optional<std::vector<uint8_t>> read_image(const std::string& file,
                                               int& cols, int& rows,
                                               int& channels) {
  std::unique_ptr<uint8_t, STBImageDeleter> image_data(
      stbi_load(file.c_str(), &cols, &rows, &channels, 0));
  if (!image_data) {
    throw std::runtime_error("Failed to load image: " + file);
  }

  printf("Source image: %s %dx%d (%d)\n", file.c_str(), cols, rows, channels);
  if (channels != 3 && channels != 4) {
    throw std::runtime_error(
        "Image format not supported, only RGB and RGBA images are supported");
  }

  return {std::vector<uint8_t>(image_data.get(),
                               image_data.get() + cols * rows * channels)};
}

void print_help() {
  std::cout << "Usage: gaussianblur <smoothing_factor> <input_file> "
               "[apply_to_alpha]\n";
  std::cout << "  <smoothing_factor> : The smoothing factor for the Gaussian "
               "blur (must be > 0).\n";
  std::cout << "  <input_file>       : The input image file.\n";
  std::cout << "  [apply_to_alpha]            : Optional. If set to 1, the "
               "convolution is done on the 4th channel (alpha channel).\n";
  std::cout << "                       If not provided or set to 0, the "
               "convolution is done on the first 3 channels only.\n";
}

int main(int argc, char* argv[]) {
  // Check for help argument
  if (argc == 2 &&
      (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
    print_help();
    return 0;
  }

  // Check for the correct number of arguments
  if (argc < 3 || argc > 4) {
    std::cerr << "Invalid number of arguments.\n";
    print_help();
    return 1;
  }

  // If the image has the alpha channel, the convolution is done on the 4th
  // channel if alpha is true, otherwise on the first 3 channels only
  bool apply_to_alpha = false;
  if (argc == 4) apply_to_alpha = std::stoi(argv[3]) == 1;
  std::string file_name(argv[2]);
  float sigma = std::stof(argv[1]);
  if (sigma <= 0) {
    printf("Invalid smoothing factor\n");
    return 1;
  }

  int cols, rows, channels;
  std::optional<std::vector<uint8_t>> image_data;
  if (!(image_data = read_image(file_name, cols, rows, channels)).has_value())
    return 1;
  Image image = {std::move(image_data.value()), ImgGeom{rows, cols, channels}};

  gaussianblur::gaussianblur(image, sigma, apply_to_alpha);

  write_image(file_name, image);

  return 0;
}