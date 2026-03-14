#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7816_64x',
   version='1.0',
   description='Module to initialize Accton AS7816-64X platforms',
   
   packages=['as7816_64x'],
   package_dir={'as7816_64x': 'as7816-64x/classes'},
)

