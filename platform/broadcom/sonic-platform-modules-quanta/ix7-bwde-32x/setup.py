#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='ix7_bwde_32x',
   version='1.0',
   description='Module to initialize Quanta IX7-BWDE-32X platforms',
   
   packages=['ix7_bwde_32x'],
   package_dir={'ix7_bwde_32x': 'ix7-bwde-32x/classes'},
)

