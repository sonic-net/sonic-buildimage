"""Read actual default VS installer formats and installed product identities."""
import argparse
from debian.deb822 import Deb822
from sonic_apt import dependencies
import gzip
import hashlib
import io
import json
from pathlib import Path
import re
import stat
import subprocess
import tarfile
import tempfile
import zipfile


def digest(stream, algorithm='sha256'):
    value = hashlib.new(algorithm)
    for block in iter(lambda: stream.read(1 << 20), b''): value.update(block)
    return value.hexdigest()


class Region(io.RawIOBase):
    def __init__(self, source, start, size): self.source, self.start, self.size, self.position = source, start, size, 0
    def readable(self): return True
    def seekable(self): return True
    def tell(self): return self.position
    def seek(self, offset, whence=0):
        self.position = offset + (self.position if whence == 1 else self.size if whence == 2 else 0)
        if self.position < 0: raise ValueError('seek before installer payload')
        return self.position
    def read(self, size=-1):
        size = min(self.size - self.position, size) if size >= 0 else self.size - self.position
        self.source.seek(self.start + self.position); result = self.source.read(max(size, 0)); self.position += len(result)
        return result


parser = argparse.ArgumentParser()
for name in ('image', 'squashfs', 'docker', 'kernel', 'containers', 'identity', 'fips', 'version', 'variant', 'unsquashfs', 'expected-host', 'localedef'):
    parser.add_argument('--' + name, required=True)
parser.add_argument('--image-reference', action='append', required=True)
a = parser.parse_args()
kernel, spec, identity = [json.loads(Path(path).read_text()) for path in (a.kernel, a.containers, a.identity)]
version = Path(a.version).read_text().strip(); release = kernel['kernel_release']
selected = {name: value for name, value in spec['containers'].items() if value['installed']}
assert len(selected) == 28, 'first acceptance profile is the mainline default 28-container VS build'
image_repositories = dict(reference.split('=', 1) for reference in a.image_reference)
# Independently reviewed original Make selection: one-image.mk:8-12 uses
# installed debug archives with runtime fallback; docker-dash-engine.mk keeps
# its debug companion outside the installer family.
expected_repositories = {name: name + ('-dbg' if a.variant == 'debug' else '') for name in selected}
expected_repositories['docker-dash-engine'] = 'docker-dash-engine'
assert image_repositories == expected_repositories, 'selected OCI repositories differ from the original Make product profile'
with tarfile.open(a.docker) as store:
    repositories = json.load(store.extractfile('image/overlay2/repositories.json'))['Repositories']
    assert set(repositories) == set(image_repositories.values())
    for name in selected:
        repository = image_repositories[name]
        tags = repositories[repository]
        assert tags[repository + ':latest'] == tags[repository + ':' + version]
        image_id = tags[repository + ':latest'].removeprefix('sha256:')
        configuration = json.load(store.extractfile('image/overlay2/imagedb/content/sha256/' + image_id))
        assert configuration['architecture'] == 'amd64' and configuration['os'] == 'linux'


def cat(path):
    return subprocess.check_output([a.unsquashfs, '-cat', a.squashfs, path], stderr=subprocess.PIPE)

# Decode the actual libc archive with the same selected Trixie tool ABI.
with tempfile.TemporaryDirectory() as temporary:
    locale_archive = Path(temporary) / 'usr/lib/locale/locale-archive'
    locale_archive.parent.mkdir(parents=True)
    locale_archive.write_bytes(cat('usr/lib/locale/locale-archive'))
    locales = subprocess.check_output([a.localedef, '--list-archive', '--prefix=' + temporary], text=True).splitlines()
    assert 'en_US.utf8' in locales
assert b'LANG=en_US.UTF-8' in cat('etc/locale.conf')
assert any(line.split() == ['en_US.UTF-8', 'UTF-8'] for line in cat('etc/locale.gen').decode().splitlines())
locale_link = subprocess.check_output([a.unsquashfs, '-lln', a.squashfs, 'etc/default/locale'])
assert b'-> ../locale.conf' in locale_link

# build_debian.sh:54,111-113 configures fresh-image hostname and loopback names.
assert cat('etc/hostname').decode().strip() == 'sonic'
hosts = [line.split('#', 1)[0].split() for line in cat('etc/hosts').decode().splitlines()]
loopback_names = {name for fields in hosts if fields and fields[0] == '127.0.0.1' for name in fields[1:]}
assert {'sonic', 'localhost'} <= loopback_names

os_release = cat('usr/lib/os-release')
assert b'ID=debian' in os_release and b'VERSION_ID="13"' in os_release
paragraphs = [dict(fields) for fields in Deb822.iter_paragraphs(cat('var/lib/dpkg/status').decode().splitlines())]
status = {fields['Package']: fields for fields in paragraphs}
assert len(status) == len(paragraphs), 'duplicate final Debian package identities'
# Reuse Debian relationship/version parsing over the final installed state.
# This checks required package availability independently of production sources.
dependency_check = dependencies.validate({name: dependencies.package_from_fields(fields, origin='final VS SquashFS ' + name)
                                           for name, fields in status.items()}, architecture='amd64')
assert dependency_check['status'] == 'satisfied'
assert all(fields['Status'] == 'install ok installed' for fields in status.values())
expected_host = json.loads(Path(a.expected_host).read_text())
required = {package for group in expected_host['groups'] for package in group['packages']}
assert required <= set(status), 'missing mainline source packages: ' + ','.join(sorted(required - set(status)))
# Independently fixed original dh_installsystemd units catch missing payloads even
# when a producer's control metadata still reports the package installed.
unit_expectations = expected_host['host_service_units']['files']
unit_listing = subprocess.check_output([a.unsquashfs, '-lln', a.squashfs, *unit_expectations], text=True)
for path, expected in unit_expectations.items():
    assert hashlib.sha256(cat(path)).hexdigest() == expected['sha256'], path + ': original unit bytes differ'
    rows = [line.split() for line in unit_listing.splitlines() if line.split() and line.split()[-1] == 'squashfs-root/' + path]
    assert len(rows) == 1 and rows[0][0] == stat.filemode(stat.S_IFREG | int(expected['mode'], 8)) and rows[0][1] == str(expected['uid']) + '/' + str(expected['gid']), path + ': original unit mode/owner differs'
for package, expected in expected_host['versions'].items():
    assert status[package]['Version'] == expected, package + ': mainline source version differs'
for package in json.loads(Path(a.fips).read_text()):
    assert status[package['package']]['Version'] == package['version'], package['package']
assert status['initramfs-tools']['Version'] == '0.142' and status['initramfs-tools-core']['Version'] == '0.142'
assert 'sonic-platform-vs' not in status, 'platform module remains a lazy installer package'
sonic = dict(line.split(':', 1) for line in cat('etc/sonic/sonic_version.yml').decode().splitlines() if ':' in line)
for key, expected in {'build_version': version, 'kernel_version': release, 'commit_id': identity['commit_id'], 'branch': identity['branch'], 'built_by': identity['built_by']}.items():
    assert sonic[key].strip().strip("'\"") == expected, key
shadow = next(line.split(':') for line in cat('etc/shadow').decode().splitlines() if line.startswith('admin:'))
users = json.loads(cat('etc/sonic/default_users.json'))
assert users['admin']['password'] == shadow[1] and shadow[1].startswith('$y$')
assert int(shadow[2]) == identity['lastchg_day']
packages = json.loads(cat('var/lib/sonic-package-manager/packages.json'))
assert set(packages) == {value['manifest']['package']['name'] for value in selected.values()}
for repository, value in selected.items():
    record = packages[value['manifest']['package']['name']]
    assert record['repository'] == image_repositories[repository]
    assert record['built-in'] == (not value['local_package'])
for name, target in [('vmlinuz', 'boot/vmlinuz-' + release), ('initrd.img', 'boot/initrd.img-' + release)]:
    listing = subprocess.check_output([a.unsquashfs, '-lln', a.squashfs, name])
    assert ('-> ' + target).encode() in listing
assert cat('usr/lib/modules/' + release + '/modules.dep')
assert cat('var/lib/kdump/latest_sysctls-' + release) == cat('etc/kdump/sysctl.conf')
assert int(cat('var/lib/kdump/size_initrd.img-' + release)) > 0

with Path(a.image).open('rb') as source:
    header = bytearray()
    while True:
        line = source.readline(); header.extend(line)
        assert line
        if line == b'exit_marker\n': break
    start = source.tell(); size = int(re.search(rb'^payload_image_size=(\d+)$', header, re.M)[1])
    assert Path(a.image).stat().st_size == start + size
    assert digest(Region(source, start, size), 'sha1') == re.search(rb'^payload_sha1=([0-9a-f]+)$', header, re.M)[1].decode()
    with tarfile.open(fileobj=Region(source, start, size), mode='r:') as outer:
        script = outer.extractfile('installer/install.sh').read()
        assert ('image_version="' + version + '"').encode() in script
        # onie-mk-demo.sh appends sonic_fips=1 only for ENABLE_FIPS=y.
        # The mainline default includes FIPS packages but leaves it disabled.
        assert re.search(rb'^extra_cmdline_linux=$', script, re.M)
        # installer/install.sh:read_conf_file accepts an unterminated final line.
        machine_conf = outer.extractfile('installer/machine.conf').read().decode()
        assert dict(line.split('=', 1) for line in machine_conf.splitlines()) == {
            'machine': 'vs', 'platform': 'x86_64-vs-r0',
        }
        assert 'x86_64-kvm_x86_64-r0' in outer.extractfile('installer/platforms_asic').read().decode().splitlines()
        with zipfile.ZipFile(outer.extractfile('installer/fs.zip')) as payload:
            assert all(not member.flag_bits & 8 for member in payload.infolist())
            assert payload.getinfo('fs.squashfs').compress_type == zipfile.ZIP_STORED
            with payload.open('fs.squashfs') as installed, Path(a.squashfs).open('rb') as expected:
                assert digest(installed) == digest(expected)
            for record in kernel['boot'].values():
                with payload.open(record['path']) as member: assert digest(member) == record['sha256']
            assert payload.getinfo('boot/initrd.img-' + release).file_size > 0
            with gzip.GzipFile(fileobj=payload.open('platform.tar.gz')) as platform, tarfile.open(fileobj=platform, mode='r|') as archive:
                entries = list(archive)
                assert sum(member.isfile() and member.name.endswith('.deb') for member in entries) == 5
                lazy = next(member for member in entries if member.name.endswith('/sonic-platform-vs_1.0_amd64.deb') and member.issym())
                assert lazy.linkname == '../common/sonic-platform-vs_1.0_amd64.deb'
            docker = payload.open('dockerfs.tar.gz') if 'dockerfs.tar.gz' in payload.namelist() else outer.extractfile('installer/dockerfs.tar.gz')
            with gzip.GzipFile(fileobj=docker) as installed, Path(a.docker).open('rb') as expected:
                assert digest(installed) == digest(expected)
print(a.variant + ' VS AMD64 Trixie FIPS DASH installer: 28 containers, source identities, kernel/crash artifacts and nested formats passed')
