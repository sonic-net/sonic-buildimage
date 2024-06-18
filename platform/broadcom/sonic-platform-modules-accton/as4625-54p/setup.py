#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='as4625_54p',
   version='1.0',
   description='Module to initialize Accton AS4625-54P platforms',

   packages=['as4625_54p'],
   package_dir={'as4625_54p': 'as4625-54p/classes'},
)
