#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7312_54x',
   version='1.0',
   description='Module to initialize Accton AS7312-54X platforms',
   
   packages=['as7312_54x'],
   package_dir={'as7312_54x': 'as7312-54x/classes'},
)

