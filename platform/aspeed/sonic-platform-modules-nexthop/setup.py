import os
from setuptools import setup

setup(
    name="sonic-platform",
    version="1.0",
    description="SONiC platform API implementation on NextHop Aspeed Platforms",
    license="Apache 2.0",
    author="Nexthop Team",
    author_email="opensource@nexthop.ai",
    url="https://github.com/nexthop-ai/sonic-buildimage",
    packages=["sonic_platform"],
    package_dir={
        "sonic_platform": "b27/sonic_platform",
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
    keywords="sonic SONiC platform PLATFORM aspeed bmc nexthop",
)

