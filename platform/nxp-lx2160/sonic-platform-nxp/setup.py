from setuptools import setup

setup(
    name="sonic-platform",
    version="1.0",
    description="SONiC platform API for the NXP LX2160ARDB",
    license="Apache 2.0",
    packages=["sonic_platform"],
    classifiers=[
        "Environment :: Plugins",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 3",
    ],
)
