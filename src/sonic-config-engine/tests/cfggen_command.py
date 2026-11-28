"""Run the cfggen script with declared YANG, database, and device fixtures."""

from functools import partial
from pathlib import Path
import runpy
import shutil
import sys
from unittest.mock import patch

import minigraph
import sonic_yang_cfg_generator
from swsscommon import swsscommon


def select_mmu_with_fixtures(device_root, profile, platform, hwsku):
    """Run the unchanged MMU selector against the writable test device tree."""
    walk = minigraph.os.walk
    exec_cmd = minigraph.exec_cmd

    def walk_devices(path, *args, **kwargs):
        return walk(device_root if path == "/sonic/device" else path, *args, **kwargs)

    def copy_fixture(command):
        if len(command) == 4 and command[:2] == ["sudo", "cp"]:
            shutil.copyfile(command[2], command[3])
        else:
            return exec_cmd(command)

    with patch.object(minigraph.os, "walk", walk_devices), \
            patch.object(minigraph, "exec_cmd", copy_fixture):
        return select_mmu_profiles(profile, platform, hwsku)


select_mmu_profiles = minigraph.select_mmu_profiles


if __name__ == "__main__":
    models, script = sys.argv[1:3]
    # The existing mock connectors load these same database/namespace fixtures
    # through swsssdk. Initialize the real native registry from their declared
    # paths as well, instead of requiring an installed image's /var/run files.
    swsscommon.SonicDBConfig.initializeGlobalConfig(str(
        Path(__file__).parent / "mock_tables" / "database_global.json"
    ))
    minigraph.select_mmu_profiles = partial(
        select_mmu_with_fixtures,
        str(Path(script).resolve().parents[2] / "device"),
    )
    # Keep the real parser and validation; supply the same constructor argument
    # that an installed image obtains from /usr/local/yang-models.
    sonic_yang_cfg_generator.SonicYangCfgDbGenerator = partial(
        sonic_yang_cfg_generator.SonicYangCfgDbGenerator,
        yang_models_dir=models,
    )
    sys.argv = [script] + sys.argv[3:]
    runpy.run_path(script, run_name="__main__")
