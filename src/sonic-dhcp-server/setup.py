from setuptools import setup

dependencies = [
    "psutil",
    "coverage"
]

test_deps = [
    "pytest"
]

py_modules = [
    "dhcp_server_utils",
    "dhcp_cfggen",
    "dhcp_lease"
]

setup(
    name="sonic-dhcp-server",
    install_requires=dependencies,
    description="Module of SONiC built-in dhcp_server",
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
        "dhcp_server"
    ],
    entry_points={
        "console_scripts": [
            "dhcpservd = dhcp_server.dhcpservd:main"
        ]
    },
    py_modules=py_modules,
    classifiers=[
        "Intended Audience :: Developers",
        "Operating System :: Linux",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8"
    ]
)
