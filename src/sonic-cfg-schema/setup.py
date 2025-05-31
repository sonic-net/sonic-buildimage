from setuptools import setup

# read me
with open('README.md') as readme_file:
    readme = readme_file.read()

setup(
    name='sonic-cfg-schema',
    version='1.0.0',
    description="SONiC Config Schema Package",
    long_description=readme + '\n\n',
    author="SONiC Team",
    author_email='sonicproject@googlegroups.com',
    url='https://github.com/sonic-net/sonic-buildimage',
    py_modules=['cfg_schema'],
    data_files=[('', ['cfg_schema.h'])],
    include_package_data=True,
    license="Apache Software License 2.0",
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
    ],
    keywords='sonic-cfg-schema',
    install_requires=[
        'sonic-yang-mgmt',
        'sonic-yang-models',
    ],
    zip_safe=False,
)
