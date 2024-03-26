#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='es9632xq',
   version='1.0',
   description='Module to initialize Accton ES9632XQ platforms',

   packages=['es9632xq'],
   package_dir={'es9632xq': 'es9632xq/classes'},
)

