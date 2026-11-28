"""Shared image naming, boot references and streaming archive codecs."""
from contextlib import contextmanager
import re
import shlex
import subprocess
import tarfile


def image_options(spec):
    options = {key: spec.get(key, default) for key, default in {
        'payload_name': 'fs.zip', 'squashfs_name': 'fs.squashfs',
        'dockerfs_name': 'dockerfs.tar.gz', 'docker_directory': 'docker',
        'docker_mode': 'disk',
    }.items()}
    for key in ('payload_name', 'squashfs_name', 'dockerfs_name', 'docker_directory'):
        value = options[key]
        if not re.fullmatch(r'[A-Za-z0-9_][A-Za-z0-9_+.-]*', value) or value in ('.', '..'):
            raise ValueError('image name must be a shell-safe basename: ' + key)
    members = [options[k] for k in ('squashfs_name', 'dockerfs_name', 'docker_directory')]
    if len(set(members)) != len(members) or set(members) & {'boot', 'platform', 'platform.tar.gz'}:
        raise ValueError('conflicting image member names')
    if options['docker_mode'] not in ('disk', 'ram'):
        raise ValueError('unsupported docker_mode')
    return options


def installer_config(text, options):
    values = {name: options[key] for name, key in {
        'INSTALLER_PAYLOAD': 'payload_name', 'FILESYSTEM_SQUASHFS': 'squashfs_name',
        'FILESYSTEM_DOCKERFS': 'dockerfs_name', 'DOCKERFS_DIR': 'docker_directory',
    }.items()}
    values['docker_inram'] = 'on' if options['docker_mode'] == 'ram' else 'off'
    return text + '\n# Image assembly attributes are authoritative.\n' + ''.join(
        key + '=' + shlex.quote(value) + '\n' for key, value in values.items())


def boot_config(text, kernel, options, version=None, root=None):
    """Bind legacy installer boot references and explicit GRUB placeholders."""
    replacements = {'KERNEL_RELEASE': kernel['kernel_release'], 'SQUASHFS_NAME': options['squashfs_name'],
                    'DOCKERFS_NAME': options['dockerfs_name']}
    if version is not None:
        replacements['IMAGE_VERSION'] = version
    if root is not None:
        replacements['SONIC_ROOT'] = root
    for key, value in replacements.items():
        text = text.replace('%%' + key + '%%', value)
    if version is not None:
        image = 'image-' + version
        text = re.sub(r'/image-[^/\s\"\']+/boot/', '/' + image + '/boot/', text)
        text = re.sub(r'\bloop=image-[^/\s]+/[^\s\"\']+', 'loop=' + image + '/' + options['squashfs_name'], text)
        if root is not None:
            lines = []
            for line in text.splitlines(keepends=True):
                if re.match(r'\s*(?:linux|linuxefi)\s+/', line) and '/' + image + '/boot/vmlinuz-' in line:
                    line = re.sub(r'\broot=[^\s\"\']+', lambda m: 'root=' + root, line)
                lines.append(line)
            text = ''.join(lines)
    # default_platform.conf historically embeds the build's kernel release.
    # Change only the kernel/initrd path component, preserving its shell context.
    return re.sub(r'(/boot/)(vmlinuz|initrd\.img)-[^\s\"\'\\;]+',
                  lambda m: m[1] + m[2] + '-' + kernel['kernel_release'], text)


@contextmanager
def compressed_output(path, compression, zstd=None, pigz=None, gzip_level=None):
    if compression not in ('none', 'gzip', 'zstd'):
        raise ValueError('unsupported archive compression: ' + compression)
    tool = pigz if compression == 'gzip' else zstd
    if compression != 'none' and not tool:
        raise ValueError(compression + ' output requires a declared ' + ('pigz' if compression == 'gzip' else 'zstd') + ' executable')
    with open(path, 'wb') as raw:
        if compression == 'none':
            yield raw
            return
        command = [tool, '-n', '-c'] if compression == 'gzip' else [tool, '-q', '-c']
        if compression == 'gzip' and gzip_level is not None:
            command.append(f'-{gzip_level}')
        process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=raw)
        try:
            yield process.stdin
        finally:
            try:
                process.stdin.close()
            except BrokenPipeError:
                pass
            status = process.wait()
            if status:
                raise ValueError(compression + ' compression failed')


@contextmanager
def archive_input(path, zstd=None):
    with open(path, 'rb') as raw:
        magic = raw.read(4)
        raw.seek(0)
        if magic == b'\x28\xb5\x2f\xfd':
            if not zstd:
                raise ValueError('zstd input requires a declared zstd executable')
            process = subprocess.Popen([zstd, '-q', '-d', '-c', str(path)], stdout=subprocess.PIPE)
            try:
                with tarfile.open(fileobj=process.stdout, mode='r|') as archive:
                    yield archive
                # Check the complete compressed stream, including its checksum.
                while process.stdout.read(1024 * 1024):
                    pass
            finally:
                process.stdout.close()
                status = process.wait()
            if status:
                raise ValueError('zstd decompression failed')
        else:
            with tarfile.open(fileobj=raw, mode='r|*') as archive:
                yield archive
