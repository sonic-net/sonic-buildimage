#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='sonic_platform',
   version='1.0',
   description='ESQC610-56SQ sonic platform API',
   
   packages=['sonic_platform'],
   package_dir={'sonic_platform': 'esqc610-56sq/sonic_platform'},
)