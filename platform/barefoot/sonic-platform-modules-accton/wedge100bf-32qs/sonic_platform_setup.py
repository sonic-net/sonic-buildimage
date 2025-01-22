from setuptools import setup

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='Barefoot',
    maintainer_email='',
    packages=[
        'sonic_platform',
        'sonic_platform/pltfm_mgr_rpc',
        'sonic_platform/bfn_extensions',
    ],
    package_data = {'sonic_platform':['logging.conf']},
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
