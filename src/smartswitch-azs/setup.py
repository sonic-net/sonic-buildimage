from setuptools import setup, find_packages

# read me
with open('README.rst') as readme_file:
    readme = readme_file.read()

setup(
    author="Anf Rotterdam",
    author_email='anishsdirects@microsoft.com',
    python_requires='>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*, !=3.4.*',
    classifiers=[
        'Development Status :: 1 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Natural Language :: English',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],
    description="Package contains Python scripts for smartswitch-azs for SONiC.",
    license="GNU General Public License v3",
        long_description=readme + '\n\n',
    scripts=[
      'link_state_tracker.py',
      'bgp_session_tracker.py'
    ],
    install_requires = [
        'pyroute2',
        'netifaces==0.11.0',
        'sonic-py-common'
    ],
    tests_require = [
        'pytest>3',
        'xmltodict==0.12.0',
        'ijson'
    ],
    extras_require = {
        'testing': [
            'pytest>3',
            'xmltodict==0.12.0',
            'ijson'
        ],
    },
    setup_requires = [
        'pytest-runner',
        'wheel'
    ],
    include_package_data=True,
    keywords=['sonic-smartswitch-azs', 'link_state_tracker'],
    name='sonic-smartswitch-azs',
    py_modules=['link_state_tracker', 'configutil', 'bgp_session_tracker', 'mock_modules'],
    packages=find_packages(),
    version='1.0',
    zip_safe=False,
    entry_points = {
        'console_scripts' : [
            'smartswitch-azs=smartswitch_azs.smartswitch_azs:main',
            'bgp_session_tracker=bgp_session_tracker.bgp_session_tracker:main'
        ]
    }
)