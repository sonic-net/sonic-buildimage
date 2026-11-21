"""Check installed DHCP utility payload, metadata, entry points and CSV resource."""

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
site, source = map(Path, sys.argv[1:3])
dist = importlib.metadata.distribution('sonic-dhcp-utilities')
assert dist.version == '1.0'
entries = {entry.name: entry for entry in dist.entry_points if entry.group == 'console_scripts'}
assert set(entries) == {'dhcprelayd', 'dhcpservd'}, entries
for entry in entries.values():
    module = importlib.import_module(entry.module)
    assert Path(module.__file__).resolve().is_relative_to(site), module.__file__
    assert callable(entry.load()), entry
resource = 'dhcp_utilities/dhcpservd/dhcp_option.csv'
assert (site / resource).read_bytes() == (source / resource).read_bytes()
from dhcp_utilities.common.utils import validate_str_type
assert validate_str_type('ipv4-address', '192.0.2.1')
assert not validate_str_type('ipv4-address', 'invalid address')
print('Installed DHCP utilities: metadata, entry points, CSV and validation passed')
'''


def main():
    wheel, setup = (Path(value).absolute() for value in sys.argv[1:])
    contract = setup_contract(setup)
    with installed_wheels([wheel], exclude_source_roots=[setup.parent, setup.resolve().parent]) as installed:
        assert_wheel_contract(wheel, setup, contract, installed)
        subprocess.run(
            [str(installed.python), '-I', '-c', PROBE, str(installed.site), str(setup.parent)],
            cwd=installed.root, env=installed.env, check=True, timeout=60,
        )
    write_wheel_receipt([wheel], required=True, details={
        'component': contract['name'],
        'scope': 'installed owner wheel with declared dependencies; no service startup',
    })


if __name__ == '__main__':
    main()
