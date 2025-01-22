import setuptools

setuptools.setup(
    name = 'sonic-frr-mgmt-framework',
    version = '1.0',
    description = 'Utility to dynamically configuration FRR based on config DB data update event',
    url = 'https://github.com/Azure/sonic-buildimage',
    packages = setuptools.find_packages(),
    entry_points = {
        'console_scripts': [
            'frrcfgd = frrcfgd.frrcfgd:main',
        ]
    },
    install_requires = [
        'jinja2>=2.10',
        'netaddr==0.8.0',
        'pyyaml==6.0.1',
        'zipp==1.2.0', # importlib-resources needs zipp and seems to have a bug where it will try to import too new of a version for Python 2
    ],
    setup_requires = [
        'pytest-runner',
        'wheel'
    ],
    tests_require = [
        'pytest',
        'pytest-cov',
        'sonic-config-engine'
    ],
    data_files = [('sonic/frrcfgd', ['templates/bgpd/bgpd.conf.j2',
                                     'templates/bgpd/bgpd.conf.db.j2',
                                     'templates/bgpd/bgpd.conf.db.route_map.j2',
                                     'templates/bgpd/bgpd.conf.db.pref_list.j2',
                                     'templates/bgpd/bgpd.conf.db.nbr_or_peer.j2',
                                     'templates/bgpd/bgpd.conf.db.nbr_af.j2',
                                     'templates/bgpd/bgpd.conf.db.comm_list.j2',
                                     'templates/bgpd/bgpd.conf.db.addr_family.j2',
                                     'templates/bgpd/bgpd.conf.db.addr_family.evpn.j2',
                                     'templates/bfdd/bfdd.conf.j2',
                                     'templates/ospfd/ospfd.conf.j2',
                                     'templates/ospfd/ospfd.conf.db.area.j2',
                                     'templates/ospfd/ospfd.conf.db.comm_list.j2',
                                     'templates/ospfd/ospfd.conf.db.distributeroute.j2',
                                     'templates/ospfd/ospfd.conf.db.interface.j2',
                                     'templates/ospfd/ospfd.conf.db.policyrange.j2',
                                     'templates/ospfd/ospfd.conf.db.vlink.j2',
                                     'templates/staticd/staticd.conf.j2',
                                     'templates/staticd/staticd.db.conf.j2',
                                     'templates/staticd/staticd.db.default_route.conf.j2',
                                     'templates/frr/frr.conf.j2'])
    ]
)
