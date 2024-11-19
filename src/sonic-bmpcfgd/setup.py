from __future__ import print_function
import sys
from setuptools import setup
import pkg_resources
from packaging import version

setup(
    name = 'sonic-bmpcfgd-services',
    version = '1.0',
    description = 'Python services which run in the bmp container',
    license = 'Apache 2.0',
    author = 'SONiC Team',
    author_email = 'linuxnetdev@microsoft.com',
    url = 'https://github.com/Azure/sonic-buildimage',
    maintainer = 'Feng Pan',
    maintainer_email = 'fenpan@microsoft.com',
    scripts = [
        'scripts/bmpcfgd'
    ],
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
        'parameterized',
        'pytest',
        'pyfakefs',
        'sonic-py-common',
        'pytest-cov'
    ],
    extras_require = {
        "testing": [
            'parameterized',
            'pytest',
            'pyfakefs',
            'sonic-py-common'
        ]
    },
    classifiers = [
        'Development Status :: 3 - Alpha',
        'Environment :: Console',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3.7',
        'Topic :: System',
    ],
    keywords = 'sonic SONiC bmp services',
    test_suite = 'setup.get_test_suite'
)
