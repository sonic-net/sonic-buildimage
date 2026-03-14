from setuptools import setup, find_packages

# read me
with open('README.rst') as readme_file:
    readme = readme_file.read()

setup(
    author="lnos-coders",
    author_email='lnos-coders@linkedin.com',
    python_requires='>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*, !=3.4.*',
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Natural Language :: English',
        "Programming Language :: Python :: 2",
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],
    description="Package contains Python Library for YANG for sonic.",
    license="GNU General Public License v3",
    long_description=readme + '\n\n',
    scripts = [
        'sonic-cfg-help',
    ],
    install_requires = [
        'xmltodict==0.12.0',
        'ijson==3.2.3',
        'jsonpointer>=1.9',
        'jsondiff>=1.2.0',
        'tabulate==0.9.0'
    ],
    tests_require = [
        'pytest>3',
        'xmltodict==0.12.0',
        'ijson==3.2.3',
        'jsondiff>=1.2.0'
    ],
    setup_requires = [
        'pytest-runner',
        'wheel'
    ],
    include_package_data=True,
    keywords='sonic-yang-mgmt',
    name='sonic-yang-mgmt',
    py_modules=['sonic_yang', 'sonic_yang_ext', 'sonic_yang_path'],
    packages=find_packages(),
    version='1.0',
    zip_safe=False,
)
