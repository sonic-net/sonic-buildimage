#!/usr/bin/env python3
"""Bind the installed kernel Debian package to its verified Bazel source pedigree."""

import hashlib
import json
import subprocess
from pathlib import Path
import re
import sys
import tarfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from tools.bazel.kernel import prepare as kernel
import sbom_fragment
import sbom_purl


SOURCE_URL = "https://github.com/securely1g/sonic-linux-kernel"
DEBIAN_ARCHIVE_URL = "https://packages.trafficmanager.net/public/debian-security/pool/updates/main/l/linux/"


def replace_recipe_components(components, installed):
    """Replace the image recipe with verified provenance; preserve other DEBs."""
    return [record for record in components if record.get("name") != installed["name"]] + [installed]


def _installed_hash(root, archive, member):
    path = root / member.name.removeprefix("./")
    resolved = path.resolve(strict=True)
    kernel.require(resolved.is_relative_to(root) and not path.is_symlink()
                   and resolved.is_file() and resolved.stat().st_size == member.size,
                   "kernel SBOM installed file is missing, redirected or has changed size: " + member.name)
    digest = hashlib.sha256()
    with archive.extractfile(member) as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    expected = digest.hexdigest()
    kernel.require(kernel.sha256(resolved) == expected,
                   "kernel SBOM installed bytes differ from the selected runtime DEB: " + member.name)
    return expected


def _patches(workspace, manifest, commit):
    source = workspace / kernel.KERNEL_PATH
    # Use the same SONiC series/CVE interpretation as the existing SBOM and VEX
    # emitters. The separate Debian packaging patch is applied explicitly by
    # the kernel action; Debian's own patches remain in the source ancestor.
    records = sbom_fragment.enumerate_patches(str(source / "patches-sonic"))
    records.extend(record for record in sbom_fragment.enumerate_patches(str(source / "patches-debian"))
                   if record["name"] in {"disable-secureboot-config-checks.patch", "bazel-debug-sources.patch",
                                         "bazel-relocatable-tools.patch"})
    inventory = {item["name"]: item["sha256"] for item in manifest["source_files"]}
    patches, properties = [], []
    for record in records:
        path = Path(record["path"]).resolve().relative_to(source).as_posix()
        kernel.require(inventory.get(path) == record["sha256"],
                       "kernel SBOM patch differs from the compiled source inventory: " + path)
        entry = sbom_fragment._patch_entry(record)
        entry["diff"]["url"] = SOURCE_URL + "/blob/" + commit + "/" + path
        patches.append(entry)
        properties.append({"name": "sonic:patch_sha256", "value": record["sha256"] + "  " + path})
    return patches, properties


def component(debs_directory, workspace, rootfs):
    """Return one host kernel component; fail if provenance or installed bytes differ."""
    workspace = Path(workspace).resolve(strict=True)
    debs_directory = Path(debs_directory)
    rootfs = Path(rootfs).resolve(strict=True)
    kernel.require(rootfs.is_dir(), "kernel SBOM requires the installed rootfs")
    manifest = json.loads(kernel.regular(debs_directory / kernel.MANIFEST).read_text())
    provenance = json.loads(kernel.regular(debs_directory / kernel.PROVENANCE).read_text())
    kernel.require(isinstance(manifest, dict) and isinstance(provenance, dict)
                   and isinstance(manifest.get("kernel_config"), dict)
                   and isinstance(provenance.get("launcher"), dict)
                   and isinstance(provenance["launcher"].get("architecture"), str),
                   "kernel SBOM requires the recorded configuration and execution architecture")
    verified = kernel.verify(debs_directory, workspace,
                             architecture=provenance["launcher"]["architecture"],
                             kernel_config=manifest["kernel_config"])
    manifest, provenance = verified["manifest"], verified["provenance"]
    abi = manifest["kernel_abi"]
    runtime = next(item for item in manifest["packages"] if item["kind"] == "runtime")
    name = runtime["package"]
    copyright_path = "usr/share/doc/" + name + "/copyright"
    image_path = "boot/vmlinuz-" + abi
    module_prefixes = ("lib/modules/" + abi + "/", "usr/lib/modules/" + abi + "/")
    status = subprocess.check_output([
        "dpkg-query", "--admindir=" + str(rootfs / "var/lib/dpkg"), "-W",
        "-f=${Status}\t${Version}\t${Architecture}", name], text=True)
    kernel.require(status == "install ok installed\t" + runtime["version"] + "\t" + runtime["architecture"],
                   "kernel SBOM requires the selected installed Debian package record")
    checked = {}
    modules = 0
    with subprocess.Popen(["dpkg-deb", "--fsys-tarfile", str(debs_directory / runtime["name"])], stdout=subprocess.PIPE) as process:
        with tarfile.open(fileobj=process.stdout, mode="r|") as archive:
            for member in archive:
                path = member.name.removeprefix("./")
                is_module = path.startswith(module_prefixes) and re.search(r"\.ko(?:\.(?:xz|gz|zst))?$", path)
                if path in {image_path, copyright_path} or is_module:
                    kernel.require(member.isfile(), "kernel SBOM expected a regular runtime file: " + path)
                    checked[path] = _installed_hash(rootfs, archive, member)
                    modules += bool(is_module)
        # tarfile stops at the end markers; drain dpkg-deb padding before wait.
        for _ in iter(lambda: process.stdout.read(1024 * 1024), b""):
            pass
        kernel.require(process.wait() == 0, "cannot inspect the kernel Debian payload")
    kernel.require(image_path in checked and copyright_path in checked,
                   "kernel SBOM requires the installed image and Debian copyright")
    kernel.require(modules == manifest["debug"]["module_count"],
                   "kernel SBOM installed module count differs from the build manifest")
    commit = provenance["kernel_gitlink"]
    purl = sbom_purl.build("deb", name, runtime["version"], namespace="sonic",
                           qualifiers={"arch": runtime["architecture"]})
    patches, patch_properties = _patches(workspace, manifest, commit)
    ancestor = {
        "type": "library", "name": "linux", "version": manifest["package_version"],
        "purl": sbom_purl.build("deb", "linux", manifest["package_version"], namespace="debian", qualifiers={"arch": "source"}),
        "externalReferences": [
            {"type": "distribution", "url": DEBIAN_ARCHIVE_URL + item["name"],
             "hashes": [{"alg": "SHA-256", "content": item["sha256"]}]}
            for item in manifest["source_archives"]
        ],
    }
    pedigree = {"ancestors": [ancestor]}
    if patches:
        pedigree["patches"] = patches
    return {
        "type": "operating-system", "name": name, "version": manifest["package_version"],
        "bom-ref": purl, "purl": purl,
        "cpe": "cpe:2.3:o:linux:linux_kernel:" + manifest["kernel_version"] + ":*:*:*:*:*:*:*",
        "hashes": [{"alg": "SHA-256", "content": runtime["sha256"]}],
        "licenses": [{"expression": "GPL-2.0-only"}],
        "pedigree": pedigree,
        "externalReferences": [
            {"type": "vcs", "url": SOURCE_URL + "/tree/" + commit},
            {"type": "license", "url": "file:///" + copyright_path,
             "hashes": [{"alg": "SHA-256", "content": checked[copyright_path]}]},
        ],
        "properties": [
            {"name": "sonic:fragment_kind", "value": "recipe-emit"},
            {"name": "sonic:recipe_type", "value": "BAZEL_KERNEL_DEB"},
            {"name": "sonic:scope", "value": "host-image"},
            {"name": "sonic:artifact_filename", "value": runtime["name"]},
            {"name": "sonic:artifact_format", "value": "deb"},
            {"name": "sonic:src_path", "value": kernel.KERNEL_PATH.as_posix()},
            {"name": "sonic:arch", "value": manifest["architecture"]},
            {"name": "sonic:submodule_commit", "value": commit},
            {"name": "sonic:source_tree_sha256", "value": manifest["source_tree_sha256"]},
            {"name": "sonic:kernel_abi", "value": abi},
            {"name": "sonic:kernel_build_id", "value": manifest["debug"]["kernel_build_id"]},
            {"name": "sonic:installed_vmlinuz_sha256", "value": checked[image_path]},
            {"name": "sonic:installed_module_count", "value": str(modules)},
            *patch_properties,
        ],
    }
