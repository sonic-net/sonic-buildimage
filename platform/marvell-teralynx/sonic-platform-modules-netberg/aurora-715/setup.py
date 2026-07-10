#!/usr/bin/env python

import os
# import sys
from setuptools import setup
os.listdir

setup(
    name='sonic_platform',
    version='1.0',
    description='Netberg Aurora 715 sonic platform API',

    packages=['sonic_platform'],
    package_dir={'sonic_platform': 'aurora-715/sonic_platform'},


    classifiers=[
        'Development Status :: 3 - Beta',
        'Environment :: Plugins',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.7',
        'Topic :: Utilities',
    ],
)
