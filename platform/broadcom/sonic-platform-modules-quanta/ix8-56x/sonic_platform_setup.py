import os
from setuptools import setup
os.listdir

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation on Quanta Platforms',
    license='Apache 2.0',    
    packages=['sonic_platform'],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Plugins',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',        
        'Programming Language :: Python :: 3.7',
        'Topic :: Utilities',
    ],
    keywords='sonic SONiC platform PLATFORM',
)
