"""SWIG rules for generating Python bindings."""

load(":extract.bzl", _swig_lib_deb = "swig_lib_deb")
load(":gen.bzl", _swig_gen = "swig_gen", _swig_gen_go = "swig_gen_go")

swig_lib_deb = _swig_lib_deb
swig_gen = _swig_gen
swig_gen_go = _swig_gen_go
