#!/usr/bin/env python

import os
from setuptools import setup
os.listdir

setup(
   name='sonic_platform',
   version='1.0',
   description='escc601-32Q sonic platform API',
   
   packages=['sonic_platform'],
   package_dir={'sonic_platform': 'escc601-32q/sonic_platform'},
)