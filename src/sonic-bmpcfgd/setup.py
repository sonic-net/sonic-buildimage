import setuptools


dependencies = [
    'sonic_py_common',
]

setuptools.setup(
    name = 'sonic-bmpcfgd-services',
    version = '1.0',
    description = 'Python services which run in the bmp container',
    license = 'Apache 2.0',
    author = 'SONiC Team',
    author_email = 'linuxnetdev@microsoft.com',
    url = 'https://github.com/Azure/sonic-buildimage',
    maintainer = 'Feng Pan',
    maintainer_email = 'fenpan@microsoft.com',
    packages=[
        'bmpcfgd',
        'tests'
    ],
    install_requires = [
        'jinja2>=2.10',
        'netaddr==0.8.0',
        'pyyaml==6.0.1',
        'ipaddress==1.0.23'
    ],
    entry_points={
        'console_scripts': [
            'bmpcfgd = bmpcfgd.bmpcfgd:main',
        ]
    },
    setup_requires = [
        'pytest-runner',
        'wheel'
    ],
    tests_require = [
        'parameterized',
        'pytest',
        'pyfakefs',
        'sonic-py-common',
        'pytest-cov'
    ],
)
