"""Installed Common paths inherited by source-built service images."""

DIRECTORY_ALIASES = {
    "bin": "usr/bin",
    "sbin": "usr/sbin",
    "lib": "usr/lib",
    "var/run": "/run",
}

def common_files(architecture):
    triplet = {"amd64": "x86_64-linux-gnu", "arm64": "aarch64-linux-gnu"}[architecture]
    return {
        "usr/lib/" + triplet + "/libswsscommon.so.0.0.0": {"kind": "file", "uid": 0, "gid": 0},
        "run/redis/sonic-db/database_config.json": {"kind": "file", "mode": 0o644, "uid": 0, "gid": 0},
    }

def service_images_policy(image, architecture):
    """Runtime/debug children preserve Common from their config-engine parent."""
    files = common_files(architecture)
    return {
        "architecture": architecture,
        "images": [{
            "name": name,
            "archive": name + ".gz",
            "tag": name + ":latest",
            "parent": parent,
            "files": files,
            "directory_aliases": DIRECTORY_ALIASES,
            "inherit_files": files.keys(),
        } for name, parent in [
            (image, "docker-config-engine-trixie"),
            (image + "-dbg", image),
        ]],
        "symbol_pairs": [{
            "runtime": image,
            "debug": image + "-dbg",
            "paths": [path for path in files if path.endswith(".so.0.0.0")],
        }],
    }
