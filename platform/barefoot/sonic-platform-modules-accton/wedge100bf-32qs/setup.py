#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='wedge100bf-32qs',
   version='1.0',
   description='Module to initialize Accton wedge100bf-32qs platforms',

   packages=['wedge100bf-32qs'],
   package_dir={'wedge100bf-32qs': 'wedge100bf-32qs/classes'},
)
