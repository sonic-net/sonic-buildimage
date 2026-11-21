"""Compare every wheel member with Make's setup.py bdist_wheel output."""

from email.parser import BytesParser
import os
from pathlib import Path
import runpy
import shutil
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile


def main():
    wheel, setup = (Path(arg).absolute() for arg in sys.argv[1:])
    os.umask(0o022)
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        shutil.copyfile(setup, root / "setup.py")
        for package in ("sonic_platform_pddf_base", "sonic_platform_ref", "tests"):
            shutil.copytree(setup.parent / package, root / package)
            for source in (root / package).glob("*.py"):
                source.chmod(0o755 if source.stat().st_mode & 0o111 else 0o644)
        os.chdir(root)
        os.environ["SOURCE_DATE_EPOCH"] = "315532800"
        sys.argv = ["setup.py", "bdist_wheel", "--dist-dir", str(root / "dist")]
        runpy.run_path("setup.py", run_name="__main__")

        with ZipFile(wheel) as actual, ZipFile(root / "dist" / wheel.name) as expected:
            assert set(actual.namelist()) == set(expected.namelist())
            for member in expected.infolist():
                assert actual.read(member.filename) == expected.read(member), member.filename
                info = actual.getinfo(member.filename)
                assert info.date_time == member.date_time, member.filename
                assert info.external_attr == member.external_attr, member.filename
            metadata = BytesParser().parsebytes(actual.read(next(
                name for name in actual.namelist() if name.endswith(".dist-info/METADATA")
            )))
            assert metadata.get_all("Requires-Dist") == ["jsonschema==2.6.0"]
            assert "sonic_platform_pddf_base/pddfapi.py" in actual.namelist()
            assert not any(name.startswith(("sonic_platform_ref/", "tests/")) for name in actual.namelist())


if __name__ == "__main__":
    main()
