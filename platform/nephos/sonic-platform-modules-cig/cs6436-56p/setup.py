#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
       name='cs6436-56p',
       version='1.0.0',
       description='Module to initialize Cig CS6436-56P platforms',

       packages=['cs6436-56p'],
       package_dir={'cs6436-56p': 'cs6436-56p/classes'},
    )
