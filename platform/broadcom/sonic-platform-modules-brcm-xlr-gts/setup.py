#!/usr/bin/env python

from setuptools import setup
import os

setup(
   name='platform-xlr-gts',
   version='%s' % os.environ.get('BRCM_XLR_GTS_PLATFORM_MODULE_VERSION', '1.0'),
   description='Module to initialize Broadcom XLR/GTS platforms',
   packages=[],
)

