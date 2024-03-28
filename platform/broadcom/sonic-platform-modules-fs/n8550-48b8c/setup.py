#!/usr/bin/env pytho

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='n8550_48b8c',
   version='1.0',
   description='Module to initialize FS N8550-48B8C platforms',
   
   packages=['n8550_48b8c'],
   package_dir={'n8550_48b8c': 'n8550-48b8c/classes'},
)

