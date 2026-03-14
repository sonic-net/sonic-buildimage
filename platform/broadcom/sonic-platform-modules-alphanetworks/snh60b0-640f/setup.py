#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='snh60b0_640f',
   version='1.0',
   description='Module to initialize Alphanetworks SNH60B0-640F platforms',
   
   packages=['snh60b0_640f'],
   package_dir={'snh60b0_640f': 'snh60b0-640f/classes'},
)

