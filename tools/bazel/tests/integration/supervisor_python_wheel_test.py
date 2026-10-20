"""Install the source wheel and exercise the supervisor listener's public API."""

from pathlib import Path
import subprocess
import sys
import zipfile

from tools.bazel.tests.integration.installed_wheel_fixture import installed_wheels, write_wheel_receipt
from tools.bazel.tests.integration.wheel_contract import assert_wheel_contract, setup_contract


def main():
    wheel, setup, common = (Path(value).absolute() for value in sys.argv[1:])
    contract = setup_contract(setup)
    assert not contract["packages"]
    assert contract["scripts"] == ["scripts/supervisor-proc-exit-listener"]
    with zipfile.ZipFile(wheel) as package:
        assert not any(name.endswith(".py") for name in package.namelist())

    own_roots = [Path(__file__).absolute().parents[4], Path(__file__).resolve().parents[4]]
    with installed_wheels((common, wheel), exclude_source_roots=own_roots) as installed:
        assert_wheel_contract(wheel, setup, contract, installed)
        root, python, site = installed.root, installed.python, installed.site
        executable = installed.scripts / "supervisor-proc-exit-listener"
        fixture = root / "critical-processes"
        fixture.write_text("group:services\nprogram:syncd\n")
        probe = """
import importlib.metadata, pathlib, runpy, sys
from sonic_py_common import daemon_base
site = pathlib.Path(sys.argv[1])
assert pathlib.Path(daemon_base.__file__).resolve().is_relative_to(site)
distribution = importlib.metadata.distribution('sonic-supervisord-utilities')
assert distribution.version == sys.argv[4]
namespace = runpy.run_path(sys.argv[2])
assert namespace['get_group_and_process_list'](sys.argv[3]) == (['services'], ['syncd'])
try:
    namespace['main']([])
except SystemExit as result:
    assert result.code == 1, result.code
else:
    raise AssertionError('The installed listener accepted a missing container name')
assert importlib.metadata.version('supervisor') == '4.3.0'
print('Installed listener, native imports and critical-process parsing passed')
"""
        env = installed.env
        subprocess.run([str(python), "-I", "-c", probe, str(site), str(executable),
                        str(fixture), contract["version"]], cwd=root, env=env, check=True, timeout=30)
        command = subprocess.run([str(executable)], cwd=root, env=env, capture_output=True, text=True, timeout=30)
        assert command.returncode == 1 and not command.stderr, command
    write_wheel_receipt([wheel])


if __name__ == "__main__":
    main()
