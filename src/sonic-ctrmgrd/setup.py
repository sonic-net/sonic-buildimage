from setuptools import setup, find_packages

setup_requirements = ['pytest-runner']

test_requirements = ['pytest>=3']

# read me
with open('README.rst') as readme_file:
    readme = readme_file.read()

setup(
    author="sonic-dev",
    author_email='remanava@microsoft.com',
    python_requires='>=3.6',
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Natural Language :: English',
        'Programming Language :: Python :: 3.6',
    ],
    description="Package contains remote container mgmt modules",
    url='https://github.com/Azure/sonic-buildimage',
    tests_require=[
        'pytest',
        'pytest-cov',
        'sonic-py-common',
    ],
    install_requires=['netaddr==0.8.0', 'pyyaml', 'docker'],
    license="GNU General Public License v3",
    long_description=readme + '\n\n',
    include_package_data=True,
    name='sonic_ctrmgrd',
    py_modules=[],
    packages=find_packages(),
    setup_requires=setup_requirements,
    version='1.0.0',
    scripts=['ctrmgr/container', 'ctrmgr/ctrmgr_tools.py', 'ctrmgr/kube_commands.py', 'ctrmgr/ctrmgrd.py', 'ctrmgr/docker-wait-any'],
    zip_safe=False,
)
