#!/usr/bin/env python3
"""Generate a minimal standalone Dockerfile render context from Make rules."""

import json
import os
import subprocess
from pathlib import Path


def resolve(variables, arch):
    makefile = (
        f"CONFIGURED_ARCH := {arch}\n"
        "BLDENV := trixie\n"
        "include rules/libnl3.mk\n"
        "include rules/sonic-fips.mk\n"
        + "\n".join(f"$(info {name}=$({name}))" for name in variables)
        + "\nall:;@:\n"
    )
    result = subprocess.run(
        ["make", "-f", "-", "--no-print-directory", "all"],
        input=makefile, capture_output=True, text=True, check=True,
    )
    values = dict(line.split("=", 1) for line in result.stdout.splitlines())
    if any(not values.get(name) or "$(" in values[name] for name in variables):
        raise RuntimeError(f"Unresolved Dockerfile variables: {values}")
    return values


def main():
    arch = os.environ.get("CONFIGURED_ARCH", "amd64")
    arch_map = {"amd64": "amd64", "arm64": "arm64v8", "armhf": "arm32v7"}
    ctx = {
        "DOCKER_USERNAME": "sonic",
        "DOCKER_USERTAG": "latest",
        "CONFIGURED_ARCH": arch,
        "CONFIGURED_PLATFORM": "",
        "DEFAULT_CONTAINER_REGISTRY": "",
        "DOCKER_BASE_ARCH": arch_map.get(arch, arch),
        "DOCKER_EXTRA_OPTS": "",
        "ENABLE_ASAN": "n",
    }
    # VS Dockerfiles reference LIBNL3_VERSION_SONIC; FIPS Dockerfiles use
    # FIPS_GOLANG_MAIN_VERSION. Both come from Make, not hardcoded versions.
    ctx.update(resolve(("FIPS_GOLANG_MAIN_VERSION", "LIBNL3_VERSION_SONIC"), arch))
    Path(".j2_context.json").write_text(json.dumps(ctx, indent=2), encoding="utf-8")
    print(json.dumps(ctx, indent=2))


if __name__ == "__main__":
    main()
