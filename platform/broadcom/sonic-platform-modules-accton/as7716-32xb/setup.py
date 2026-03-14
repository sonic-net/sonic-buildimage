#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7716_32xb',
   version='1.0',
   description='Module to initialize Accton AS7716-32XB platforms',
   
   packages=['as7716_32xb'],
   package_dir={'as7716_32xb': 'as7716-32xb/classes'},
)

