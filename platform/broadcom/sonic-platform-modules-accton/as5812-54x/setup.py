#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as5812_54x',
   version='1.0',
   description='Module to initialize Accton AS5812-54X platforms',
   
   packages=['as5812_54x'],
   package_dir={'as5812_54x': 'as5812-54x/classes'},
)

