#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7726_32x',
   version='1.0',
   description='Module to initialize Accton AS7726-32X platforms',
   
   packages=['as7726_32x'],
   package_dir={'as7726_32x': 'as7726-32x/classes'},
)

