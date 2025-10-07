from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_include
from setuptools import setup

ext_modules = [
    Pybind11Extension(
        "optipricer",
        [
            "src/python_bindings.cpp",
        ],
        include_dirs=[
            "include",
            get_include(),
        ],
        language='c++',
        cxx_std=14,
    ),
]

setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
