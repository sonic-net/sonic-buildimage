#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='ix7_32x',
   version='1.0',
   description='Module to initialize Quanta IX7-32X platforms',
   
   packages=['ix7_32x'],
   package_dir={'ix7_32x': 'ix7-32x/classes'},
)

