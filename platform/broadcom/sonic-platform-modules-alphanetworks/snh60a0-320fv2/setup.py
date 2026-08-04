#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
   name='snh60a0_320fv2',
   version='1.0',
   description='Module to initialize Alphanetworks SNH60A0-320FV2 platforms',
   
   packages=['snh60a0_320fv2'],
   package_dir={'snh60a0_320fv2': 'snh60a0-320fv2/classes'},
)

