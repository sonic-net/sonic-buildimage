#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='as4625_54t',
   version='1.0',
   description='Module to initialize Accton AS4625-54T platforms',

   packages=['as4625_54t'],
   package_dir={'as4625_54t': 'as4625-54t/classes'},
)
