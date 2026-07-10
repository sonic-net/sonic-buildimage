from __future__ import print_function
from setuptools import setup
import sys
import pkg_resources
from packaging import version

# sonic_dependencies, version requirement only supports '>='
sonic_dependencies = ['sonic-py-common']

dependencies = [
    'natsort',
    'docker'
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
    name='system-health',
    version='1.0',
    description='SONiC system health package',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='Junchao Chen',
    maintainer_email='junchaow@mellanox.com',
    install_requires=dependencies,
    packages=[
        'health_checker',
        'tests'
    ],
    scripts=[
        'scripts/healthd',
    ],
    setup_requires=[
        'pytest-runner'
    ],
    tests_require=[
        'pytest',
        'mock>=2.0.0'
    ],
    classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: No Input/Output (Daemon)',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3.7',
        'Topic :: System :: Hardware',
    ],
    keywords='SONiC sonic HEALTH health',
    test_suite='setup.get_test_suite'
)
