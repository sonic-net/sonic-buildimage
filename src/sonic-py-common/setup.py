from __future__ import print_function
import sys
from setuptools import setup
import pkg_resources
from packaging import version

# sonic_dependencies, version requirement only supports '>='
sonic_dependencies = ['redis-dump-load']

dependencies = [
    'natsort',
    'pyyaml',
]

dependencies += sonic_dependencies
for package in sonic_dependencies:
    try:
        package_dist = pkg_resources.get_distribution(package.split(">=")[0])
    except pkg_resources.DistributionNotFound:
        print(package + " is not found!", file=sys.stderr)
        print("Please build and install SONiC python wheels dependencies from sonic-buildimage", file=sys.stderr)
        exit(1)
    if ">=" in package:
        if version.parse(package_dist.version) >= version.parse(package.split(">=")[1]):
            continue
        print(package + " version not match!", file=sys.stderr)
        exit(1)

setup(
    name='sonic-py-common',
    version='1.0',
    description='Common Python libraries for SONiC',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/SONiC',
    maintainer='Joe LeVeque',
    maintainer_email='jolevequ@microsoft.com',
    install_requires=dependencies,
    packages=[
        'sonic_py_common',
    ],
    setup_requires= [
        'pytest-runner',
        'wheel'
    ],
    tests_require=[
        'pytest',
        'mock==3.0.5' # For python 2. Version >=4.0.0 drops support for py2
    ],
    entry_points={
        'console_scripts': [
            'sonic-db-load = sonic_py_common.sonic_db_dump_load:sonic_db_dump_load',
            'sonic-db-dump = sonic_py_common.sonic_db_dump_load:sonic_db_dump_load',
        ],
    },
    classifiers=[
        'Intended Audience :: Developers',
        'Operating System :: Linux',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python',
    ],
    keywords='SONiC sonic PYTHON python COMMON common',
    test_suite = 'setup.get_test_suite'
)

