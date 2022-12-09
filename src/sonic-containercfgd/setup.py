from setuptools import setup

dependencies = [
    'sonic_py_common',
]

setup(
    name='sonic-containercfgd',
    version='1.0',
    description='SONiC container config daemon package',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='linuxnetdev@microsoft.com',
    url='https://github.com/Azure/sonic-buildimage',
    maintainer='Junchao Chen',
    maintainer_email='junchaow@nvidia.com',
    install_requires=dependencies,
    entry_points={
        'console_scripts': [
            'containercfgd = containercfgd.containercfgd:main',
        ]
    },
    packages=[
        'containercfgd',
        'tests'
    ],
    setup_requires=[
        'pytest-runner'
    ],
    tests_require=[
        'pytest',
        'mock>=2.0.0'
    ],
    classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: No Input/Output (Daemon)',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3.7',
        'Topic :: System :: Hardware',
    ],
    keywords='SONiC sonic container config daemon',
    test_suite='setup.get_test_suite'
)
