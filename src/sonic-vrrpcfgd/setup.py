import setuptools

setuptools.setup(
    name = 'sonic-vrrpcfgd',
    version = '1.0',
    description = 'Utility to dynamically generate VRRP configuration for FRR',
    url = 'https://github.com/Azure/sonic-buildimage',
    packages = setuptools.find_packages(),
    entry_points = {
        'console_scripts': [
            'vrrpcfgd = vrrpcfgd.vrrpcfgd:main',
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
