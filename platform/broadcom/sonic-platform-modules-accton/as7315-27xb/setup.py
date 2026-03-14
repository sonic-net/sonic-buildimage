#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7315_27xb',
   version='1.0',
   description='Module to initialize Accton AS7315-27XB platforms',
   
   packages=['as7315_27xb'],
   package_dir={'as7315_27xb': 'as7315-27xb/classes'},
)

