#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='ix8_56x',
   version='1.0',
   description='Module to initialize Quanta IX8-56X platforms',
   
   packages=['ix8_56x'],
   package_dir={'ix8_56x': 'ix8-56x/classes'},
)

