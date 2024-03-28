#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='n5850_48x6c',
   version='1.0',
   description='Module to initialize FS N5850-48X6C platforms',
   
   packages=['n5850_48x6c'],
   package_dir={'n5850_48x6c': 'n5850-48x6c/classes'},
)

