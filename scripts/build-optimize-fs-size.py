#!/usr/bin/env python3

import argparse
import hashlib
import os
import shutil
import subprocess
import sys

from collections import defaultdict
from functools import cached_property

DRY_RUN = False
def enable_dry_run(enabled):
    global DRY_RUN # pylint: disable=global-statement
    DRY_RUN = enabled

class File:
    def __init__(self, path):
        self.path = path

    def __str__(self):
        return self.path

    def rmtree(self):
        if DRY_RUN:
           print(f'rmtree {self.path}')
           return
        shutil.rmtree(self.path)

    def hardlink(self, src):
        if DRY_RUN:
           print(f'hardlink {self.path} {src}')
           return
        st = self.stats
        os.remove(self.path)
        os.link(src.path, self.path)
        os.chmod(self.path, st.st_mode)
        os.chown(self.path, st.st_uid, st.st_gid)
        os.utime(self.path, times=(st.st_atime, st.st_mtime))

    @property
    def name(self):
        return os.path.basename(self.path)

    @cached_property
    def stats(self):
        return os.stat(self.path)

    @cached_property
    def size(self):
        return self.stats.st_size

    @cached_property
    def checksum(self):
        with open(self.path, 'rb') as f:
            return hashlib.md5(f.read()).hexdigest()

class FileManager:
    def __init__(self, path):
        self.path = path
        self.files = []
        self.folders = []
        self.nindex = defaultdict(list)
        self.cindex = defaultdict(list)

    def add_file(self, path):
        if not os.path.isfile(path) or os.path.islink(path):
            return
        f = File(path)
        self.files.append(f)

    def load_tree(self):
        self.files = []
        self.folders = []
        for root, _, files in os.walk(self.path):
            self.folders.append(File(root))
            for f in files:
                self.add_file(os.path.join(root, f))
        print(f'loaded {len(self.files)} files and {len(self.folders)} folders')

    def generate_index(self):
        print('Computing file hashes')
        for f in self.files:
            self.nindex[f.name].append(f)
            self.cindex[(f.name, f.checksum)].append(f)

    def create_hardlinks(self):
        print('Creating hard links')
        for files in self.cindex.values():
            if len(files) <= 1:
                continue
            orig = files[0]
            for f in files[1:]:
                f.hardlink(orig)

class FsRoot:
    def __init__(self, path):
        self.path = path

    def iter_fsroots(self):
        yield self.path
        dimgpath = os.path.join(self.path, 'var/lib/docker/overlay2')
        for layer in os.listdir(dimgpath):
            yield os.path.join(dimgpath, layer, 'diff')

    def collect_fsroot_size(self):
        cmd = ['du', '-sb', self.path]
        p = subprocess.run(cmd, text=True, check=False,
                           stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        return int(p.stdout.split()[0])

    def _remove_root_paths(self, relpaths):
        for root in self.iter_fsroots():
            for relpath in relpaths:
                path = os.path.join(root, relpath)
                if os.path.isdir(path):
                    if DRY_RUN:
                        print(f'rmtree {path}')
                    else:
                        shutil.rmtree(path)

    def remove_docs(self):
        self._remove_root_paths([
            'usr/share/doc',
            'usr/share/doc-base',
            'usr/local/share/doc',
            'usr/local/share/doc-base',
        ])

    def remove_mans(self):
        self._remove_root_paths([
            'usr/share/man',
            'usr/local/share/man',
        ])

    def remove_licenses(self):
        self._remove_root_paths([
            'usr/share/common-licenses',
        ])

    def hardlink_under(self, path):
        fm = FileManager(os.path.join(self.path, path))
        fm.load_tree()
        fm.generate_index()
        fm.create_hardlinks()

    def remove_platforms(self, filter_func):
        devpath = os.path.join(self.path, 'usr/share/sonic/device')
        for platform in os.listdir(devpath):
            if not filter_func(platform):
                path = os.path.join(devpath, platform)
                if DRY_RUN:
                    print(f'rmtree platform {path}')
                else:
                    shutil.rmtree(path)

    def remove_modules(self, modules):
        modpath = os.path.join(self.path, 'lib/modules')
        kversion = os.listdir(modpath)[0]
        kmodpath = os.path.join(modpath, kversion)
        for module in modules:
            path = os.path.join(kmodpath, module)
            if os.path.isdir(path):
                if DRY_RUN:
                    print(f'rmtree module {path}')
                else:
                    shutil.rmtree(path)

    def remove_firmwares(self, firmwares):
        fwpath = os.path.join(self.path, 'lib/firmware')
        for fw in firmwares:
            path = os.path.join(fwpath, fw)
            if os.path.isdir(path):
                if DRY_RUN:
                    print(f'rmtree firmware {path}')
                else:
                    shutil.rmtree(path)


    def specialize_aboot_image(self):
        fp = lambda p: '-' not in p or 'arista' in p or 'common' in p
        self.remove_platforms(fp)
        self.remove_modules([
           'kernel/drivers/gpu',
           'kernel/drivers/infiniband',
        ])
        self.remove_firmwares([
           'amdgpu',
           'i915',
           'mediatek',
           'nvidia',
           'radeon',
        ])

    def specialize_image(self, image_type):
        if image_type == 'aboot':
           self.specialize_aboot_image()

def parse_args(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('fsroot',
        help="path to the fsroot build folder")
    parser.add_argument('-s', '--stats', action='store_true',
        help="show space statistics")
    parser.add_argument('--hardlinks', action='append',
        help="path where similar files need to be hardlinked")
    parser.add_argument('--remove-docs', action='store_true',
        help="remove documentation")
    parser.add_argument('--remove-licenses', action='store_true',
        help="remove license files")
    parser.add_argument('--remove-mans', action='store_true',
        help="remove manpages")
    parser.add_argument('--image-type', default=None,
        help="type of image being built")
    parser.add_argument('--dry-run', action='store_true',
        help="only display what would happen")
    return parser.parse_args(args)

def main(args):
    args = parse_args(args)

    enable_dry_run(args.dry_run)

    fs = FsRoot(args.fsroot)
    if args.stats:
        begin = fs.collect_fsroot_size()
        print(f'fsroot size is {begin} bytes')

    if args.remove_docs:
        fs.remove_docs()

    if args.remove_mans:
        fs.remove_mans()

    if args.remove_licenses:
        fs.remove_licenses()

    if args.image_type:
        fs.specialize_image(args.image_type)

    for path in args.hardlinks:
        fs.hardlink_under(path)

    if args.stats:
        end = fs.collect_fsroot_size()
        pct = 100 - end / begin * 100
        print(f'fsroot reduced to {end} from {begin} {pct:.2f}')

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
