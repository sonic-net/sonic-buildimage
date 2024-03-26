#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='es9632xx',
   version='1.0',
   description='Module to initialize Accton ES9632XX platforms',

   packages=['es9632xx'],
   package_dir={'es9632xx': 'es9632xx/classes'},
)

