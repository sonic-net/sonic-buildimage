#!/usr/bin/env python3

from setuptools import setup

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation for NextHop Aspeed AST2700 BMC',
    license='Apache 2.0',
    author='NextHop',
    author_email='opensource@nexthop.ai',
    url='https://github.com/nexthop-ai/private-sonic-buildimage',
    maintainer='NextHop',
    maintainer_email='opensource@nexthop.ai',
    packages=['sonic_platform'],
    package_dir={'sonic_platform': 'sonic_platform'},
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Plugins',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3.9',
        'Topic :: Utilities',
    ],
    keywords='sonic SONiC platform PLATFORM bmc aspeed ast2700 nexthop',
)

