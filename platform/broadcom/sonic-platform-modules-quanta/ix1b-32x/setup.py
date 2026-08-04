#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='ix1b_32x',
   version='1.0',
   description='Module to initialize Quanta IX1B-32X platforms',
   
   packages=['ix1b_32x'],
   package_dir={'ix1b_32x': 'ix1b-32x/classes'},
)

