#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='n9550_32d',
   version='1.0',
   description='Module to initialize FS N9550-32D platforms',
   
   packages=['n9550_32d'],
   package_dir={'n9550_32d': 'n9550-32d/classes'},
)

