from setuptools import setup

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='support',
    url='',
    maintainer='support',
    maintainer_email='',
    packages=[
        'sonic_platform',
        'plat_hal',
        'wbutil',
        'eepromutil',
        'hal-config',
        'config',
    ],
    py_modules=[
        'hal_pltfm',
        'platform_util',
        'platform_intf',
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
        'Programming Language :: Python :: 3.7',
        'Topic :: Utilities',
    ],
    keywords='sonic SONiC platform PLATFORM',
)
