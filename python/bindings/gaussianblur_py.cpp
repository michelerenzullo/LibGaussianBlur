#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <gaussianblur/gaussianblur.h>

namespace py = pybind11;

PYBIND11_MODULE(gaussianblur, m) {
    m.doc() = "Python bindings for LibGaussianBlur";

    // Bind ImgGeom struct
    py::class_<ImgGeom>(m, "ImgGeom")
        .def(py::init<>())
        .def_readwrite("rows", &ImgGeom::rows)
        .def_readwrite("cols", &ImgGeom::cols)
        .def_readwrite("channels", &ImgGeom::channels);

    // Bind Image struct
    py::class_<Image>(m, "Image")
        .def(py::init<>())
        .def_readwrite("data", &Image::data)
        .def_readwrite("geom", &Image::geom);

    // Bind the gaussianblur function.
    // This function modifies the Image in place.
    m.def("gaussianblur", &gaussianblur::gaussianblur,
          py::arg("image"),
          py::arg("sigma"),
          py::arg("apply_to_alpha"),
          "Applies Gaussian blur to the provided image");
}