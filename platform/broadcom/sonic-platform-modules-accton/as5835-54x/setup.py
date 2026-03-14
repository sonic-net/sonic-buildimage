#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as5835_54x',
   version='1.0',
   description='Module to initialize Accton AS5835-54X platforms',
   
   packages=['as5835_54x'],
   package_dir={'as5835_54x': 'as5835-54x/classes'},
)

