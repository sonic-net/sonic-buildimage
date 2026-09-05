#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='ix9_32x',
   version='1.0',
   description='Module to initialize Quanta IX9-32X platforms',

   packages=['ix9_32x'],
   package_dir={'ix9_32x': 'ix9-32x/classes'},
)

