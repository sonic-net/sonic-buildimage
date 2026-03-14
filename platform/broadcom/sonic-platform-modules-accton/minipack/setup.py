#!/usr/bin/env python

import os
import sys
from setuptools import setup, Extension
os.listdir

module1 = Extension("fbfpgaio", sources = ["minipack/lib/fbfpgaiomodule.c"])

setup(
   name='minipack',
   version='1.0',
   description='Module to initialize Accton MiniPack platforms',
   
   packages=['minipack'],
   package_dir={'minipack': 'minipack/classes'},
   ext_modules=[module1],
   
)

