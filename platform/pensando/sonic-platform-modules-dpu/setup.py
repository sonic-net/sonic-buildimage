from setuptools import setup, find_packages
from setuptools.command.build_py import build_py as _build_py
import distutils.command
import os
import sys
import glob

class GrpcTool(distutils.cmd.Command):
    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        import grpc_tools.protoc

        proto_files = glob.glob('sonic_platform/proto/**/*.proto', recursive=True)  # Find all .proto files

        for proto_file in proto_files:
            grpc_tools.protoc.main([
                'grpc_tools.protoc',
                '-Isonic_platform/proto',
                '--python_out=sonic_platform',
                '--grpc_python_out=sonic_platform',
                proto_file
            ])

class BuildPyCommand(_build_py, object):

    def initialize_options(self):
        # execute GrpcTool only if the proto dir is present.
        if os.path.exists('sonic_platform/proto'):
            self.run_command('GrpcTool')

        proto_py_files = glob.glob('sonic_platform/**/*_pb2*.py', recursive=True)
        for py_file in proto_py_files:
            if not os.path.exists(py_file):
                print('Required file not present: {0}'.format(py_file))
                sys.exit(1)

        super(BuildPyCommand, self).initialize_options()

setup(
    name='sonic-platform',
    version='1.0',
    description='SONiC platform API implementation on Pensando DPU Platform',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='AMD',
    maintainer_email='shantanu.shrivastava@amd.com',
    packages=[
        'sonic_platform'
    ],
    package_data={'': ['*.json', '*.sh', '*.service', 'meta/*']},
    cmdclass={
        'build_py': BuildPyCommand,
        'GrpcTool': GrpcTool
    },
    setup_requires=[
        'grpcio-tools<=1.66.2',
        'pytest-runner'
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
