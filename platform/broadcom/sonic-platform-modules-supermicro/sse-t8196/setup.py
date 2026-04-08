#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='sse-t8196',
   version='1.0',
   description='Module to initialize Supermicro T8196 platforms',

   packages=['sse-t8196'],
   package_dir={'sse-t8196': ''}
)

