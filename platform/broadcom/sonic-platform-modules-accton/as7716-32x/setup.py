#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7716_32x',
   version='1.0',
   description='Module to initialize Accton AS7716-32X platforms',
   
   packages=['as7716_32x'],
   package_dir={'as7716_32x': 'as7716-32x/classes'},
)

