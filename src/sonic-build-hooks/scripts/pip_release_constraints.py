"""Drop the frozen pins a pip install's own requirements rule out.

Usage: pip_release_constraints.py CONSTRAINT_FILE PIP_ARG...

The pip build hook passes the frozen versions-py3 file to every pip install as
a constraint file. A requirement the install itself states wins over the
frozen pin: an exact or ranged requirement on the command line, in a
requirements file (-r), or in a local wheel's Requires-Dist. This removes, from
CONSTRAINT_FILE, the pin of each package whose frozen version a stated
requirement excludes, and the pin of each local wheel's own package. Every
other pin stays. Each dropped pin is reported on stderr.

It runs under the interpreter of the pip being wrapped, and uses that pip's
vendored packaging, so it evaluates versions and markers as pip does.
"""
import os
import re
import sys
import zipfile

try:
    from pip._vendor.packaging.markers import InvalidMarker
    from pip._vendor.packaging.requirements import InvalidRequirement, Requirement
    from pip._vendor.packaging.utils import canonicalize_name
except ImportError:
    from packaging.markers import InvalidMarker
    from packaging.requirements import InvalidRequirement, Requirement
    from packaging.utils import canonicalize_name

# pip install options that take a value, so the value is not a requirement.
VALUE_OPTIONS = {
    '-c', '--constraint', '-r', '--requirement', '-e', '--editable', '-t', '--target',
    '--platform', '--python-version', '--implementation', '--abi', '--root', '--prefix',
    '--src', '--upgrade-strategy', '-C', '--config-settings', '--global-option',
    '--install-option', '--no-binary', '--only-binary', '--progress-bar', '--root-user-action',
    '--report', '-i', '--index-url', '--extra-index-url', '-f', '--find-links',
    '--log', '--log-file', '--proxy', '--retries', '--timeout', '--default-timeout',
    '--exists-action', '--trusted-host', '--cert', '--client-cert', '--cache-dir',
    '--keyring-provider', '--use-feature', '--use-deprecated', '--python', '--group',
}
REQUIREMENT_OPTION = re.compile(r'^(?:-r|--requirement)(?:=|\s+)?(?P<path>\S+)$')
WHEEL_WITH_EXTRAS = re.compile(r'^(?P<path>.+\.whl)(\[(?P<extras>[^\]]*)\])?$')
PIN = re.compile(r'^\s*(?P<name>[A-Za-z0-9][A-Za-z0-9._-]*)\s*==\s*(?P<version>[^\s;#]+)')


def parse_requirement(text):
    try:
        return Requirement(text)
    except (InvalidRequirement, InvalidMarker):
        return None


def marker_applies(req, extras):
    if req.marker is None:
        return True
    for extra in list(extras) or ['']:
        try:
            if req.marker.evaluate({'extra': extra}):
                return True
        except Exception:
            return True
    return False


def requirements_file(path, seen):
    """Requirements in a requirements file, following nested -r."""
    path = os.path.abspath(path)
    if path in seen or not os.path.isfile(path):
        return []
    seen.add(path)
    with open(path) as f:
        text = re.sub(r'\\\n', ' ', f.read())
    reqs = []
    for line in text.splitlines():
        line = re.sub(r'(^|\s)#.*$', '', line).strip()
        if not line:
            continue
        m = REQUIREMENT_OPTION.match(line)
        if m:
            reqs += requirements_file(os.path.join(os.path.dirname(path), m.group('path')), seen)
            continue
        if line.startswith('-'):
            continue
        # Per-requirement options, such as --hash, follow the requirement.
        req = parse_requirement(re.split(r'\s+--?[A-Za-z]', line)[0])
        if req is not None:
            reqs.append(req)
    return reqs


def wheel(path, extras):
    """The wheel's own name and the requirements it installs."""
    with zipfile.ZipFile(path) as zf:
        meta = next(n for n in zf.namelist() if re.match(r'^[^/]+\.dist-info/METADATA$', n))
        text = zf.read(meta).decode('utf-8', 'replace')
    headers = text.split('\n\n', 1)[0]
    m = re.search(r'^Name:\s*(\S+)', headers, re.M)
    name = m.group(1) if m else None
    reqs = []
    for value in re.findall(r'^Requires-Dist:\s*(.+)$', headers, re.M):
        req = parse_requirement(value.strip())
        if req is not None and marker_applies(req, extras):
            reqs.append(req)
    return name, reqs


def stated(args):
    """Package names the install installs itself, and the requirements it states."""
    if 'install' not in args:
        return set(), []
    args = args[args.index('install') + 1:]
    own, reqs, seen = set(), [], set()
    i = 0
    while i < len(args):
        arg = args[i]
        i += 1
        if arg.startswith('-'):
            if arg in ('-r', '--requirement'):
                if i < len(args):
                    reqs += requirements_file(args[i], seen)
                i += 1
            elif REQUIREMENT_OPTION.match(arg):
                reqs += requirements_file(REQUIREMENT_OPTION.match(arg).group('path'), seen)
            elif arg in VALUE_OPTIONS:
                i += 1
            continue
        m = WHEEL_WITH_EXTRAS.match(arg)
        if m and os.path.isfile(m.group('path')):
            extras = [e.strip() for e in (m.group('extras') or '').split(',') if e.strip()]
            try:
                name, wheel_reqs = wheel(m.group('path'), extras)
            except (zipfile.BadZipfile, StopIteration):
                continue
            if name:
                own.add(canonicalize_name(name))
            reqs += wheel_reqs
            continue
        req = parse_requirement(arg)
        if req is not None and (req.specifier or req.url):
            reqs.append(req)
    return own, reqs


def excludes(req, version):
    if req.url:
        return True
    try:
        return not req.specifier.contains(version, prereleases=True)
    except Exception:
        return True


def main():
    constraint_file, args = sys.argv[1], sys.argv[2:]
    own, reqs = stated(args)
    with open(constraint_file) as f:
        lines = f.readlines()
    by_name = {}
    for req in reqs:
        by_name.setdefault(canonicalize_name(req.name), []).append(req)
    kept = []
    for line in lines:
        m = PIN.match(line)
        if m:
            name = canonicalize_name(m.group('name'))
            version = m.group('version')
            if name in own:
                sys.stderr.write('pip build hook: installing %s from a local wheel, not the frozen %s\n'
                                 % (m.group('name'), version))
                continue
            wanted = [str(r) for r in by_name.get(name, []) if excludes(r, version)]
            if wanted:
                sys.stderr.write('pip build hook: the install requires %s, releasing the frozen %s==%s\n'
                                 % (', '.join(wanted), m.group('name'), version))
                continue
        kept.append(line)
    if len(kept) != len(lines):
        with open(constraint_file, 'w') as f:
            f.writelines(kept)


if __name__ == '__main__':
    if sys.version_info[0] >= 3:
        main()
