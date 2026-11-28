"""Compare fresh-image conffile state with Trixie's actual dpkg.

Contract: deb-conffiles(5), including its remove-on-upgrade flag. The oracle
builds and configures a script-free package in a private root as the test user.
"""
from email.parser import Parser
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

from tools.bazel.image.vs.dpkg_state import unpacked


tools = Path(sys.argv[1]).resolve() / 'bin'
with tempfile.TemporaryDirectory() as temporary:
    work = Path(temporary)
    package = work / 'package'
    (package / 'DEBIAN').mkdir(parents=True)
    (package / 'etc').mkdir()
    (package / 'var/run/oracle').mkdir(parents=True)
    (package / 'var/run/oracle/config.json').write_text('{"original": true}\n')
    control = ('Package: sonic-conffiles-oracle\nVersion: 1\nArchitecture: all\n'
               'Maintainer: Test <test@example.com>\nDescription: conffiles oracle\n')
    conffiles = ('remove-on-upgrade /etc/old.conf\n/etc/present.conf\n'
                 '/etc/path with spaces.conf\n/var/run/oracle/config.json\n')
    (package / 'DEBIAN/control').write_text(control)
    (package / 'DEBIAN/conffiles').write_text(conffiles)
    for name in ('present.conf', 'path with spaces.conf'):
        (package / 'etc' / name).write_text('original ' + name + '\n')
    deb = work / 'package.deb'
    subprocess.run([str(tools / 'dpkg-deb'), '-Znone', '--build', str(package), str(deb)], check=True)
    root = work / 'root'
    (root / 'run').mkdir(parents=True)
    (root / 'var').mkdir()
    (root / 'var/run').symlink_to('../run')
    layout = work / 'layout.tar'
    with tarfile.open(layout, 'w') as archive:
        for name in ('run', 'var', 'var/run'):
            archive.add(root / name, arcname=name, recursive=False)
    command = [str(tools / 'dpkg'), '--force-not-root', '--root=' + str(root), '--log=' + str(work / 'dpkg.log')]
    subprocess.run([*command, '--unpack', str(deb)], check=True)
    subprocess.run([*command, '--configure', 'sonic-conffiles-oracle'], check=True)
    expected = Parser().parsestr((root / 'var/lib/dpkg/status').read_text())['Conffiles']
    payload, controls = work / 'data.tar', work / 'control.tar'
    for output, directory in ((payload, package / 'etc'), (controls, package / 'DEBIAN')):
        with tarfile.open(output, 'w') as archive:
            for path in sorted(directory.iterdir()):
                archive.add(path, arcname=('etc/' if output == payload else '') + path.name)
    # Native Common's declared installation TAR uses canonical /run, while
    # its unchanged Debian control retains /var/run. The original DEB above
    # installs through the same directory alias into the private root.
    with tarfile.open(payload, 'a') as archive:
        for path in sorted((package / 'var/run').rglob('*')):
            archive.add(path, arcname='run/' + str(path.relative_to(package / 'var/run')), recursive=False)
    output = work / 'state.tar'
    spec = {'packages': [['sonic-conffiles-oracle', str(payload), str(controls)]], 'layout': str(layout)}
    unpacked(spec, output, work / 'receipt.json')
    with tarfile.open(output) as archive:
        actual = Parser().parsestr(archive.extractfile('./var/lib/dpkg/status').read().decode())['Conffiles']
    assert actual == expected, (actual, expected)
    with tarfile.open(output) as archive:
        assert archive.extractfile('./var/lib/dpkg/info/sonic-conffiles-oracle.conffiles').read() == conffiles.encode()

    # Both the Debian producer and image assembly reject a flagged file that
    # is still shipped. This must not turn a malformed package into a removal.
    (package / 'etc/old.conf').write_text('must be absent\n')
    assert subprocess.run([str(tools / 'dpkg-deb'), '-Znone', '--build', str(package), str(deb)]).returncode != 0
    with tarfile.open(payload, 'a') as archive:
        archive.add(package / 'etc/old.conf', arcname='etc/old.conf')
    try:
        unpacked(spec, output, work / 'receipt.json')
    except ValueError as error:
        assert 'remove-on-upgrade' in str(error), error
    else:
        raise AssertionError('shipped remove-on-upgrade conffile accepted')
print('fresh conffile state matches real dpkg, including removal flags and paths with spaces')
