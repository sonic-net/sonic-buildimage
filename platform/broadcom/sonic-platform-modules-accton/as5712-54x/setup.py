#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as5712_54x',
   version='1.0',
   description='Module to initialize Accton AS5712-54X platforms',
   
   packages=['as5712_54x'],
   package_dir={'as5712_54x': 'as5712-54x/classes'},
)

