#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7712_32x',
   version='1.0',
   description='Module to initialize Accton AS7712-32X platforms',
   
   packages=['as7712_32x'],
   package_dir={'as7712_32x': 'as7712-32x/classes'},
)

