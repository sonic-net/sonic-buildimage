#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='sonic_platform',
   version='1.0',
   description='ESC600-128Q sonic platform API',
   
   packages=['sonic_platform'],
   package_dir={'sonic_platform': 'esc600-128q/sonic_platform'},
)