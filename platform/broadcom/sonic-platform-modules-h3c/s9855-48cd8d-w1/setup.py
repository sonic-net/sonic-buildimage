#!/usr/bin/env python

from setuptools import setup

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation on H3C Platforms',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='H3C',
    maintainer_email='h3c-sonic@h3c.com',
    packages=[
        'sonic_platform',
        'vendor_sonic_platform'
    ],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Plugins',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 2.7',
        'Topic :: Utilities',
    ],
    keywords='sonic SONiC platform PLATFORM',
)
