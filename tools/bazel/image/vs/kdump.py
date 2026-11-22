"""Replay the patched kdump kernel hook's file effects without a target chroot."""
from contextlib import ExitStack
import copy
import io
import json
from pathlib import Path
import re
import sys
import tarfile

from merge_layers import Dir, Inode, Tree, apply_layer, emit


def data(node):
    if isinstance(node.src, bytes): return node.src
    archive, member = node.src
    return archive.extractfile(member).read()


def configuration(spec, output, audit):
    kernel = json.loads(Path(spec['kernel_metadata']).read_text()); release = kernel['kernel_release']
    tree = Tree()
    with ExitStack() as stack:
        for path in spec['layers']:
            archive = stack.enter_context(tarfile.open(path)); apply_layer(tree, archive, path)
        executable = tree.lookup(['usr', 'sbin', 'mkinitramfs'])
        source = tree.lookup(['etc', 'initramfs-tools'])
        boot = tree.lookup(kernel['boot']['vmlinuz']['path'].split('/'))
        if executable is None or not executable.meta.mode & 0o111 or boot is None or not isinstance(source, Dir):
            raise ValueError('kdump kernel hook prerequisites are not satisfied by the selected host')
        init = source.children.get('initramfs.conf')
        if init is None: raise ValueError('kdump kernel hook requires the installed initramfs.conf')
        # The selected mainline SONiC patch removes KDUMP_MODULES=dep. Fail if
        # a future configured host introduces another implicit policy.
        for name, node in source.children.get('conf.d', Dir()).children.items():
            if isinstance(node, Inode) and node.src is not None and re.search(rb'^\s*KDUMP_MODULES=', data(node), re.M):
                raise ValueError('review explicit KDUMP_MODULES policy in ' + name)
        copied = copy.copy(source); copied.children = dict(source.children)
        content = data(init) + b'\nKDUMP=y\n'
        member = tarfile.TarInfo('initramfs.conf'); member.size = len(content)
        modified = Inode(member, content); modified.meta = init.meta
        copied.children['initramfs.conf'] = modified
        if spec.get('modules'):
            original = copied.children.get('modules')
            content = (data(original) if original else b'') + ''.join(name + '\n' for name in spec['modules']).encode()
            member = tarfile.TarInfo('modules'); member.size, member.mode = len(content), 0o644
            copied.children['modules'] = Inode(member, content)
        result = Tree(); parent = result.resolve_dir(['var', 'lib', 'kdump'], create=True)
        # kernel-postinst forcibly recreates this private configuration copy.
        marker = tarfile.TarInfo('opaque'); marker.size = 0
        opaque = Inode(marker, b''); copied.children = dict(copied.children)
        copied.children['.wh..wh..opq'] = opaque
        parent.children['initramfs-tools'] = copied
        with tarfile.open(output, 'w', format=tarfile.PAX_FORMAT) as archive: emit(result, archive, [])
    Path(audit).write_text(json.dumps({'schema': 1, 'kernel_release': release, 'config_dir': '/var/lib/kdump/initramfs-tools',
        'mkinitramfs_executable': True, 'linux_version_boot_image': kernel['boot']['vmlinuz']['path'], 'INITRD': 'Yes',
        'KDUMP': 'y', 'host_loaded_modules': 'not consulted; explicit caller module list', 'extra_modules': spec.get('modules', [])}, indent=2) + '\n')


def installed(spec, output, audit):
    kernel = json.loads(Path(spec['kernel_metadata']).read_text()); release = kernel['kernel_release']
    result = json.loads(Path(spec['initramfs_audit']).read_text())
    if result['config_dir'] != '/var/lib/kdump/initramfs-tools' or result['kernel_release'] != release:
        raise ValueError('kdump requires its own crash initramfs for the selected kernel')
    with tarfile.open(output, 'w', format=tarfile.PAX_FORMAT) as archive:
        image = Path(spec['initramfs']); member = tarfile.TarInfo('var/lib/kdump/initrd.img-' + release)
        member.mode, member.size = 0o644, image.stat().st_size
        with image.open('rb') as source: archive.addfile(member, source)
        for name, target in [('vmlinuz', '/' + kernel['boot']['vmlinuz']['path']), ('initrd.img', '/var/lib/kdump/initrd.img-' + release)]:
            member = tarfile.TarInfo('var/lib/kdump/' + name); member.type, member.mode, member.linkname = tarfile.SYMTYPE, 0o777, target
            archive.addfile(member)
        for name, content in [('latest_sysctls-' + release, Path(spec['sysctl']).read_bytes()),
                              ('size_initrd.img-' + release, (str(result['uncompressed_size_mib']) + '\n').encode())]:
            member = tarfile.TarInfo('var/lib/kdump/' + name); member.mode, member.size = 0o644, len(content)
            archive.addfile(member, io.BytesIO(content))
    Path(audit).write_text(json.dumps({'schema': 1, 'kernel_release': release, 'crash_initramfs': spec['initramfs'],
        'size_mib': result['uncompressed_size_mib'], 'sysctl': spec['sysctl']}, indent=2) + '\n')


if __name__ == '__main__':
    mode, request, output, audit = sys.argv[1:]
    (configuration if mode == 'configuration' else installed)(json.loads(Path(request).read_text()), output, audit)
