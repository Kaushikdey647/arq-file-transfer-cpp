#include "lib/arq.h"
#include "lib/pybind11/pybind11.h"
#include "lib/pybind11/stl.h"

namespace py = pybind11;

PYBIND11_MODULE(arq, m) {
    py::class_<arq>(m, "arq")
        .def(py::init<>())
        .def("encode_frames", &arq::encode_frames)
        .def("decode_frames", &arq::decode_frames)
        .def("arq_sock_listen", &arq::arq_sock_listen)
        .def("arq_sock_connect", &arq::arq_sock_connect);
}