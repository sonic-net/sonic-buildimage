#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='n5850_48s6c',
   version='1.0',
   description='Module to initialize FS N5850 platforms',
   
   packages=['n5850_48s6c'],
   package_dir={'n5850_48s6c': 'n5850-48s6c/classes'},
)

