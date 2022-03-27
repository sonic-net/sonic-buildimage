from setuptools import setup
import glob

setup(
    name='clx-utils',
    version='0.1',
    description='Clounix Command-line utilities for SONiC',
    license='Apache 2.0',
    maintainer='Zhiqian.Wu',
    maintainer_email='Zhiqian.Wu@clounixinc.com',
    author='Clounix Team',
    packages=[
        'port_breakout',
    ],
    package_data={
        'port_breakout': ['port_config.clx.j2']
    },
    scripts=[
        'scripts/sfpdet'
    ],
    data_files=[
        ('/etc/bash_completion.d', glob.glob('bash_completion.d/*')),
    ],
    entry_points={
        'console_scripts': [
            'port-breakout=port_breakout.main:cli'
        ]
    },
    keywords='sonic SONiC utilities command line cli CLI clounix'
)
