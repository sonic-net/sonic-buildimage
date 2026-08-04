#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='ix8a_bwde_56x',
   version='1.0',
   description='Module to initialize Quanta IX8A-BWDE-56X platforms',
   
   packages=['ix8a_bwde_56x'],
   package_dir={'ix8a_bwde_56x': 'ix8a-bwde-56x/classes'},
)

