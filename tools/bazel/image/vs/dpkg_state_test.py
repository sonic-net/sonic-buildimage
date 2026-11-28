"""Query the generated dpkg database with Debian's real dpkg-query executable."""
from email.parser import Parser
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

state, query, busybox_control, ifup_control = sys.argv[1:]
with tempfile.TemporaryDirectory() as directory, tarfile.open(state) as archive:
    root = Path(directory)
    for member in archive:
        if member.isfile():
            path = root / member.name.removeprefix('./')
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(archive.extractfile(member).read())
    for package, control in [('busybox', busybox_control), ('ifupdown2', ifup_control)]:
        with tarfile.open(control) as controls:
            fields_member = next(member for member in controls if member.name.removeprefix('./') == 'control')
            fields = Parser().parsestr(controls.extractfile(fields_member).read().decode())
            version = subprocess.check_output([query, '--admindir=' + str(root / 'var/lib/dpkg'), '-W', '-f=${Version}|${db:Status-Status}', package], text=True)
            assert version == fields['Version'] + '|unpacked'
            paths = subprocess.check_output([query, '--admindir=' + str(root / 'var/lib/dpkg'), '-L', package], text=True)
            assert '/.\n' in paths
            if package == 'ifupdown2':
                postinst = next(member for member in controls if member.name.removeprefix('./') == 'postinst')
                assert (root / 'var/lib/dpkg/info/ifupdown2.postinst').read_bytes() == controls.extractfile(postinst).read()
                assert '/usr/share/ifupdown2/__main__.py\n' in paths
print('real dpkg-query reports exact source/APT versions, unpacked state and original maintainer scripts')
