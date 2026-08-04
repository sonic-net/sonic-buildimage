#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='ix8c_56x',
   version='1.0',
   description='Module to initialize Quanta IX8C-56X platforms',

   packages=['ix8c_56x'],
   package_dir={'ix8c_56x': 'ix8c-56x/classes'},
)

