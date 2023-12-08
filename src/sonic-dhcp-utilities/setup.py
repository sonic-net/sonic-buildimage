from setuptools import setup, find_packages

dependencies = [
    "psutil"
]

test_deps = [
    "pytest",
    "freezegun"
]

setup(
    name="sonic-dhcp-utilities",
    install_requires=dependencies,
    description="Module of SONiC built-in dhcp_utilities",
    version="1.0",
    url="https://github.com/Azure/sonic-buildimage",
    tests_require=test_deps,
    author="SONiC Team",
    author_email="yaqiangzhu@microsoft.com",
    setup_requires=[
        "pytest-runner",
        "wheel",
    ],
    packages=[
        "dhcp_utilities.common",
        "dhcp_utilities.dhcpservd",
        "dhcp_utilities.dhcprelayd"
    ],
    entry_points={
        "console_scripts": [
            "dhcprelayd = dhcp_utilities.dhcprelayd.dhcprelayd:main",
            "dhcpservd = dhcp_utilities.dhcpservd.dhcpservd:main"
        ]
    },
    classifiers=[
        "Intended Audience :: Developers",
        "Operating System :: Linux",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8"
    ],
    package_data={
        "dhcp_utilities.dhcpservd": ["dhcp_option.csv"]
    }
)
