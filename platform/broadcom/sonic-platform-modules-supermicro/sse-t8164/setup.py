#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='sse-t8164',
   version='1.0',
   description='Module to initialize Supermicro T8164 platforms',

   packages=['sse-t8164'],
   package_dir={'sse-t8164': ''}
)

