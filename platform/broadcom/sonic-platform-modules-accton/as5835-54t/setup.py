#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='as5835_54t',
   version='1.0',
   description='Module to initialize Accton AS5835-54T platforms',
   
   packages=['as5835_54t'],
   package_dir={'as5835_54t': 'as5835-54t/classes'},
)

