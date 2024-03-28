#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='n8550_32c',
   version='1.0',
   description='Module to initialize FS N8550-32C platforms',
   
   packages=['n8550_32c'],
   package_dir={'n8550_32c': 'n8550-32c/classes'},
)

