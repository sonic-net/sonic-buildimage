#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='sonic_platform',
   version='1.0',
   description='Module to initialize Celestica B3010 platforms',
      
   packages=['questone2', 'sonic_platform'],
   package_dir={'questone2'      : 'classes',
                'sonic_platform': 'sonic_platform'},
)

