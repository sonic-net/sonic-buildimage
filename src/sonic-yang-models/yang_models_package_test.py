"""Verify the installed models keep the Python distribution's complete contract."""

from email.parser import BytesParser
from importlib import metadata
from pathlib import Path
import sys
import tarfile
import tempfile
import zipfile


def main():
    package, wheel_path, models = map(Path, sys.argv[1:])
    with zipfile.ZipFile(wheel_path) as wheel, tarfile.open(package) as archive:
        metadata_paths = [n for n in wheel.namelist() if n.endswith('.dist-info/METADATA')]
        assert len(metadata_paths) == 1, metadata_paths
        distribution = BytesParser().parsebytes(wheel.read(metadata_paths[0]))
        assert distribution['Name'] == 'sonic-yang-models'
        assert distribution['Version'] == '1.0'
        members = {m.name.lstrip('./'): m for m in archive.getmembers() if m.isfile()}
        prefix = 'usr/local/lib/python3.13/dist-packages/'
        installed_metadata = prefix + metadata_paths[0]
        assert installed_metadata in members, 'Installed Python distribution metadata is missing'
        assert archive.extractfile(members[installed_metadata]).read() == wheel.read(metadata_paths[0])
        model_names = {p.name for p in models.glob('*.yang')}
        assert len(model_names) > 100, 'Production model manifest is incomplete'
        for directory in ['yang-models', 'cvlyang-models']:
            wheel_prefix = 'sonic_yang_models-1.0.data/data/' + directory + '/'
            expected = {n[len(wheel_prefix):] for n in wheel.namelist() if n.startswith(wheel_prefix)}
            installed_prefix = 'usr/local/' + directory + '/'
            actual = {n[len(installed_prefix):] for n in members if n.startswith(installed_prefix)}
            assert expected == actual == model_names, (directory, expected ^ actual)
            for name in expected:
                payload = archive.extractfile(members[installed_prefix + name]).read()
                assert payload == wheel.read(wheel_prefix + name), name
                if directory == 'yang-models':
                    assert payload == (models / name).read_bytes(), name
        for m in members.values():
            assert m.uid == m.gid == 0, (m.name, m.uid, m.gid)
        # This is the discovery path pip uses for an already-installed dependency.
        # Data-only archives passed the old test but cannot satisfy this lookup.
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name, member in members.items():
                if name.startswith(prefix) and '.dist-info/' in name:
                    destination = root / name[len(prefix):]
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    destination.write_bytes(archive.extractfile(member).read())
            distributions = list(metadata.distributions(path=[str(root)]))
            assert [(d.metadata['Name'], d.version) for d in distributions] == [('sonic-yang-models', '1.0')]
    print('Verified source wheel, Python distribution metadata, both model variants and installed ownership')


if __name__ == '__main__':
    main()
