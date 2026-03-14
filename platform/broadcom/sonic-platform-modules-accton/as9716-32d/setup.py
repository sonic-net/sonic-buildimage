#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as9716_32d',
   version='1.0',
   description='Module to initialize Accton AS9716-32D platforms',
   
   packages=['as9716_32d'],
   package_dir={'as9716_32d': 'as9716-32d/classes'},
)

