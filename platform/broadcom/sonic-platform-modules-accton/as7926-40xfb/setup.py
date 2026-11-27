#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='as7926_40xfb',
   version='1.0',
   description='Module to initialize Accton AS7926-40XFB platforms',

   packages=['as7926_40xfb'],
   package_dir={'as7926_40xfb': 'as7926-40xfb/classes'},
)

