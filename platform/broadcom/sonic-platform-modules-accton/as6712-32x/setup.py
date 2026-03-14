#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as6712_32x',
   version='1.0',
   description='Module to initialize Accton AS6712-32X platforms',
   
   packages=['as6712_32x'],
   package_dir={'as6712_32x': 'as6712-32x/classes'},
)

