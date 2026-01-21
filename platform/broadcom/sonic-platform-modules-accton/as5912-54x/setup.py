#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as5912_54x',
   version='1.0',
   description='Module to initialize Accton AS5912-54X platforms',
   
   packages=['as5912_54x'],
   package_dir={'as5912_54x': 'as5912-54x/classes'},
)

