#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7312_54xs',
   version='1.0',
   description='Module to initialize Accton AS7312-54XS platforms',
   
   packages=['as7312_54xs'],
   package_dir={'as7312_54xs': 'as7312-54xs/classes'},
)

