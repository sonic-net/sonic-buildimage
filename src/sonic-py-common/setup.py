from __future__ import print_function
import os
import platform
import sys
from setuptools import setup
from setuptools.command.build_py import build_py
import pkg_resources
from packaging import version


def target_architecture():
    configured_arch = os.environ.get('CONFIGURED_ARCH')
    if configured_arch:
        return configured_arch

    machine = platform.machine()
    return 'armhf' if machine.startswith(('armv6', 'armv7', 'armv8l')) else machine


include_sonic_grpc = sys.version_info >= (3, 9) and target_architecture() != 'armhf'

extra_packages = []
extra_dependencies = []
extra_setup_requires = []
extra_testing_requires = []
cmdclass = {}
license_files = None

if include_sonic_grpc:
    from generate_protos import generate

    class BuildPy(build_py):
        def run(self):
            generate()
            build_py.run(self)

    extra_packages = [
        'sonic_grpc',
        'sonic_grpc.gnoi',
    ]
    extra_dependencies = [
        'grpcio>=1.66.2',
        'protobuf>=5.29.6,<8',
    ]
    extra_setup_requires = ['grpcio-tools==1.66.2']
    extra_testing_requires = [
        'grpcio==1.66.2',
        'grpcio-tools==1.66.2',
        'protobuf==5.29.6',
    ]
    cmdclass = {'build_py': BuildPy}
    license_files = ['proto/LICENSE']

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

setup_args = dict(
    name='sonic-py-common',
    version='1.0',
    description='Common Python libraries for SONiC',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/SONiC',
    maintainer='Joe LeVeque',
    maintainer_email='jolevequ@microsoft.com',
    install_requires=dependencies + extra_dependencies,
    packages=[
        'sonic_py_common',
    ] + extra_packages,
    cmdclass=cmdclass,
    setup_requires= [
        'pytest-runner',
        'wheel',
    ] + extra_setup_requires,
    tests_require=[
        'pytest',
        'mock==3.0.5' # For python 2. Version >=4.0.0 drops support for py2
    ] + extra_testing_requires,
    extras_require={
        'testing': ['pytest'] + extra_testing_requires,
    },
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

if license_files is not None:
    setup_args['license_files'] = license_files

setup(**setup_args)
