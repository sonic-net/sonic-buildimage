from setuptools import setup

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
    python_requires='>=3.9',
    install_requires=[
        'grpcio',
        'protobuf>=4.21',
    ],
    extras_require={
        'testing': ['pytest'],
    },
    classifiers=[
        'Intended Audience :: Developers',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3',
    ],
    keywords='sonic SONiC gnoi grpc',
)
