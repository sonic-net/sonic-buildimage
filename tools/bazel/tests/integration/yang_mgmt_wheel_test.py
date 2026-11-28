"""Install the wheel and validate real SONiC YANG data outside the source tree."""

from pathlib import Path
import subprocess
import sys
import zipfile

from tools.bazel.tests.integration.installed_wheel_fixture import installed_wheels, write_wheel_receipt
from tools.bazel.tests.integration.wheel_contract import assert_wheel_contract, expected_sources, setup_contract


def main():
    wheel, setup, models = (Path(value).absolute() for value in sys.argv[1:])
    contract = setup_contract(setup)
    with zipfile.ZipFile(wheel) as package:
        assert {name for name in package.namelist() if name.endswith(".py")} == set(
            expected_sources(contract, setup.parent))

    own_roots = [Path(__file__).absolute().parents[4], Path(__file__).resolve().parents[4]]
    with installed_wheels([wheel], exclude_source_roots=own_roots) as installed:
        assert_wheel_contract(wheel, setup, contract, installed)
        root, python, site = installed.root, installed.python, installed.site
        probe = '''
import importlib.metadata, pathlib, sys
import sonic_yang, sonic_yang_ext, sonic_yang_path
site, models = map(pathlib.Path, sys.argv[1:])
for module in (sonic_yang, sonic_yang_ext, sonic_yang_path):
    assert pathlib.Path(module.__file__).resolve().is_relative_to(site), module.__file__
assert importlib.metadata.version('sonic-yang-mgmt') == '1.0'
manager = sonic_yang.SonicYang(str(models), print_log_enabled=False)
assert manager.loadYangModel() is True
assert set(manager.yangFiles) == {path.stem for path in models.glob('*.yang')}
config = {'SYSLOG_SERVER': {'192.0.2.1': {'port': '514', 'protocol': 'udp', 'severity': 'info'}}}
manager.loadData(config, quiet=True)
manager.validate_data_tree()
assert manager.getData()['SYSLOG_SERVER'] == config['SYSLOG_SERVER']
assert not manager.tablesWithOutYang
path = '/SYSLOG_SERVER/192.0.2.1/port'
assert manager.xpath_to_configdb_path(manager.configdb_path_to_xpath(path)) == path
manager.root.free()
manager.root = None
try:
    manager.loadData({'SYSLOG_SERVER': {'192.0.2.1': {'port': '70000'}}}, quiet=True)
    manager.validate_data_tree()
except sonic_yang_ext.SonicYangException:
    pass
else:
    raise AssertionError('Invalid port bypassed installed YANG validation')
print('Installed wheel: production models, ConfigDB roundtrip, path translation and invalid-data rejection passed')
'''
        env = installed.env
        subprocess.run([str(python), "-I", "-c", probe, str(site), str(models)],
                       cwd=root, env=env, check=True, timeout=120)
        script = installed.scripts / "sonic-cfg-help"
        output = subprocess.check_output([str(script), "--help"], cwd=root, env=env, text=True)
        assert "--table" in output and "--field" in output, output
    write_wheel_receipt([wheel], required=True)


if __name__ == "__main__":
    main()
