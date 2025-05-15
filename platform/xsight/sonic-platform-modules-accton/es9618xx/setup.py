#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='es9618xx',
   version='1.0',
   description='Module to initialize Accton ES9618XX platforms',

   packages=['es9618xx'],
   package_dir={'es9618xx': 'es9618xx/classes'},
)

