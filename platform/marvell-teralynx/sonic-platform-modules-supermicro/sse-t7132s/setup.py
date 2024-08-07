from setuptools import setup

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation on Supermicro SSE-T7132S',
    license='Apache 2.0',
    author='SuperMicro Team',
    author_email='support@supermicro.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='SuperMicro',
    maintainer_email='support@supermicro.com',
    packages=[
        'sonic_platform',
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

