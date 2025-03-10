import setuptools

setuptools.setup(
    name = 'sonic-bgpcfgd',
    version = '1.0',
    description = 'Utility to dynamically generate BGP configuration for FRR',
    author = 'Pavel Shirshov',
    author_email = 'pavelsh@microsoft.com',
    url = 'https://github.com/Azure/sonic-buildimage',
    packages = setuptools.find_packages(),
    entry_points = {
        'console_scripts': [
            'bgpcfgd = bgpcfgd.main:main',
            'staticroutebfd = staticroutebfd.main:main',
            'bgpmon = bgpmon.bgpmon:main',
            'bfdmon = bfdmon.bfdmon:main',
        ]
    },
    install_requires = [
        'jinja2>=2.10',
        'netaddr==0.8.0',
        'pyyaml==6.0.1',
        'ipaddress==1.0.23'
    ],
    setup_requires = [
        'pytest-runner',
        'wheel'
    ],
    tests_require = [
        'pytest',
        'pytest-cov',
        'sonic-config-engine'
    ]
)
