#include <emscripten/bind.h>
#include <emscripten/val.h>

typedef emscripten::val em_val;

#include <gaussianblur/gaussianblur.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"
#include "../stb_image_write.h"

struct STBImageDeleter {
  void operator()(uint8_t* ptr) const { stbi_image_free(ptr); }
};

struct free_delete {
  void operator()(void* ptr) const { free(ptr); }
};

std::vector<unsigned char> write_image_to_memory(const Image& image,
                                                 const std::string& ext) {
  std::vector<unsigned char> memory_buffer;

  if (ext == "jpg" || ext == "jpeg") {
    stbi_write_jpg_to_func(
        [](void* context, void* data, int size) {
          auto* buffer = reinterpret_cast<std::vector<unsigned char>*>(context);
          buffer->insert(buffer->end(), static_cast<unsigned char*>(data),
                         static_cast<unsigned char*>(data) + size);
        },
        &memory_buffer, image.geom.cols, image.geom.rows, image.geom.channels,
        image.data.data(), 100);
  } else {
    // Default to PNG if format is unsupported
    stbi_write_png_to_func(
        [](void* context, void* data, int size) {
          auto* buffer = reinterpret_cast<std::vector<unsigned char>*>(context);
          buffer->insert(buffer->end(), static_cast<unsigned char*>(data),
                         static_cast<unsigned char*>(data) + size);
        },
        &memory_buffer, image.geom.cols, image.geom.rows, image.geom.channels,
        image.data.data(), image.geom.channels * image.geom.cols);
  }

  return memory_buffer;
}

std::optional<std::vector<uint8_t>> read_image(
    const std::unique_ptr<uint8_t[], free_delete> file, const size_t file_size,
    int& cols, int& rows, int& channels) {
  std::unique_ptr<uint8_t, STBImageDeleter> image_data(
      stbi_load_from_memory(file.get(), file_size, &cols, &rows, &channels, 0));
  if (!image_data) {
    throw std::runtime_error("Failed to load image: ");
  }

  printf("Source image: %dx%d (%d)\n", cols, rows, channels);
  if (channels != 3 && channels != 4) {
    throw std::runtime_error(
        "Image format not supported, only RGB and RGBA images are supported");
  }

  return {std::vector<uint8_t>(image_data.get(),
                               image_data.get() + cols * rows * channels)};
}

em_val run(uintptr_t args_ptr, uintptr_t target_ptr, size_t target_size) {
#ifdef __EMSCRIPTEN_PTHREADS__
  printf("Multi-Threaded LibGaussianBlur, threads available: %d\n",
         (int)std::thread::hardware_concurrency());
#else
  printf("Single-Threaded LibGaussianBlur\n");
#endif

  auto args_ = std::unique_ptr<char[], free_delete>((char*)args_ptr);
  auto target = std::unique_ptr<uint8_t[], free_delete>((uint8_t*)target_ptr);
  const em_val Uint8Array = em_val::global("Uint8Array");
  em_val file_copy = em_val::null();
  std::string extension = "png";

  bool apply_to_alpha = false;
  float sigma = 0.0f;

#ifdef TIMING
  auto start = chrono::steady_clock::now();  // benchmark disabled
#endif
  // treat the char arguments array as a string
  std::string args(args_.get());

  const char* delimiter = "-";
  args.append(delimiter);  // add final "-" in order to read the last argument
                           // inside the for loop
  size_t pos = 0;
  args.erase(0, args.find(delimiter) +
                    strlen(delimiter));  // ignore the first empty entry ""
                                         // before the first "-"
  for (std::string arg; (pos = args.find(delimiter)) != std::string::npos;
       args.erase(0, pos + strlen(delimiter))) {
    arg = args.substr(0, pos);
    switch (arg[0]) {
      case 'a':
        apply_to_alpha = true;
        break;
      case 's':
        sigma = (float)atof(arg.c_str() + 1);
        break;
      case 'e':
        extension = arg.substr(1);
        extension.erase(0, extension.find_first_not_of(" "));
        extension.erase(extension.find_last_not_of(" ") + 1);
        break;
      default:
        // fprintf(stderr, "Unknown option \"-%c \".\n", arg[0]);
        break;
    }
  }

  if (sigma > 0) {
    int cols, rows, channels;
    std::optional<std::vector<uint8_t>> image_data;
    if ((image_data =
             read_image(std::move(target), target_size, cols, rows, channels))
            .has_value()) {
      Image image = {std::move(image_data.value()),
                     ImgGeom{rows, cols, channels}};

      gaussianblur::gaussianblur(image, sigma, apply_to_alpha);

      auto img_in_mem = write_image_to_memory(image, extension);

      file_copy = Uint8Array.new_(
          emscripten::typed_memory_view(img_in_mem.size(), img_in_mem.data()));
    }
  }

  // Fill the js output object
  em_val results = em_val::object();
  results.set("file_output", file_copy);

  // Check mem leaks
  //__lsan_do_recoverable_leak_check();

  return results;
}

EMSCRIPTEN_BINDINGS(my_module) { emscripten::function("start", &run); }