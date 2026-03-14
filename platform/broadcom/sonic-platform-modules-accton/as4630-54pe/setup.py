#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as4630_54pe',
   version='1.0',
   description='Module to initialize Accton AS4630-54PE platforms',
   
   packages=['as4630_54pe'],
   package_dir={'as4630_54pe': 'as4630-54pe/classes'},
)

