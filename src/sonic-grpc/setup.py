from pathlib import Path
import sys

from setuptools import setup
from setuptools.command.build_py import build_py

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_protos import generate


class BuildPy(build_py):
    def run(self):
        generate()
        super().run()

setup(
    name='sonic-grpc',
    version='1.0',
    description='gRPC client frameworks for SONiC (gNOI, and future gNMI/gRIBI)',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/sonic-net/sonic-buildimage',
    packages=[
        'sonic_grpc',
        'sonic_grpc.gnoi',
    ],
    cmdclass={'build_py': BuildPy},
    python_requires='>=3.9',
    install_requires=[
        'grpcio>=1.66.2',
        'protobuf>=5.29.6,<8',
    ],
    extras_require={
        'testing': [
            'build',
            'grpcio==1.66.2',
            'grpcio-tools==1.66.2',
            'protobuf==5.29.6',
            'pytest',
            'setuptools>=61',
            'wheel',
        ],
    },
    license_files=['proto/LICENSE'],
    classifiers=[
        'Intended Audience :: Developers',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3',
    ],
    keywords='sonic SONiC gnoi grpc',
)
