from setuptools import setup

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation on Ciena Platforms based on PDDF',
    license='Apache 2.0',
    author='Ciena',
    author_email='support@ciena.com',
    url='https://www.ciena.com/',
    packages=['sonic_platform', 'ciena'],
    package_dir={
        'sonic_platform': 'common/sonic_platform',
        'ciena': 'common/ciena',
    },
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
    keywords='sonic SONiC platform PLATFORM',
)
