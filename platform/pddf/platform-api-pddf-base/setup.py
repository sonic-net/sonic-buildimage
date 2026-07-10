
from setuptools import setup

setup(
    name='sonic-platform-pddf-common',
    version='1.0',
    description='SONIC platform APIs base (2.0) implementation on PDDF supported platforms',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='Fuzail Khan',
    maintainer_email='fuzail.khan@broadcom.com',
    packages=[
        'sonic_platform_pddf_base',
    ],
    install_requires=[
        'jsonschema==2.6.0'
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
        'Programming Language :: Python :: 3.7',
        'Topic :: Utilities',
    ],
    keywords='sonic SONiC platform PLATFORM',
)
