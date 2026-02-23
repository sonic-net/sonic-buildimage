import os
import sys
from setuptools import setup

os.listdir

setup(
    name="sonic-platform",
    version="1.0",
    description="SONiC platform API implementation on Nexthop Platforms based on PDDF",
    license="Apache 2.0",
    author="Nexthop Team",
    author_email="michaelc@nexthop.ai",
    url="https://github.com/nexthop-ai/sonic-buildimage",
    packages=["sonic_platform", "nexthop"],
    package_dir={
        "sonic_platform": "common/sonic_platform",
        "nexthop": "common/nexthop",
    },
    tests_require=["pytest"],
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Environment :: Plugins",
        "Intended Audience :: Developers",
        "Intended Audience :: Information Technology",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: Apache Software License",
        "Natural Language :: English",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 3.9",
        "Topic :: Utilities",
    ],
    keywords="sonic SONiC platform PLATFORM",
    test_suite='setup.get_test_suite'
)
