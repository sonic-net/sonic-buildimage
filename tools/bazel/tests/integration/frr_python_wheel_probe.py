"""Check the FRR Python owners' installed wheels without starting their services."""

from pathlib import Path
import subprocess
import sys

from tools.bazel.tests.integration.installed_wheel_fixture import installed_wheels, write_wheel_receipt
from tools.bazel.tests.integration.wheel_contract import assert_wheel_contract, setup_contract


PROBE = r'''
import importlib
import importlib.metadata
from pathlib import Path
import sys

site, prefix = map(Path, sys.argv[1:3])
name = sys.argv[3]
distribution = importlib.metadata.distribution(name)
assert distribution.version == '1.0'
entries = {entry.name: entry for entry in distribution.entry_points if entry.group == 'console_scripts'}
expected = ({'bgpcfgd', 'staticroutebfd', 'bgpmon', 'bfdmon'} if name == 'sonic-bgpcfgd' else {'frrcfgd'})
assert set(entries) == expected, entries
for entry in entries.values():
    module = importlib.import_module(entry.module)
    assert Path(module.__file__).resolve().is_relative_to(site), module.__file__
    assert callable(entry.load()), entry

if name == 'sonic-bgpcfgd':
    from bgpcfgd.directory import Directory
    from bgpcfgd.managers_bgp import is_bgp_identifier_valid
    directory = Directory()
    notifications = []
    directory.subscribe([('CONFIG_DB', 'BGP_NEIGHBOR', '192.0.2.1')], lambda: notifications.append('changed'))
    directory.put('CONFIG_DB', 'BGP_NEIGHBOR', '192.0.2.1', {'asn': '64512'})
    assert notifications == ['changed']
    assert directory.get('CONFIG_DB', 'BGP_NEIGHBOR', '192.0.2.1') == {'asn': '64512'}
    directory.remove('CONFIG_DB', 'BGP_NEIGHBOR', '192.0.2.1')
    assert not directory.path_exist('CONFIG_DB', 'BGP_NEIGHBOR', '192.0.2.1')
    assert is_bgp_identifier_valid('192.0.2.1')
    assert not is_bgp_identifier_valid('invalid address')
else:
    from frrcfgd.frrcfgd import CachedDataWithOp, extract_cmd_daemons
    from jinja2 import Environment, FileSystemLoader
    assert extract_cmd_daemons('[bgpd,zebra]router bgp 64512') == (['bgpd', 'zebra'], 'router bgp 64512')
    assert extract_cmd_daemons('router bgp 64512') == (None, 'router bgp 64512')
    assert repr(CachedDataWithOp('peer', CachedDataWithOp.OP_UPDATE)) == '(peer, UPDATE)'
    templates = prefix / 'sonic/frrcfgd'
    environment = Environment(loader=FileSystemLoader(templates))
    rendered = environment.get_template('bfdd.conf.j2').render(BFD_PEER_SINGLE_HOP={
        ('192.0.2.1', 'Ethernet0', 'default', '192.0.2.2'): {
            'enabled': 'true', 'desired-minimum-tx-interval': '500',
        },
    })
    assert 'peer 192.0.2.1 interface Ethernet0 vrf default local-address 192.0.2.2' in rendered
    assert 'transmit-interval 500' in rendered and 'no shutdown' in rendered
print(name + ': installed imports, entry points and owner behavior passed')
'''


def main():
    wheel, setup = (Path(value).absolute() for value in sys.argv[1:])
    contract = setup_contract(setup)
    # Keep declared application/third-party dependencies, but never import this
    # component from its checkout or runfiles source directory.
    own_roots = [setup.parent, setup.resolve().parent]
    with installed_wheels([wheel], exclude_source_roots=own_roots) as installed:
        assert_wheel_contract(wheel, setup, contract, installed)
        subprocess.run(
            [str(installed.python), '-I', '-c', PROBE, str(installed.site),
             str(installed.environment), contract['name']],
            cwd=installed.root, env=installed.env, check=True, timeout=60,
        )
    write_wheel_receipt([wheel], required=True, details={
        'component': contract['name'],
        'scope': 'installed owner payload with declared dependencies; no daemon or image acceptance',
    })


if __name__ == '__main__':
    main()
