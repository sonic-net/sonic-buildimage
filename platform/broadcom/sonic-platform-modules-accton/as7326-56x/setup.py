#!/usr/bin/env pytho

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as7326_56x',
   version='1.0',
   description='Module to initialize Accton AS7326-56X platforms',
   
   packages=['as7326_56x'],
   package_dir={'as7326_56x': 'as7326-56x/classes'},
)

