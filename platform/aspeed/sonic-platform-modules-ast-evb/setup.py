import os
from setuptools import setup

setup(
    name="sonic-platform",
    version="1.0",
    description="SONiC platform API implementation on Aspeed EVB Platforms",
    license="Apache 2.0",
    author="Aspeed Technology Inc.",
    author_email="opensource@aspeedtech.com",
    url="https://github.com/AspeedTech-BMC/sonic-buildimage",
    packages=["sonic_platform"],
    package_dir={
        "sonic_platform": "ast2700/sonic_platform",
    },
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Environment :: Plugins",
        "Intended Audience :: Developers",
        "Intended Audience :: Information Technology",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: Apache Software License",
        "Natural Language :: English",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 3.11",
        "Topic :: Utilities",
    ],
    keywords="sonic SONiC platform PLATFORM aspeed bmc",
)

