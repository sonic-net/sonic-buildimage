from setuptools import setup

DEVICE_NAME = 'accton'
HW_SKU = 'x86_64-accton_as7116_54x-r0'

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation on Accton Platforms',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='Simon Ji',
    maintainer_email='Simon.Ji@mediatek.com',
    packages=[
       'sonic_platform',
    ],
    package_dir={
       'sonic_platform': '../../../device/{}/{}/sonic_platform'.format(DEVICE_NAME, HW_SKU)},
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
