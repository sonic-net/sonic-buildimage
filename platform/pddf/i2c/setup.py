#!/usr/bin/env python

from setuptools import setup
import os

setup(
   name='pddf-platform',
   version='%s' % os.environ.get('PLATFORM_MODULE_VERSION', '1.0'),
   description='Module to initialize Platform',
   packages=[
      'modules',
   ],
)

