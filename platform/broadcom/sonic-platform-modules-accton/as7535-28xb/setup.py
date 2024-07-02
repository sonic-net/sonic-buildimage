#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='as7535_28xb',
   version='1.0',
   description='Module to initialize Accton AS7535-28XB platforms',

   packages=['as7535_28xb'],
   package_dir={'as7535_28xb': 'as7535-28xb/classes'},
)
