#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='sonic_platform',
   version='1.0',
   description='ESC602-32Q sonic platform API',
   
   packages=['sonic_platform'],
   package_dir={'sonic_platform': 'esc602-32q/sonic_platform'},
)