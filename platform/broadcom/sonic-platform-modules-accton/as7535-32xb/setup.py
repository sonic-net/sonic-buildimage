#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='as7535_32xb',
   version='1.0',
   description='Module to initialize Accton AS7535-32XB platforms',

   packages=['as7535_32xb'],
   package_dir={'as7535_32xb': 'as7535-32xb/classes'},
)
