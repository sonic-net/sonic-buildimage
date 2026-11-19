from types import SimpleNamespace

import pytest

from tests import test_j2files


MMU_FILES = ("pg_profile_lookup.ini", "qos.json.j2",
             "buffers_defaults_t0.j2", "buffers_defaults_t1.j2")


@pytest.mark.parametrize("lookup_location", ["hwsku", "platform", "symlink"])
def test_mmu_templates_backup_and_restore(tmp_path, lookup_location):
    platform_dir = tmp_path / "platform"
    hwsku_dir = platform_dir / "hwsku"
    backup_dir = tmp_path / "backup"
    hwsku_dir.mkdir(parents=True)
    backup_dir.mkdir()

    platform_lookup = platform_dir / "pg_profile_lookup.ini"
    platform_lookup.write_text("platform headroom\n")
    for name in MMU_FILES[1:]:
        (hwsku_dir / name).write_text("original " + name)
    hwsku_lookup = hwsku_dir / MMU_FILES[0]
    if lookup_location == "hwsku":
        hwsku_lookup.write_text("HWSKU headroom\n")
    elif lookup_location == "symlink":
        hwsku_lookup.symlink_to("../pg_profile_lookup.ini")

    source_lookup = platform_lookup if lookup_location == "platform" else hwsku_lookup
    expected_headroom = source_lookup.read_text()
    helper = SimpleNamespace(test_dir=str(backup_dir))
    copy_templates = test_j2files.TestJ2Files.copy_mmu_templates
    copy_templates(helper, str(hwsku_dir))

    assert (backup_dir / MMU_FILES[0]).read_text() == expected_headroom
    for name in MMU_FILES:
        source = source_lookup if name == MMU_FILES[0] else hwsku_dir / name
        source.write_text("modified during rendering")
    copy_templates(helper, str(hwsku_dir), revert=True)

    assert source_lookup.read_text() == expected_headroom
    for name in MMU_FILES[1:]:
        assert (hwsku_dir / name).read_text() == "original " + name
    assert not list(backup_dir.iterdir())
    if lookup_location == "platform":
        assert not hwsku_lookup.exists()
    elif lookup_location == "hwsku":
        assert platform_lookup.read_text() == "platform headroom\n"
    else:
        assert hwsku_lookup.is_symlink()
