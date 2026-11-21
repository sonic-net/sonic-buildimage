"""Validate an image consumer's inputs from the VS build without invoking Bazel or Make."""

from contextlib import contextmanager
import gzip
import json
from pathlib import Path
import re
import subprocess
import uuid

from tools.bazel.ci import validate_runtime
from tools.bazel.ci.artifact_validation import require, sha
from tools.bazel.ci.vs_archive_reuse import verify_execution_settings
from tools.bazel.oci.image_validation import docker_archive


# These are the installed service entrypoints from the final Make Dockerfile stages.
# Probes override --entrypoint to inspect the image and cannot check its normal startup this way.
SUPERVISORD = "/usr/local/bin/supervisord"
PMON = "docker-platform-monitor"
ENTRYPOINTS = {
    "docker-base-trixie": None, "docker-config-engine-trixie": None,
    "docker-dash-engine": "/usr/bin/start.sh", "docker-dash-ha": SUPERVISORD,
    "docker-database": "/usr/local/bin/docker-database-init.sh", "docker-eventd": SUPERVISORD,
    "docker-fpm-frr": "/usr/bin/docker_init.sh", "docker-gbsyncd-vs": SUPERVISORD,
    "docker-lldp": "/usr/bin/docker-lldp-init.sh", "docker-macsec": SUPERVISORD,
    "docker-mux": "/usr/bin/docker-init.sh", "docker-nat": SUPERVISORD,
    "docker-orchagent": "/usr/bin/docker-init.sh", PMON: "/usr/bin/docker_init.sh",
    "docker-router-advertiser": "/usr/bin/docker-init.sh",
    "docker-sflow": SUPERVISORD, "docker-snmp": "/usr/bin/docker-snmp-init.sh",
    "docker-sonic-bmp": SUPERVISORD, "docker-sonic-gnmi": SUPERVISORD,
    "docker-sonic-mgmt-framework": SUPERVISORD, "docker-sonic-otel": SUPERVISORD,
    "docker-sonic-p4rt": SUPERVISORD, "docker-swss-layer-trixie": SUPERVISORD,
    "docker-syncd-vs": SUPERVISORD, "docker-sysmgr": SUPERVISORD,
    "docker-teamd": "/usr/bin/docker-teamd-init.sh",
    "docker-gnmi-sidecar": SUPERVISORD, "docker-restapi-sidecar": SUPERVISORD,
    "docker-gnmi-watchdog": SUPERVISORD, "docker-bmp-watchdog": SUPERVISORD,
}

# These services check their inherited empty ExposedPorts on master. SNMP's
# EXPOSE is in the build stage, and its final stage inherits from Config Engine.
INHERITED_PORTS = {
    "docker-dash-ha", "docker-fpm-frr", "docker-macsec", "docker-sflow", "docker-snmp",
    "docker-sonic-otel", "docker-sysmgr", "docker-teamd",
}


def static_i386_files(image):
    if image != PMON:
        return {}
    vendor = Path(__file__).resolve().parents[3] / "dockers" / PMON / "ssd_tools/SmartCmd"
    return {"usr/bin/SmartCmd": sha(vendor)}


def verify_amd64_files(image, files):
    """PMON additionally ships the unmodified, statically linked i386 SmartCmd."""
    exceptions = static_i386_files(image)
    for path, digest in exceptions.items():
        installed = files.get(path, {})
        require(installed.get("sha256") == digest and installed.get("elf_machine") == 3
                and installed.get("elf_type") == 2 and installed.get("kind") == "file",
                "Missing or changed PMON compatibility executable: " + path)
    for path, item in files.items():
        if item.get("elf_type") in (2, 3) and path not in exceptions:
            require(item.get("elf_machine") == 62, image + " contains a non-AMD64 executable or shared library: " + path)


def verify_startup(image, settings):
    """Check the configured default independently of the commands used by smoke tests."""
    if image not in ENTRYPOINTS:
        return  # New families still receive platform, release, layer, tag and checksum checks.
    entrypoint = ENTRYPOINTS[image]
    if entrypoint is None:
        require(not settings.get("Entrypoint") and not settings.get("Cmd"), "Unexpected base image startup command")
    else:
        verify_execution_settings(settings, [entrypoint], allow_empty_cmd=True)
    if image in INHERITED_PORTS:
        require(not settings.get("ExposedPorts"), "Unexpected exposed ports in " + image)


def parent_with_make_settings(directory, variant, parent_config, fixtures):
    """Overlay the Make-declared key/value inputs before checking other inherited OCI settings."""
    config = dict(parent_config)
    for filename, key in (("components.labels", "Labels"), ("image.env", "Env")):
        name = variant + ".make." + filename
        if name not in fixtures:
            continue
        declared = dict(line.split("=", 1) for line in (directory / name).read_text().splitlines())
        if key == "Labels":
            config[key] = {**config.get(key, {}), **declared}
        else:
            inherited = dict(row.split("=", 1) for row in config.get(key, []))
            config[key] = [name + "=" + value for name, value in (inherited | declared).items()]
    return config


def read_oci(directory, variant):
    """Read the file inventory and config taken from the producer's OCI layout."""
    with gzip.open(directory / (variant + ".oci.json.gz"), "rt", encoding="utf-8") as stream:
        return json.load(stream)


def bundle_file(directory, name):
    """Resolve a regular file published at the root of this family's artifact."""
    require(isinstance(name, str) and name and Path(name).name == name and name not in (".", ".."),
            "Invalid VS artifact filename: " + repr(name))
    path = directory / name
    require(path.is_file() and not path.is_symlink(), "Missing VS artifact file: " + name)
    return path


def validate_artifacts(directory, image, revision, debug_enabled, build_version):
    """Bind every selected archive and probe input to the checkout and producer's layout."""
    bundle = json.loads(bundle_file(directory, "bundle.json").read_bytes())
    require(bundle["image"] == image and bundle["revision"] == revision,
            "VS artifact does not match the selected container and checkout")
    require(bundle["architecture"] == "amd64", "VS artifact has the wrong architecture")
    require(bundle["debug_enabled"] is debug_enabled and isinstance(bundle["supports_debug"], bool),
            "VS artifact does not match the requested debug setting")
    require(isinstance(build_version, str) and build_version and bundle["build_version"] == build_version,
            "VS artifact has the wrong VS release")
    variants = ["runtime"]
    if debug_enabled and bundle["supports_debug"]:
        variants.append("debug")
    archives = bundle["archives"]
    require(set(archives) == set(variants), "VS artifact does not contain exactly the selected variants")

    fixtures = bundle["fixtures"]
    for name, digest in fixtures.items():
        require(sha(bundle_file(directory, name)) == digest, "VS fixture checksum differs: " + name)

    for variant in variants:
        spec = archives[variant]
        name = image if variant == "runtime" else image + "-dbg"
        require(spec["file"] == name + ".gz", "VS archive has the wrong filename for " + name)
        require(variant + ".oci.json.gz" in fixtures, "Missing producer OCI metadata for " + name)
        oci = read_oci(directory, variant)
        config = oci["config"]
        verify_startup(image, config["config"])
        require((config["os"], config["architecture"]) == ("linux", bundle["architecture"]),
                "OCI metadata has the wrong platform for " + name)
        require(config["config"].get("Labels", {}).get("Tag") == bundle["build_version"],
                "OCI image has the wrong VS release for " + name)
        require(oci["config_digest"] == spec["config_digest"] and oci["layers"] == spec["layers"]
                and config["rootfs"] == spec["rootfs"], "Producer OCI descriptors disagree for " + name)
        rootfs = spec["rootfs"]
        layers = spec["layers"]
        require(layers and rootfs["type"] == "layers" and len(rootfs["diff_ids"]) == len(layers),
                "Invalid VS filesystem layer count for " + name)
        for layer in layers:
            require(re.fullmatch(r"sha256:[a-f0-9]{64}", layer["digest"]) and layer["size"] >= 0,
                    "Invalid VS layer descriptor for " + name)
        archive = bundle_file(directory, spec["file"])
        result = docker_archive(archive, name + ":latest", spec["config_digest"], layers)
        require(result["sha256"] == spec["sha256"], "VS archive checksum differs: " + name)
    if "debug" in archives:
        runtime = archives["runtime"]
        debug = archives["debug"]
        count = len(runtime["layers"])
        require(debug["layers"][:count] == runtime["layers"]
                and debug["rootfs"]["diff_ids"][:count] == runtime["rootfs"]["diff_ids"],
                "The debug image does not extend the selected VS runtime")
    return bundle


@contextmanager
def loaded_image(directory, spec, bundle):
    """Load this verified export under a job-private tag, run checks, then release that tag."""
    tag = "sonic-vs-probe:" + uuid.uuid4().hex
    try:
        image, config, _ = validate_runtime.load_archive(directory / spec["file"], tag, spec["config_digest"])
        validate_runtime.verify_loaded_image(image, config, spec["config_digest"])
        require((config["os"], config["architecture"]) == ("linux", bundle["architecture"])
                and config["rootfs"] == spec["rootfs"], "Docker loaded different VS filesystem layers")
        require(config["config"].get("Labels", {}).get("Tag") == bundle["build_version"],
                "Docker loaded a different VS release")
        yield tag
    finally:
        subprocess.run(["docker", "image", "rm", tag], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       timeout=60, check=False)
