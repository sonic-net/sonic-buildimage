from setuptools import setup

setup(
    name="nexthop-utils",
    version="1.0",
    description="Common utilities for Nexthop SONiC platforms",
    license="Apache 2.0",
    author="Nexthop Team",
    author_email="sonic-oss@nexthop.ai",
    url="https://github.com/nexthop-ai/sonic-buildimage",
    packages=["nexthop_utils"],
    install_requires=["click", "natsort"],
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
        "Programming Language :: Python :: 3.11",
        "Topic :: Utilities",
    ],
    keywords="sonic SONiC nexthop utils eeprom platform",
)
