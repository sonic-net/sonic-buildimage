#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
       name='esc600-128q',
       version='1.0',
       description='Module to initialize Cameo Esc601-128q platforms',

       packages=['esc600-128q'],
       package_dir={'esc600-128q': 'esc600-128q/credo_baldeagle/python_wheel'},
       package_data={'esc600-128q': 'esc600-128q/credo_baldeagle/python_wheel/*.py'}
    )
