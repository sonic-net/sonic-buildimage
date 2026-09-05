#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
       name='cs5435-54p',
       version='1.0.0',
       description='Module to initialize Cig CS5435-54P platforms',

       packages=['cs5435-54p'],
       package_dir={'cs5435-54p': 'cs5435-54p/classes'},
    )
