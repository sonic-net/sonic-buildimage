"""Use the target SONiC package manager with labels from the offline image bundle."""

import argparse
from contextlib import ExitStack
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile

from sonic_package_manager import PackageManager
from sonic_package_manager.metadata import MetadataResolver
from sonic_package_manager.source import PackageSource


class BuildMetadataResolver(MetadataResolver):
    """Built-in images have their original configs in the bundle; optional packages use Docker."""

    def __init__(self, manager, labels):
        super().__init__(manager.docker, manager.registry_resolver)
        self.labels = labels

    def from_local(self, image, use_local_manifest=False, name=None, use_edit=False):
        if image in self.labels and not (use_local_manifest or use_edit):
            return self.from_labels(self.labels[image])
        return super().from_local(image, use_local_manifest, name, use_edit)


class LayoutPackageSource(PackageSource):
    """Install a package whose image is already present in the installer bundle."""

    def __init__(self, manager, tag, layout, stack):
        super().__init__(manager.database, manager.docker, manager.metadata_resolver)
        self.tag = tag
        self.layout = layout
        self.image_id = layout.manifest["config"]["digest"]
        self.stack = stack
        self.tree = None

    def get_metadata(self):
        return self.metadata_resolver.from_labels(self.layout.config["config"]["Labels"])

    def install(self, package):
        package.entry.image_id = self.image_id
        package.entry.docker_image_reference = self.tag
        if not package.repository:
            package.entry.repository = self.tag.rsplit(":", 1)[0]

    def uninstall(self, package):
        # Rollback may clear the package entry. The immutable bundle stays intact.
        package.entry.image_id = None

    def extract(self, image_id, source, destination):
        from merge_layers import Inode, REG_TYPES, Tree, apply_layer, split_path

        if image_id != self.image_id:
            raise ValueError(f"Unexpected package image for {source}: {image_id}")
        if self.tree is None:
            self.tree = Tree()
            for path, layer in zip(self.layout.layers, self.layout.manifest["layers"], strict=True):
                if layer["mediaType"].endswith("+zstd"):
                    plain = self.stack.enter_context(tempfile.TemporaryFile())
                    subprocess.run(["zstd", "-dc", str(path)], check=True, stdout=plain)
                    plain.seek(0)
                    archive = tarfile.open(fileobj=plain, mode="r:")
                else:
                    archive = tarfile.open(path, mode="r:*")
                apply_layer(self.tree, self.stack.enter_context(archive), path)
        node = self.tree.lookup(split_path(source))
        if not isinstance(node, Inode) or node.type not in REG_TYPES:
            raise ValueError(f"Missing regular CLI plugin in {self.tag}: {source}")
        archive, member = node.src
        target = Path(destination)
        target.parent.mkdir(parents=True, exist_ok=True)
        with archive.extractfile(member) as plugin, target.open("wb") as out:
            shutil.copyfileobj(plugin, out)
        os.chown(target, node.meta.uid, node.meta.gid)
        os.chmod(target, node.meta.mode)
        os.utime(target, (node.meta.mtime, node.meta.mtime))


def install_layout(manager, layout_path, tag, platform, inventory, options):
    from oci_layout import validate_layout

    layout = validate_layout(Path(layout_path), platform)
    with ExitStack() as stack:
        source = LayoutPackageSource(manager, tag, layout, stack)
        if source.image_id != inventory[tag]:
            raise ValueError(f"Package layout differs from the installed bundle: {tag}")
        # PackageManager installs the services, plugin destinations, feature state and database.
        # Its normal Docker extraction hook would try to create a container on the build host.
        stack.callback(setattr, manager.docker, "extract", manager.docker.extract)
        manager.docker.extract = source.extract
        if manager.is_installed(source.get_package().name):
            manager.upgrade_from_source(source, **options)
        else:
            manager.install_from_source(source, **options)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("labels", type=argparse.FileType("r"))
    parser.add_argument("inventory", type=argparse.FileType("r"))
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("shutdown-order")
    install = commands.add_parser("install")
    source = install.add_mutually_exclusive_group(required=True)
    source.add_argument("--from-tarball")
    source.add_argument("--from-layout", nargs=3, metavar=("LAYOUT", "TAG", "PLATFORM"))
    source.add_argument("--expression")
    install.add_argument("--enable", action="store_true")
    install.add_argument("--set-owner")
    args = parser.parse_args()
    manager = PackageManager.get_manager()
    labels = json.load(args.labels)
    inventory = json.load(args.inventory)
    # The local package database looks up an installed package by image ID.
    labels.update({image_id: labels[tag] for tag, image_id in inventory.items()})
    manager.metadata_resolver = BuildMetadataResolver(manager, labels)
    if args.command == "shutdown-order":
        manager.service_creator.generate_shutdown_sequence_files(manager.get_installed_packages())
    else:
        options = {"enable": args.enable}
        if args.set_owner:
            options["default_owner"] = args.set_owner
        if args.from_layout:
            install_layout(manager, *args.from_layout, inventory, options)
        else:
            manager.install(expression=args.expression, tarball=args.from_tarball, **options)


if __name__ == "__main__":
    # The chroot stages the OCI readers alongside this script, including under PYTHONSAFEPATH.
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    main()
