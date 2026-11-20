"""Use the target SONiC package manager with labels from the offline image bundle."""

import argparse
import json

from sonic_package_manager import PackageManager
from sonic_package_manager.metadata import MetadataResolver


class BuildMetadataResolver(MetadataResolver):
    """Built-in images have their original configs in the bundle; optional packages use Docker."""

    def __init__(self, manager, labels):
        super().__init__(manager.docker, manager.registry_resolver)
        self.labels = labels

    def from_local(self, image, use_local_manifest=False, name=None, use_edit=False):
        if image in self.labels and not (use_local_manifest or use_edit):
            return self.from_labels(self.labels[image])
        return super().from_local(image, use_local_manifest, name, use_edit)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("labels", type=argparse.FileType("r"))
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("shutdown-order")
    install = commands.add_parser("install")
    source = install.add_mutually_exclusive_group(required=True)
    source.add_argument("--from-tarball")
    source.add_argument("--expression")
    install.add_argument("--enable", action="store_true")
    install.add_argument("--set-owner")
    args = parser.parse_args()
    manager = PackageManager.get_manager()
    manager.metadata_resolver = BuildMetadataResolver(manager, json.load(args.labels))
    if args.command == "shutdown-order":
        manager.service_creator.generate_shutdown_sequence_files(manager.get_installed_packages())
    else:
        options = {"enable": args.enable}
        if args.set_owner:
            options["default_owner"] = args.set_owner
        manager.install(expression=args.expression, tarball=args.from_tarball, **options)


if __name__ == "__main__":
    main()
