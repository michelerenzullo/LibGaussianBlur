#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <gaussianblur/gaussianblur.h>

namespace py = pybind11;

PYBIND11_MODULE(gaussianblur, m) {
    m.doc() = "Python bindings for LibGaussianBlur: Apply Gaussian blur to images through Fast Fourier Transform (FFT).";

    // Bind ImgGeom struct.
    py::class_<ImgGeom>(m, "ImgGeom", "Structure that stores image geometry including number of rows, columns, and channels.")
        .def(py::init<>(), "Creates an empty ImgGeom object.")
        .def_readwrite("rows", &ImgGeom::rows, "Number of rows in the image.")
        .def_readwrite("cols", &ImgGeom::cols, "Number of columns in the image.")
        .def_readwrite("channels", &ImgGeom::channels, "Number of channels in the image.");

    // Bind Image struct.
    py::class_<Image>(m, "Image", "Structure representing an image, containing image data and its geometry.")
        .def(py::init<>(), "Creates an empty Image object.")
        .def_readwrite("data", &Image::data, "The image pixel data.")
        .def_readwrite("geom", &Image::geom, "Geometry information for the image.");

    // Bind the gaussianblur function.
    // This function modifies the Image in place.
    m.def("gaussianblur", &gaussianblur::gaussianblur,
          py::arg("image"),
          py::arg("sigma"),
          py::arg("apply_to_alpha"),
          "Applies a Gaussian blur to the provided image. Parameters:\n"
          " - image: the image object to be blurred\n"
          " - sigma: standard deviation for the Gaussian kernel\n"
          " - apply_to_alpha: boolean flag to apply the blur to the alpha channel if present");
}