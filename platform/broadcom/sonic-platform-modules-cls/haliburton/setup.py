from setuptools import setup

DEVICE_NAME = 'celestica'
HW_SKU = 'x86_64-cel_e1031-r0'

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation on Celestica Platforms',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='Wirut Getbamrung',
    maintainer_email='wgetbumr@celestica.com',
    packages=[
        'sonic_platform',
    ],
    package_dir={
        'sonic_platform': '../../../../device/{}/{}/sonic_platform'.format(DEVICE_NAME, HW_SKU)},
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
