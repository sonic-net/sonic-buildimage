#!/usr/bin/env python3
"""Exercise shared OCI configuration and payload checks with an unrelated image policy."""

import copy
import gzip
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tarfile
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

ROOT = Path(__file__).absolute().parents[4]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.validation_inputs import validation_input_cache
from tools.bazel.tests.oci_base_fixture import digest, oci_files, tar_entries as layer, write_layout
from tools.bazel.oci import image_validation as subject
from tools.bazel.oci import package_payloads as validate_payloads
from tools.bazel.oci import package_contract as source_packages
from tools.bazel.tests.oci_contract_fixture import identity, owner_policies, package_policy, source_contract

OWNER_POLICIES = {}
ALTERNATIVE_PATHS = tuple("etc/alternatives/" + name for name in ("editor", "ex", "rview", "vi", "view"))
DEBUG_ALTERNATIVES = [{"paths": list(ALTERNATIVE_PATHS), "before": "/usr/bin/vim.tiny", "after": "/usr/bin/vim.basic"}]


def write_image(path, layers, manifest, *, entrypoint=None, settings=None):
    config = {
        "architecture": "amd64", "os": "linux",
        "rootfs": {"type": "layers", "diff_ids": [digest(data) for data in layers]},
        "config": {"Entrypoint": entrypoint or ["/usr/local/bin/supervisord"],
                   "Env": ["DEBIAN_FRONTEND=noninteractive"], **(settings or {}),
                   "Labels": {"com.azure.sonic.manifest": json.dumps(manifest, separators=(",", ":"))}},
    }
    write_layout(path, oci_files(json.dumps(config, separators=(",", ":")).encode(), layers))


class ValidateImageTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="oci-image-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.policy = package_policy()
        self.source_contract = source_contract()
        self.source_contract_path = self.root / "source-contract.json"
        self.source_contract_path.write_text(json.dumps(self.source_contract))
        self.policy["image_validation"] = {
            "entrypoint": ["/usr/local/bin/supervisord"], "command": None,
            "environment": {"DEBIAN_FRONTEND": "noninteractive"},
            "startup_files": {
                "usr/bin/start.sh": {"source": "start.sh", "mode": 0o755},
                "etc/supervisor/conf.d/supervisord.conf": {"source": "supervisord.conf", "mode": 0o644},
                "etc/supervisor/critical_processes": {"source": "critical_processes", "mode": 0o644},
            },
            "debug_alternatives": DEBUG_ALTERNATIVES,
            "runtime_tag": "docker-example:latest", "debug_tag": "docker-example-dbg:latest",
        }
        for name in ("start.sh", "supervisord.conf", "critical_processes"):
            (self.root / name).write_text("fixture " + name + "\n")
        self.filter_path = ROOT / "dockers/docker-base-trixie/dpkg_01_drop"
        self.runtime_manifest = self.root / "runtime-manifest.json"
        self.debug_manifest = self.root / "debug-manifest.json"
        self.runtime_manifest.write_text('{"container_name":"syncd","version":"1.0.0"}\n')
        self.debug_manifest.write_text('{"container_name":"syncd","version":"1.0.0-dbg"}\n')
        header = bytearray(64)
        header[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", header, 16, 3, 62)
        self.ssh_elf = bytes(header) + b"FIPS OpenSSH runtime"
        self.runtime_handoff, self.runtime_payload = self.handoff("runtime")
        self.debug_handoff, self.debug_payload = self.handoff("debug", runtime=self.runtime_handoff)
        self.base = layer([("usr/bin/base", b"base\n", 0o755)])
        copied = []
        for source, destination, mode in (
            ("start.sh", "usr/bin/start.sh", 0o755),
            ("supervisord.conf", "etc/supervisor/conf.d/supervisord.conf", 0o644),
            ("critical_processes", "etc/supervisor/critical_processes", 0o644),
        ):
            copied.append((destination, (self.root / source).read_bytes(), mode))
        self.config_layer = layer(copied)
        self.runtime_layers = [self.base, self.runtime_payload.read_bytes(), self.config_layer]
        self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes()]
        self.write_images()

    def handoff(self, variant, runtime=None):
        directory = self.root / (variant + "-handoff")
        directory.mkdir(parents=True)
        package = "example-runtime" if variant == "runtime" else "example-runtime-dbgsym"
        payload = directory / "payload.tar"
        packages = [(package, "1.0", [("usr/share/" + package + "/data", package.encode(), 0o644)])]
        if variant == "runtime":
            packages.append(("openssh-client", "1.0+fips", [("usr/bin/ssh", self.ssh_elf, 0o755)]))
        records, entries = [], []
        for name, version, members in packages:
            data = layer(members)
            records.append({"package": name, "version": version, "architecture": "amd64",
                "source_deb": name + "_" + version + "_amd64.deb", "source_size": 1,
                "source_sha256": hashlib.sha256(name.encode()).hexdigest(),
                "control_sha256": hashlib.sha256((name + " control").encode()).hexdigest(),
                "control_fields": {"Package": name, "Version": version, "Architecture": "amd64"},
                "payload_sha256": hashlib.sha256(data).hexdigest(), "payload_size": len(data),
                "payload_members": len(members)})
            entries.extend(members)
        payload.write_bytes(layer(entries))
        digest = hashlib.sha256(payload.read_bytes()).hexdigest()
        value = {"schema": 1, "image": self.policy["image"], "variant": variant,
                 "architecture": "amd64", "distribution": "trixie", "features": dict(self.policy["features"]),
                 "required_packages": [record["package"] for record in records], "debug_apt_packages": [], "packages": records,
                 "payload": {"path": "payload.tar", "sha256": digest, "size": payload.stat().st_size, "members": len(entries)}}
        if runtime:
            value["runtime_manifest_sha256"] = hashlib.sha256(runtime.read_bytes()).hexdigest()
            value["debug_apt_packages"] = ["gdb", "gdbserver", "sshpass", "strace", "vim"]
        manifest = directory / "manifest.json"
        manifest.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
        return manifest, payload

    def append_debug_package(self, package, version, entries):
        additional = layer(entries)
        combined = io.BytesIO()
        with tarfile.open(fileobj=combined, mode="w", format=tarfile.GNU_FORMAT) as output:
            for data in (self.debug_payload.read_bytes(), additional):
                with tarfile.open(fileobj=io.BytesIO(data), mode="r:") as archive:
                    for member in archive:
                        output.addfile(member, archive.extractfile(member) if member.isfile() else None)
        self.debug_payload.write_bytes(combined.getvalue())
        manifest = json.loads(self.debug_handoff.read_bytes())
        manifest["packages"].append({"package": package, "version": version, "architecture": "amd64",
            "source_deb": package + "_" + version + "_amd64.deb", "source_size": 1,
            "source_sha256": hashlib.sha256((package + version).encode()).hexdigest(),
            "control_sha256": hashlib.sha256((package + " control").encode()).hexdigest(),
            "payload_sha256": hashlib.sha256(additional).hexdigest(), "payload_size": len(additional),
            "payload_members": len(entries)})
        manifest["required_packages"].append(package)
        manifest["payload"] = {"path": "payload.tar", "sha256": hashlib.sha256(combined.getvalue()).hexdigest(),
                               "size": len(combined.getvalue()), "members": sum(item["payload_members"] for item in manifest["packages"])}
        self.debug_handoff.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")

    def write_archive(self, path, *, tag="docker-example:latest", config_bytes=None, corrupt_layer=False):
        _, manifest, _, _ = subject.image(self.root / "runtime.oci")
        digest = manifest["config"]["digest"]
        if config_bytes is None:
            config_bytes = (self.root / "runtime.oci/blobs/sha256" / digest[7:]).read_bytes()
        entries = [("config.json", config_bytes, 0o644)]
        entries += [("layer" + str(index) + ".tar", b"changed" if corrupt_layer and index == 0 else data, 0o644)
                    for index, data in enumerate(self.runtime_layers)]
        docker_manifest = [{"Config": "config.json", "RepoTags": [tag],
                            "Layers": ["layer" + str(index) + ".tar" for index in range(len(self.runtime_layers))]}]
        entries.append(("manifest.json", json.dumps(docker_manifest).encode(), 0o644))
        path.write_bytes(gzip.compress(layer(entries), mtime=0))
        return digest

    def write_images(self, *, runtime_entrypoint=None):
        write_image(self.root / "runtime.oci", self.runtime_layers, json.loads(self.runtime_manifest.read_bytes()),
                    entrypoint=runtime_entrypoint)
        write_image(self.root / "debug.oci", self.debug_layers, json.loads(self.debug_manifest.read_bytes()),
                    entrypoint=runtime_entrypoint)

    def validate(self, **kwargs):
        return subject.validate_images(self.root / "runtime.oci", self.root / "debug.oci",
                                       self.runtime_handoff, self.debug_handoff,
                                       self.runtime_manifest, self.debug_manifest, self.root, fixture=True,
                                       policy=self.policy, dpkg_filter_path=self.filter_path, **kwargs)

    def inherited_symbols(self, *, install=True):
        """Add an independently recorded imported pair to the image fixtures."""
        path = "usr/lib/x86_64-linux-gnu/libsonicdbcli.so.0.0.0"
        runtime = b"inherited runtime fixture"
        debug_path = "usr/lib/debug/.build-id/aa/fixture.debug"
        debug = b"inherited symbols fixture"
        self.runtime_layers.append(layer([(path, runtime, 0o644)]))
        self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes()]
        payload = self.root / "base-symbols.tar"
        payload.write_bytes(layer([(debug_path, debug, 0o644)]))
        if install:
            self.debug_layers.append(payload.read_bytes())
        self.write_images()
        descriptor, _, _, _ = subject.image(self.root / "runtime.oci")
        receipt = self.root / "base-symbols.json"
        receipt.write_text(json.dumps({"schema": 1, "expected_platform": "linux/amd64",
            "runtime_manifest": descriptor["digest"], "selected_paths": [debug_path],
            "output": {"sha256": subject.sha(payload), "size": payload.stat().st_size},
            "pairs": [{"runtime_path": path, "runtime_sha256": hashlib.sha256(runtime).hexdigest(),
                       "debug_path": debug_path, "debug_sha256": hashlib.sha256(debug).hexdigest()}]}))
        return payload, receipt

    def test_dynamic_imported_symbols_are_included_in_the_debug_image(self):
        """The final-image check consumes the shared matcher receipt instead of pinned symbol IDs."""
        payload, receipt = self.inherited_symbols()
        result = self.validate(base_debug_symbols=payload, base_debug_receipt=receipt)
        self.assertEqual(len(result["inherited_base_symbols"]["pairs"]), 1)

    def test_dynamic_imported_symbols_cannot_be_omitted_from_the_image(self):
        """Producing a matching TAR is insufficient if image assembly forgets that layer."""
        payload, receipt = self.inherited_symbols(install=False)
        with self.assertRaisesRegex(ValueError, "debug imported base symbols"):
            self.validate(base_debug_symbols=payload, base_debug_receipt=receipt)

    def source_archives(self, extra_runtime=()):
        """Model owner tar payloads independently of Make's remaining package handoff."""
        records = copy.deepcopy(self.source_contract["packages"])
        directory = self.root / "source"
        directory.mkdir()
        runtime_entries, debug_entries = [], []
        for index, record in enumerate(records):
            package = record["package"]
            runtime = []
            for name, kind in record.pop("required_paths").items():
                if kind == "symlink":
                    # The OCI fixture helper supports regular files only, so
                    # the source inventory here contains the substantive paths.
                    continue
                data = self.ssh_elf + package.encode() if kind == "elf" else b"source data"
                runtime.append((name, data, 0o755 if kind == "elf" else 0o644))
            if index == 0:
                runtime.extend(extra_runtime)
            debug = [("usr/lib/debug/.build-id/ab/" + str(index) * 38 + ".debug",
                      self.ssh_elf + b"symbols" + package.encode(), 0o644)]
            runtime_path, debug_path = directory / (package + ".tar"), directory / (package + "-debug.tar")
            runtime_path.write_bytes(layer(runtime))
            debug_path.write_bytes(layer(debug))
            record.update(input_tar_sha256=subject.sha(runtime_path), files=source_packages.inventory(runtime_path))
            runtime_entries.extend(runtime)
            debug_entries.extend(debug)
        runtime_tar, debug_tar = directory / "runtime.tar", directory / "debug.tar"
        runtime_tar.write_bytes(layer(runtime_entries))
        debug_tar.write_bytes(layer(debug_entries))
        receipt = {**identity(self.policy), "schema": source_packages.RECEIPT_SCHEMA,
                   "kind": "bazel_source", "packages": records,
                   "contract_sha256": subject.sha(self.source_contract_path),
                   "base_manifest_digest": "sha256:" + "b" * 64,
                   "module_file_sha256": {name: "a" * 64 for name in {record["source"]["module"] for record in records}}}
        receipt["payload"] = {"sha256": subject.sha(runtime_tar), "size": runtime_tar.stat().st_size,
                              "members": len(source_packages.inventory(runtime_tar))}
        receipt_path = directory / "receipt.json"
        receipt_path.write_text(json.dumps(receipt))
        self.runtime_layers.append(runtime_tar.read_bytes())
        self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes(), debug_tar.read_bytes()]
        self.write_images()
        return {"source_runtime_tar": runtime_tar, "source_debug_tar": debug_tar, "source_receipt": receipt_path}

    def test_both_owner_policies_use_their_declared_input_adapters(self):
        """SWSS validates source-only layers; Syncd validates retained inputs with the same engine."""
        if not OWNER_POLICIES:
            self.skipTest("BUILD-generated owner policies are supplied by the Bazel target")
        for name, policy in OWNER_POLICIES.items():
            with self.subTest(owner=name):
                retained = policy["retained_source"] == "make"
                runtime_handoff = self.runtime_handoff if retained else None
                debug_handoff = self.debug_handoff if retained else None
                if retained:
                    value = json.loads(runtime_handoff.read_text())
                    value.update(identity(policy), features=policy["features"])
                    runtime_handoff.write_text(json.dumps(value))
                    value = json.loads(debug_handoff.read_text())
                    value.update(identity(policy), features=policy["features"],
                                 debug_apt_packages=policy["debug_tools"],
                                 runtime_manifest_sha256=subject.sha(runtime_handoff))
                    debug_handoff.write_text(json.dumps(value))
                expectations = policy["image_validation"]
                startup = [(path, (ROOT / item["source"]).read_bytes(), item["mode"])
                           for path, item in expectations["startup_files"].items()]
                runtime_layers = [self.base] + ([self.runtime_payload.read_bytes()] if retained else []) + [layer(startup)]
                debug_layers = runtime_layers + ([self.debug_payload.read_bytes()] if retained else [])
                settings = {"Entrypoint": expectations["entrypoint"], "Cmd": expectations["command"],
                            "Env": [key + "=" + value for key, value in expectations["environment"].items()]}
                for variant, layers in (("runtime", runtime_layers), ("debug", debug_layers)):
                    write_image(self.root / (variant + ".oci"), layers,
                                json.loads(getattr(self, variant + "_manifest").read_text()), settings=settings)
                result = subject.validate_images(self.root / "runtime.oci", self.root / "debug.oci",
                    runtime_handoff, debug_handoff, self.runtime_manifest, self.debug_manifest, ROOT,
                    policy=policy, fixture=True, dpkg_filter_path=self.filter_path if retained else None)
                self.assertEqual(result["runtime_package_count"], 2 if retained else 0)
                self.assertEqual(result["debug_package_count"], 1 if retained else 0)

    def test_source_payloads_and_receipt_are_bound_to_the_images(self):
        """Final validation proves the reused source bytes and their recorded owners are present."""
        inputs = self.source_archives()
        result = self.validate(**inputs)
        self.assertEqual(result["source_receipt_sha256"], subject.sha(inputs["source_receipt"]))
        self.assertEqual({item["package"] for item in result["source_packages"]["packages"]}, set(source_packages.package_names(self.source_contract)))
        self.assertEqual(result["source_runtime_payload_entries"], 1)
        self.assertEqual(result["source_debug_payload_entries"], len(self.source_contract["packages"]))
        inputs["source_runtime_tar"].write_bytes(inputs["source_runtime_tar"].read_bytes() + b"changed")
        with self.assertRaisesRegex(ValueError, "source receipt archive hash differs"):
            self.validate(**inputs)

    def test_debug_cannot_replace_a_source_library(self):
        """Debug adds matching symbols while preserving every source-built runtime ELF."""
        inputs = self.source_archives()
        self.debug_layers.append(layer([("usr/lib/libexample.so.2.4.0",
                                         self.ssh_elf + b"different library", 0o755)]))
        self.write_images()
        with self.assertRaisesRegex(ValueError, "debug image changes the source runtime payload"):
            self.validate(**inputs)

    def test_missing_source_layer_is_rejected(self):
        """A valid source receipt cannot stand in for installing its runtime library archive."""
        inputs = self.source_archives()
        self.runtime_layers.pop()
        self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes(), inputs["source_debug_tar"].read_bytes()]
        self.write_images()
        with self.assertRaisesRegex(ValueError, "runtime source package payload differs"):
            self.validate(**inputs)

    def test_source_inputs_must_be_complete_and_are_required_for_production(self):
        """Require both owner archives and their receipt before claiming complete validation."""
        with self.assertRaisesRegex(ValueError, "must be supplied together"):
            self.validate(source_runtime_tar=self.runtime_payload)
        complete = {name: self.runtime_payload for name in
                    ("base_path", "apt_layer", "debug_tools_layer", "runtime_apt_selection", "debug_apt_selection",
                     "runtime_archive", "debug_archive", "apt_lock_path")}
        with self.assertRaisesRegex(ValueError, "complete validation requires source"):
            subject.validate_images(self.root / "runtime.oci", self.root / "debug.oci", self.runtime_handoff,
                                    self.debug_handoff, self.runtime_manifest, self.debug_manifest, self.root, policy=self.policy,
                                    source_contract_path=self.source_contract_path, dpkg_filter_path=self.filter_path, **complete)

    def complete_make_only_inputs(self, *, direct_layers=False):
        """Supply real OCI descriptors, overlay receipts and save archives for the production path."""
        self.policy["native"].update(require_source=False, require_imported_symbols=False)
        self.policy["image_validation"].update(startup_files={}, debug_alternatives=[],
                                               required_elf_paths=["usr/bin/ssh"], package_state=False)
        if direct_layers:
            self.policy.update(retained_source="none", retained_requirements=[])
        base_stream = io.BytesIO()
        with tarfile.open(fileobj=base_stream, mode="w") as archive:
            directories = {".", "usr", "var", *[item["target"] for item in validate_payloads.DIRECTORY_ALIASES.values()]}
            for name in sorted(directories):
                member = tarfile.TarInfo(name)
                member.type, member.mode = tarfile.DIRTYPE, 0o755
                archive.addfile(member)
            for name, item in validate_payloads.DIRECTORY_ALIASES.items():
                member = tarfile.TarInfo(name)
                member.type, member.mode, member.linkname = tarfile.SYMTYPE, 0o777, item["linkname"]
                archive.addfile(member)
        base = self.root / "managed-base.oci"
        write_image(base, [base_stream.getvalue()], {"base": True})
        apt = self.root / "apt.tar"
        apt.write_bytes(layer([]))
        tools = self.root / "tools.tar"
        tools.write_bytes(layer([("usr/share/example/debug-tool", b"debug addition", 0o644)]))
        self.runtime_layers = [base_stream.getvalue(), self.runtime_payload.read_bytes(), apt.read_bytes()]
        if direct_layers:
            self.runtime_layers = [base_stream.getvalue(), apt.read_bytes(), self.runtime_payload.read_bytes()]
        self.debug_layers = self.runtime_layers + [tools.read_bytes(), self.debug_payload.read_bytes()]
        self.write_images()
        lock = self.root / "apt-lock.json"
        lock.write_text('{"version": 2}\n')
        selections = {}
        for variant, selected_base, manifest in (
                ("runtime", base, self.runtime_handoff),
                ("debug", self.root / "runtime.oci", self.debug_handoff)):
            selection = self.root / (variant + "-selection.json")
            selection.write_text(json.dumps({"schema": 1, "variant": variant,
                "base_manifest_digest": subject.image(selected_base)[0]["digest"],
                "apt_lock_sha256": subject.sha(lock), "make_manifest_sha256": subject.sha(manifest),
                "selected": [], "skipped_base": [], "skipped_make": [], "changed_non_elf_base_paths": []}))
            if direct_layers:
                selected = json.loads(selection.read_bytes())
                selected.update(image=self.policy["image"], group=selected.pop("variant"),
                                dependency_check={"status": "satisfied"},
                                skipped_retained=selected.pop("skipped_make"))
                selected.pop("make_manifest_sha256")
                selection.write_text(json.dumps(selected))
            selections[variant + "_apt_selection"] = selection
        archives = {}
        for variant, layers in (("runtime", self.runtime_layers), ("debug", self.debug_layers)):
            layout = self.root / (variant + ".oci")
            manifest = subject.image(layout)[1]
            config = (layout / "blobs/sha256" / manifest["config"]["digest"].split(":", 1)[1]).read_bytes()
            names = ["layer" + str(index) + ".tar" for index in range(len(layers))]
            saved_manifest = [{"Config": "config.json", "RepoTags": [self.policy["image_validation"][variant + "_tag"]],
                               "Layers": names}]
            entries = [("config.json", config, 0o644), ("manifest.json", json.dumps(saved_manifest).encode(), 0o644)]
            entries.extend((name, contents, 0o644) for name, contents in zip(names, layers))
            archive = self.root / (variant + "-save.tar.gz")
            archive.write_bytes(gzip.compress(layer(entries), mtime=0))
            archives[variant + "_archive"] = archive
        declared = {"runtime_payload_layers": [self.runtime_payload], "debug_payload_layers": [self.debug_payload]} if direct_layers else {}
        return {"base_path": base, "apt_layer": apt, "debug_tools_layer": tools,
                "apt_lock_path": lock, **selections, **archives, **declared}

    def validate_direct_images(self, inputs):
        return subject.validate_images(self.root / "runtime.oci", self.root / "debug.oci", None, None,
            self.runtime_manifest, self.debug_manifest, self.root, policy=self.policy, **inputs)

    def complete_receipted_layers(self, *, extra_runtime=(), omitted_runtime=()):
        """Compose an owned library with other payloads, retaining its independent receipt."""
        # This fixture installs one real file; link semantics have independent
        # package-contract tests and are not needed for the composition case.
        for record in self.source_contract["packages"]:
            record["required_paths"] = {name: kind for name, kind in record["required_paths"].items()
                                        if kind != "symlink"}
        self.source_contract_path.write_text(json.dumps(self.source_contract))
        source = self.source_archives(extra_runtime)
        combined = io.BytesIO()
        with tarfile.open(fileobj=combined, mode="w") as output:
            for path in (self.runtime_payload, source["source_runtime_tar"]):
                with tarfile.open(path) as archive:
                    for member in archive:
                        if member.name in omitted_runtime:
                            continue
                        output.addfile(member, archive.extractfile(member) if member.isfile() else None)
        self.runtime_payload.write_bytes(combined.getvalue())
        self.debug_payload = source["source_debug_tar"]
        inputs = self.complete_make_only_inputs(direct_layers=True)
        self.policy["retained_source"] = "source"
        receipt = json.loads(source["source_receipt"].read_bytes())
        receipt["base_manifest_digest"] = subject.image(inputs["base_path"])[0]["digest"]
        source["source_receipt"].write_text(json.dumps(receipt))
        for variant in ("runtime", "debug"):
            path = inputs[variant + "_apt_selection"]
            selection = json.loads(path.read_bytes())
            selection.update(variant=selection.pop("group"), skipped_source=selection.pop("skipped_retained"),
                             skipped_make=[], make_manifest_sha256=None,
                             source_receipt_sha256=subject.sha(source["source_receipt"]))
            path.write_text(json.dumps(selection))
        excludes, includes = self.root / "source-excludes.txt", self.root / "source-includes.txt"
        excludes.write_text("usr/share/doc/*\n")
        includes.write_text("\n".join("usr/share/doc/*/" + name
                                     for name in ("copyright", "LICENSE*", "COPYING*", "NOTICE*")) + "\n")
        return {**inputs, **source, "source_contract_path": self.source_contract_path,
                "source_layer_excludes": excludes, "source_layer_includes": includes}

    def test_direct_layers_keep_their_source_package_ownership_receipt(self):
        """An aggregate layer can add files while preserving independently checked owner bytes."""
        inputs = self.complete_receipted_layers()
        result = self.validate_direct_images(inputs)
        self.assertEqual(result["validation_mode"], "complete")
        self.assertEqual(result["source_receipt_sha256"], subject.sha(inputs["source_receipt"]))
        self.assertEqual(result["source_runtime_payload_entries"], 3)
        self.assertEqual(result["archives"]["runtime"]["sha256"], subject.sha(inputs["runtime_archive"]))

    def test_source_retaining_direct_layers_cannot_omit_their_receipt(self):
        """Actual source policy must not fall back to the unreceipted direct-layer adapter."""
        inputs = self.complete_make_only_inputs(direct_layers=True)
        self.policy["retained_source"] = "source"
        with self.assertRaisesRegex(ValueError, "cannot replace a required package receipt"):
            self.validate_direct_images(inputs)

    def test_direct_layers_cannot_replace_a_receipted_package(self):
        """A valid receipt cannot be hidden by composing different bytes at its installed path."""
        inputs = self.complete_receipted_layers()
        replacement = self.root / "replacement.tar"
        replacement.write_bytes(layer([("usr/lib/libexample.so.2.4.0", self.ssh_elf + b"changed", 0o755)]))
        inputs["runtime_payload_layers"].append(replacement)
        with self.assertRaisesRegex(ValueError, "declared layers change the receipted runtime payload"):
            self.validate_direct_images(inputs)

    def test_receipted_layers_apply_declared_documentation_filter(self):
        """Keep the full source receipt while the declared OCI policy excludes a changelog."""
        changelog = "usr/share/doc/example/changelog.Debian.gz"
        notices = [("usr/share/doc/example/" + name, name.encode(), 0o644)
                   for name in ("copyright", "LICENSE", "COPYING", "NOTICE")]
        inputs = self.complete_receipted_layers(
            extra_runtime=[(changelog, gzip.compress(b"source history", mtime=0), 0o644), *notices],
            omitted_runtime=[changelog])
        result = self.validate_direct_images(inputs)
        self.assertEqual(result["validation_mode"], "complete")
        self.assertIn(changelog, result["source_packages"]["packages"][0]["files"])

    def test_receipted_layers_reject_missing_legal_notice_or_library(self):
        """Filtering must not turn absent retained files into an accepted source subset."""
        notice = "usr/share/doc/example/NOTICE"
        library = "usr/lib/libexample.so.2.4.0"
        inputs = self.complete_receipted_layers(extra_runtime=[(notice, b"legal notice", 0o644)])
        original = inputs["runtime_payload_layers"][0].read_bytes()
        for missing in (notice, library):
            output = io.BytesIO()
            with tarfile.open(fileobj=io.BytesIO(original)) as source, tarfile.open(fileobj=output, mode="w") as archive:
                for member in source:
                    if member.name != missing:
                        archive.addfile(member, source.extractfile(member) if member.isfile() else None)
            inputs["runtime_payload_layers"][0].write_bytes(output.getvalue())
            with self.subTest(path=missing), self.assertRaisesRegex(ValueError, "receipted runtime payload.*" + missing):
                self.validate_direct_images(inputs)

    def test_receipted_layers_reject_changed_notice_bytes_or_metadata(self):
        """An OCI include exception preserves complete file metadata and content."""
        notice = "usr/share/doc/example/COPYING"
        inputs = self.complete_receipted_layers(extra_runtime=[(notice, b"legal notice", 0o644)])
        replacement = self.root / "changed-notice.tar"
        inputs["runtime_payload_layers"].append(replacement)
        for data, mode in ((b"changed notice", 0o644), (b"legal notice", 0o600)):
            replacement.write_bytes(layer([(notice, data, mode)]))
            with self.subTest(mode=mode), self.assertRaisesRegex(ValueError, "receipted runtime payload.*COPYING"):
                self.validate_direct_images(inputs)

    def test_oci_filter_cannot_remove_an_elf_outside_required_paths(self):
        """Even an undeclared extra ELF cannot disappear under documentation exclusions."""
        inputs = self.complete_receipted_layers(
            extra_runtime=[("usr/share/doc/example/extra-program", self.ssh_elf, 0o755)])
        with self.assertRaisesRegex(ValueError, "filter removes a required source path or ELF"):
            self.validate_direct_images(inputs)

    def test_oci_filter_cannot_remove_a_required_non_elf_path(self):
        """The owner contract's required data paths survive independently of ELF checks."""
        self.source_contract["packages"][0]["required_paths"]["usr/share/doc/example/required-data"] = "file"
        inputs = self.complete_receipted_layers()
        with self.assertRaisesRegex(ValueError, "filter removes a required source path or ELF"):
            self.validate_direct_images(inputs)

    def test_receipted_direct_layers_require_both_filter_inputs(self):
        """A caller must bind the producer's policy instead of guessing from installed files."""
        inputs = self.complete_receipted_layers()
        for missing in ("source_layer_excludes", "source_layer_includes"):
            partial = {key: value for key, value in inputs.items() if key != missing}
            with self.subTest(missing=missing), self.assertRaisesRegex(ValueError, "filter inputs must be supplied together"):
                self.validate_direct_images(partial)
        absent = {key: value for key, value in inputs.items() if key not in
                  ("source_layer_excludes", "source_layer_includes")}
        with self.assertRaisesRegex(ValueError, "requires its declared OCI filter policy"):
            self.validate_direct_images(absent)

    def test_receipted_layers_bind_both_apt_selections_and_the_base(self):
        """Runtime/debug APT must retain the same receipt prepared for this exact base."""
        inputs = self.complete_receipted_layers()
        for variant in ("runtime", "debug"):
            path = inputs[variant + "_apt_selection"]
            original = path.read_bytes()
            selection = json.loads(original)
            selection["source_receipt_sha256"] = "f" * 64
            path.write_text(json.dumps(selection))
            with self.subTest(variant=variant), self.assertRaisesRegex(ValueError, variant + " APT selection uses a different source receipt"):
                self.validate_direct_images(inputs)
            path.write_bytes(original)
        receipt = json.loads(inputs["source_receipt"].read_bytes())
        receipt["base_manifest_digest"] = "sha256:" + "f" * 64
        inputs["source_receipt"].write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "prepared for a different OCI base"):
            self.validate_direct_images(inputs)

    def test_receipted_direct_layers_bind_contract_and_archive(self):
        """Owner metadata and the checked TAR remain mandatory even with exact image layers."""
        inputs = self.complete_receipted_layers()
        contract = inputs["source_contract_path"]
        original = contract.read_bytes()
        contract.write_bytes(original + b"\n")
        with self.assertRaisesRegex(ValueError, "different reviewed contract"):
            self.validate_direct_images(inputs)
        contract.write_bytes(original)
        inputs["source_runtime_tar"].write_bytes(inputs["source_runtime_tar"].read_bytes() + b"changed")
        with self.assertRaisesRegex(ValueError, "archive hash differs"):
            self.validate_direct_images(inputs)

    def test_complete_declared_source_layers_use_shared_image_and_archive_checks(self):
        inputs = self.complete_make_only_inputs(direct_layers=True)
        result = self.validate_direct_images(inputs)
        self.assertEqual(result["validation_mode"], "complete")
        self.assertIsNone(result["source_receipt_sha256"])
        self.assertEqual(result["archives"]["runtime"]["sha256"], subject.sha(inputs["runtime_archive"]))
        self.assertEqual(result["source_runtime_payload_entries"], 2)

    def test_declared_layers_cannot_replace_required_source_receipts(self):
        inputs = self.complete_make_only_inputs(direct_layers=True)
        self.policy["native"]["require_source"] = True
        with self.assertRaisesRegex(ValueError, "requires its source contract"):
            self.validate_direct_images(inputs)

    def test_direct_layers_bind_order_even_when_installed_contents_are_identical(self):
        inputs = self.complete_make_only_inputs(direct_layers=True)
        inputs["runtime_payload_layers"] = [inputs["apt_layer"], self.runtime_payload]
        with self.assertRaisesRegex(ValueError, "layers differ from their declared"):
            self.validate_direct_images(inputs)

    def test_direct_layers_reject_changed_symbol_file_and_foreign_apt_selection(self):
        inputs = self.complete_make_only_inputs(direct_layers=True)
        self.debug_payload.write_bytes(layer([("usr/share/example-runtime-dbgsym/data", b"wrong symbols", 0o644)]))
        with self.assertRaisesRegex(ValueError, "debug source package payload"):
            self.validate_direct_images(inputs)
        inputs = self.complete_make_only_inputs(direct_layers=True)
        selection = inputs["runtime_apt_selection"]
        document = json.loads(selection.read_bytes())
        document["image"] = "another-owner"
        selection.write_text(json.dumps(document))
        with self.assertRaisesRegex(ValueError, "wrong image"):
            self.validate_direct_images(inputs)

    def test_installed_policy_binds_archive_and_effective_files(self):
        inputs = self.complete_make_only_inputs()
        runtime, debug = self.root / "runtime.oci", self.root / "debug.oci"
        path = "usr/bin/ssh"
        declarations = [{"architecture": "amd64", "images": [
            {"name": "runtime", "archive": "runtime.tar.gz", "tag": "docker-example:latest",
             "files": {path: {"mode": 0o755, "uid": 0, "gid": 0}}, "directory_aliases": {"var/run": "/run"}},
            {"name": "debug", "archive": "debug.tar.gz", "tag": "docker-example-dbg:latest",
             "parent": "runtime", "inherit_files": [path]},
        ]}]
        layouts = {"runtime": runtime, "debug": debug}
        archives = {"runtime": inputs["runtime_archive"], "debug": inputs["debug_archive"]}
        policy = subject.installed_images_policy(declarations, layouts, archives, "amd64")
        self.assertEqual(policy["images"][0]["files"][path], policy["images"][1]["files"][path])
        self.assertEqual(policy["images"][0]["archive_sha256"], subject.sha(archives["runtime"]))
        self.assertEqual(policy["images"][0]["expected_rootfs"], subject.image(runtime).config["rootfs"])
        declarations[0]["images"][0]["files"][path]["mode"] = 0o644
        with self.assertRaisesRegex(ValueError, "changed declared source file"):
            subject.installed_images_policy(declarations, layouts, archives, "amd64")

    def test_ancestry_checks_uncompressed_rootfs_as_well_as_layer_descriptors(self):
        self.complete_make_only_inputs()
        runtime, debug = subject.image(self.root / "runtime.oci"), subject.image(self.root / "debug.oci")
        config = copy.deepcopy(debug.config)
        config["rootfs"]["diff_ids"][0] = "sha256:" + "f" * 64
        with self.assertRaisesRegex(ValueError, "wrong parent"):
            subject.assert_ancestry(runtime, debug._replace(config=config), "wrong parent")

    def test_complete_make_only_image_needs_no_source_contract(self):
        """The production image validator accepts complete Make-only evidence without source receipts."""
        inputs = self.complete_make_only_inputs()
        result = subject.validate_images(self.root / "runtime.oci", self.root / "debug.oci",
            self.runtime_handoff, self.debug_handoff, self.runtime_manifest, self.debug_manifest, self.root,
            policy=self.policy, dpkg_filter_path=self.filter_path, **inputs)
        self.assertEqual(result["validation_mode"], "complete")
        self.assertIsNone(result["source_packages"])
        self.assertEqual(result["archives"]["runtime"]["tag"], "docker-example:latest")
        self.assertEqual(result["runtime_package_count"], 2)

    def test_complete_image_requires_source_contract_when_policy_requires_sources(self):
        """Full overlay evidence cannot substitute for the source contract selected by image policy."""
        inputs = self.complete_make_only_inputs()
        self.policy["native"]["require_source"] = True
        with self.assertRaisesRegex(ValueError, "requires.*source contract"):
            subject.validate_images(self.root / "runtime.oci", self.root / "debug.oci",
                self.runtime_handoff, self.debug_handoff, self.runtime_manifest, self.debug_manifest, self.root,
                policy=self.policy, dpkg_filter_path=self.filter_path, **inputs)

    def test_complete_fixture_preserves_configuration_and_payloads(self):
        """Validate runtime ancestry, configuration and imported files in complete synthetic layouts."""
        result = self.validate()
        self.assertEqual(result["validation_mode"], "fixture")
        self.assertEqual(result["runtime_layers"], 3)
        self.assertEqual(result["debug_layers"], 4)
        self.assertEqual(result["runtime_package_count"], 2)
        self.assertEqual(result["allowed_debug_package_replacements"], [])
        self.assertEqual(result["changed_non_elf_runtime_paths_in_debug"], [])

    def test_entrypoint_and_runtime_ancestry_are_required(self):
        """Require the configured entrypoint and exact runtime layer prefix in the debug image."""
        self.write_images(runtime_entrypoint=["/bin/false"])
        with self.assertRaisesRegex(ValueError, "entrypoint differs"):
            self.validate()
        self.debug_layers = [layer([("usr/bin/base", b"other base\n", 0o755)])] + self.debug_layers[1:]
        self.write_images()
        with self.assertRaisesRegex(ValueError, "exact runtime OCI layers"):
            self.validate()

    def test_missing_package_payload_is_rejected(self):
        """A valid receipt cannot replace the package bytes missing from the final image."""
        self.runtime_layers = [self.base, self.config_layer]
        self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes()]
        self.write_images()
        with self.assertRaisesRegex(ValueError, "runtime package payload differs"):
            self.validate()

    def test_debug_cannot_replace_a_deployed_elf(self):
        """Debug additions must preserve every native executable already installed in runtime."""
        header = bytearray(64)
        header[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", header, 16, 3, 62)
        self.runtime_layers.append(layer([("usr/lib/libexample.so", bytes(header) + b"runtime", 0o644)]))
        self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes(),
            layer([("usr/lib/libexample.so", bytes(header) + b"changed", 0o644)])]
        self.write_images()
        with self.assertRaisesRegex(ValueError, "changes a deployed ELF"):
            self.validate()

    def test_debug_cannot_supply_a_separate_openssh_package(self):
        """Even identical FIPS bytes must come from the runtime handoff, not a debug override."""
        self.append_debug_package("openssh-client", "1.0+fips", [("usr/bin/ssh", self.ssh_elf, 0o755)])
        self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes()]
        self.write_images()
        with self.assertRaisesRegex(ValueError, "must inherit runtime package: openssh-client"):
            self.validate()

    def test_debug_cannot_mutate_the_runtime_openssh_elf(self):
        """Reject an unlisted overlay that changes the runtime FIPS client's deployed bytes."""
        self.debug_layers.append(layer([("usr/bin/ssh", self.ssh_elf + b"changed", 0o755)]))
        self.write_images()
        with self.assertRaisesRegex(ValueError, "debug image changes the runtime package payload"):
            self.validate()

    def test_debug_cannot_change_runtime_regular_file_permissions(self):
        """Linux symlink-mode normalization must not hide a permission change on an executable."""
        self.debug_layers.append(layer([("usr/bin/ssh", self.ssh_elf, 0o644)]))
        self.write_images()
        with self.assertRaisesRegex(ValueError, "debug image changes the runtime package payload"):
            self.validate()

    def test_runtime_cannot_use_the_ordinary_openssh_package(self):
        """An ordinary Debian client cannot satisfy the FIPS runtime handoff contract."""
        manifest = json.loads(self.runtime_handoff.read_bytes())
        manifest["packages"][-1]["version"] = "1.0"
        manifest["packages"][-1]["control_fields"]["Version"] = "1.0"
        self.runtime_handoff.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        with self.assertRaisesRegex(ValueError, "requires retained package openssh-client"):
            self.validate()

    def test_docker_archive_tag_config_and_gzip_header_are_checked(self):
        """Verify installer-facing archive tags, OCI configuration, layer bytes and reproducible gzip metadata."""
        path = self.root / "docker-example.gz"
        digest = self.write_archive(path)
        descriptors = subject.image(self.root / "runtime.oci")[1]["layers"]
        result = subject.docker_archive(path, "docker-example:latest", digest, descriptors)
        self.assertEqual(result["tag"], "docker-example:latest")
        with self.assertRaisesRegex(ValueError, "tag differs"):
            subject.docker_archive(path, "wrong:latest", digest, descriptors)
        raw = bytearray(path.read_bytes())
        raw[4:8] = (1).to_bytes(4, "little")
        path.write_bytes(raw)
        with self.assertRaisesRegex(ValueError, "gzip header"):
            subject.docker_archive(path, "docker-example:latest", digest, descriptors)
        self.write_archive(path, config_bytes=b"{}")
        with self.assertRaisesRegex(ValueError, "config differs"):
            subject.docker_archive(path, "docker-example:latest", digest, descriptors)
        self.write_archive(path, corrupt_layer=True)
        with self.assertRaisesRegex(ValueError, "layer differs"):
            subject.docker_archive(path, "docker-example:latest", digest, descriptors)

    def test_complete_mode_requires_all_overlay_inputs(self):
        """Reject a production validation request without the full declared overlay and archive evidence."""
        with self.assertRaisesRegex(ValueError, "complete validation requires"):
            subject.validate_images(self.root / "runtime.oci", self.root / "debug.oci", self.runtime_handoff,
                                    self.debug_handoff, self.runtime_manifest, self.debug_manifest, self.root, policy=self.policy,
                                    source_contract_path=self.source_contract_path, dpkg_filter_path=self.filter_path)

    def test_dpkg_filter_uses_the_repository_space_separated_rules(self):
        """Apply the real dpkg inclusion and exclusion rules when comparing imported payloads."""
        item = {"kind": "file"}
        files = {name: item for name in ("usr/share/doc/example/README", "usr/share/doc/example/copyright",
                                         "usr/share/man/man1/example.1", "usr/lib/libexample.so")}
        self.assertEqual(set(subject.dpkg_filtered(files, self.filter_path)),
                         {"usr/share/doc/example/copyright", "usr/lib/libexample.so"})

    def installation_links(self, *, target="/usr/bin/compiler-15", installed=True):
        path = self.root / "installation-links.tar"
        with tarfile.open(path, "w") as archive:
            for name, destination in (("usr/bin/compiler", "/etc/alternatives/compiler"),
                                      ("etc/alternatives/compiler", target)):
                member = tarfile.TarInfo(name)
                member.type, member.mode, member.linkname = tarfile.SYMTYPE, 0o777, destination
                archive.addfile(member)
        if installed:
            self.runtime_layers.append(layer([("usr/bin/compiler-15", b"compiler fixture", 0o755)]))
        self.runtime_layers.append(path.read_bytes())
        self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes()]
        self.write_images()
        return path

    def test_installation_links_resolve_against_both_final_images(self):
        """The links layer may resolve into a separately installed APT payload."""
        path = self.installation_links()
        result = self.validate(runtime_links_layer=path)
        self.assertEqual(result["runtime_link_count"], 2)
        self.assertEqual(result["runtime_links_layer_sha256"], subject.sha(path))

    def test_installation_links_reject_missing_targets_and_cycles(self):
        for target in ("/usr/bin/missing", "/usr/bin/compiler", "../../../outside"):
            with self.subTest(target=target):
                path = self.installation_links(target=target, installed=False)
                with self.assertRaisesRegex(ValueError, "runtime installation link has no final target"):
                    self.validate(runtime_links_layer=path)

    def test_debug_cannot_remove_an_installation_link_target(self):
        path = self.installation_links()
        self.debug_layers.append(layer([("usr/bin/.wh.compiler-15", b"", 0o644)]))
        self.write_images()
        with self.assertRaisesRegex(ValueError, "debug installation link has no final target"):
            self.validate(runtime_links_layer=path)

    def test_installation_link_metadata_must_survive_in_each_image(self):
        path = self.installation_links()
        replacement = self.root / "changed-link.tar"
        with tarfile.open(replacement, "w") as archive:
            member = tarfile.TarInfo("etc/alternatives/compiler")
            member.type, member.mode, member.gid = tarfile.SYMTYPE, 0o777, 104
            member.linkname = "/usr/bin/compiler-15"
            archive.addfile(member)
        for variant in ("debug", "runtime"):
            with self.subTest(variant=variant):
                if variant == "runtime":
                    self.runtime_layers.append(replacement.read_bytes())
                self.debug_layers = self.runtime_layers + [self.debug_payload.read_bytes(), replacement.read_bytes()]
                self.write_images()
                with self.assertRaisesRegex(ValueError, variant + " installation link layer differs"):
                    self.validate(runtime_links_layer=path)

    def vim_tools(self, *, target="/usr/bin/vim.basic", uid=0, replace_binary=False, whiteout=False):
        header = bytearray(64)
        header[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", header, 16, 3, 62)
        tiny = self.root / "vim-base.tar"
        tiny.write_bytes(layer([("usr/bin/vim", bytes(header) + b"tiny", 0o755)]))
        files = {}
        subject.apply_layer(tiny, files)
        files["usr/bin/vim.tiny"] = {"kind": "hardlink", "mode": 0o755, "uid": 0, "gid": 0,
                                    "linkname": "usr/bin/vim"}
        files.update({name: {"kind": "symlink", "mode": 0o777, "uid": 0, "gid": 0,
                             "linkname": "/usr/bin/vim.tiny"} for name in ALTERNATIVE_PATHS})
        path = self.root / "vim-tools.tar"
        with tarfile.open(path, "w") as archive:
            for name in ALTERNATIVE_PATHS:
                member = tarfile.TarInfo(name)
                member.type, member.mode, member.uid, member.linkname = tarfile.SYMTYPE, 0o777, uid, target
                archive.addfile(member)
            binaries = [("usr/bin/vim.basic", bytes(header) + b"basic")]
            if replace_binary:
                binaries.append(("usr/bin/vim", bytes(header) + b"replacement"))
            if whiteout:
                binaries.append(("usr/bin/.wh.vim.tiny", b""))
            for name, data in binaries:
                member = tarfile.TarInfo(name)
                member.mode, member.size = 0o755, len(data)
                archive.addfile(member, io.BytesIO(data))
        return path, files

    def test_debug_vim_alternatives_allow_only_the_declared_link_change(self):
        """Allow the declared editor alternative update while preserving the inherited executable."""
        path, files = self.vim_tools()
        old_binary = dict(files["usr/bin/vim.tiny"])
        with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
            subject.apply_layer(path, dict(files), checked_overlay=True)
        subject.apply_debug_tools_layer(path, files, alternatives=DEBUG_ALTERNATIVES)
        self.assertEqual(files["usr/bin/vim.tiny"], old_binary)
        for name in ALTERNATIVE_PATHS:
            self.assertEqual(files[name]["linkname"], "/usr/bin/vim.basic")

    def test_debug_vim_alternatives_reject_other_targets_or_owners(self):
        """Reject an alternative update that changes an undeclared target or file owner."""
        for kwargs in ({"target": "/usr/bin/other"}, {"uid": 1}):
            with self.subTest(kwargs=kwargs):
                path, files = self.vim_tools(**kwargs)
                with self.assertRaisesRegex(ValueError, "unexpected debug alternative"):
                    subject.apply_debug_tools_layer(path, files, alternatives=DEBUG_ALTERNATIVES)
        path, files = self.vim_tools()
        files["etc/alternatives/editor"]["linkname"] = "/usr/bin/other"
        with self.assertRaisesRegex(ValueError, "unexpected debug alternative"):
            subject.apply_debug_tools_layer(path, files, alternatives=DEBUG_ALTERNATIVES)

    def test_debug_vim_policy_preserves_binary_and_whiteout_checks(self):
        """An allowed alternative update cannot bypass ELF replacement or whiteout protections."""
        for kwargs, message in (({"replace_binary": True}, "change an inherited ELF"),
                                ({"whiteout": True}, "contains a whiteout")):
            with self.subTest(kwargs=kwargs):
                path, files = self.vim_tools(**kwargs)
                before = dict(files)
                with self.assertRaisesRegex(ValueError, message):
                    subject.apply_debug_tools_layer(path, files, alternatives=DEBUG_ALTERNATIVES)
                self.assertEqual(files, before)


    def vim_command_tools(self, *, new_target="/etc/alternatives/vim", replace_tiny=False):
        path, files = self.vim_tools()
        files["usr/bin/vim.tiny"] = files.pop("usr/bin/vim")
        files["usr/bin/vim"] = {"kind": "hardlink", "mode": 0o755, "uid": 0, "gid": 0,
                                "linkname": "usr/bin/vim.tiny"}
        with tarfile.open(path, "a") as archive:
            for name, target in (("etc/alternatives/vim", "/usr/bin/vim.basic"), ("usr/bin/vim", new_target)):
                member = tarfile.TarInfo(name)
                member.type, member.mode, member.linkname = tarfile.SYMTYPE, 0o777, target
                archive.addfile(member)
            if replace_tiny:
                member = tarfile.TarInfo("usr/bin/vim.tiny")
                member.mode, member.size = 0o755, 7
                archive.addfile(member, io.BytesIO(b"changed"))
        policy = [*DEBUG_ALTERNATIVES, {"paths": ["usr/bin/vim"], "before": "usr/bin/vim.tiny",
                  "before_kind": "hardlink", "before_mode": 0o755, "after": "/etc/alternatives/vim"}]
        return path, files, policy

    def test_declared_vim_command_replaces_hardlink_without_changing_tiny(self):
        path, files, policy = self.vim_command_tools()
        original = dict(files["usr/bin/vim.tiny"])
        with self.assertRaisesRegex(ValueError, "changes a base ELF link"):
            subject.apply_debug_tools_layer(path, dict(files), alternatives=DEBUG_ALTERNATIVES)
        subject.apply_debug_tools_layer(path, files, alternatives=policy)
        self.assertEqual(files["usr/bin/vim.tiny"], original)
        self.assertEqual(subject.oci_inventory.resolve_path("usr/bin/vim", files), "usr/bin/vim.basic")

    def test_vim_command_rejects_wrong_original_metadata_and_new_target(self):
        for key, value in (("mode", 0o777), ("uid", 1), ("kind", "symlink"), ("linkname", "usr/bin/other")):
            path, files, policy = self.vim_command_tools()
            files["usr/bin/vim"][key] = value
            with self.subTest(key=key), self.assertRaisesRegex(ValueError, "unexpected debug alternative"):
                subject.apply_debug_tools_layer(path, files, alternatives=policy)
        path, files, policy = self.vim_command_tools(new_target="/usr/bin/other")
        with self.assertRaisesRegex(ValueError, "unexpected debug alternative"):
            subject.apply_debug_tools_layer(path, files, alternatives=policy)

    def test_declared_hardlink_requires_same_architecture_binaries(self):
        path, files, policy = self.vim_command_tools()
        files["usr/bin/vim.tiny"]["elf_machine"] = 183
        with self.assertRaisesRegex(ValueError, "binaries for the expected architecture"):
            subject.apply_debug_tools_layer(path, files, alternatives=policy)

    def test_declared_vim_command_cannot_replace_inherited_binary(self):
        path, files, policy = self.vim_command_tools(replace_tiny=True)
        with self.assertRaisesRegex(ValueError, "change an inherited ELF"):
            subject.apply_debug_tools_layer(path, files, alternatives=policy)


class CachedValidateImageTest(ValidateImageTest):
    """Run the same success and rejection cases with scoped inventory reuse."""

    def validate(self, **kwargs):
        with validation_input_cache([self.root]):
            return super().validate(**kwargs)


class CachedUniqueLayerPathsTest(unittest.TestCase):
    def test_shared_layer_is_scanned_once_per_validation_scope(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            first, second = root / "first.tar", root / "second.tar"
            first.write_bytes(layer([("usr/share/file", b"contents", 0o644)]))
            second.write_bytes(first.read_bytes())
            layout = SimpleNamespace(layers=[first, second, first])
            with patch.object(subject, "_verify_unique_layer_paths",
                              wraps=subject._verify_unique_layer_paths) as scan:
                with validation_input_cache([root]):
                    subject.verify_unique_layer_paths(layout)
                    subject.verify_unique_layer_paths(layout)
                    self.assertEqual(scan.call_count, 1)
                subject.verify_unique_layer_paths(SimpleNamespace(layers=[first]))
                self.assertEqual(scan.call_count, 2)

    def test_duplicate_paths_are_rejected_on_every_attempt(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / "duplicate.tar"
            path.write_bytes(layer([("./file", b"one", 0o644), ("file", b"two", 0o644)]))
            with validation_input_cache([root]), \
                    patch.object(subject, "_verify_unique_layer_paths",
                                 wraps=subject._verify_unique_layer_paths) as scan:
                for _ in range(2):
                    with self.assertRaisesRegex(ValueError, "repeats a path"):
                        subject.verify_unique_layer_paths(SimpleNamespace(layers=[path]))
                self.assertEqual(scan.call_count, 2)

    def test_cached_success_still_rejects_changed_layer(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / "layer.tar"
            path.write_bytes(layer([("file", b"one", 0o644)]))
            with self.assertRaisesRegex(ValueError, "validation input changed"):
                with validation_input_cache([root]):
                    subject.verify_unique_layer_paths(SimpleNamespace(layers=[path]))
                    path.write_bytes(layer([("file", b"two", 0o644)]))
                    subject.verify_unique_layer_paths(SimpleNamespace(layers=[path]))


if __name__ == "__main__":
    OWNER_POLICIES, remaining = owner_policies()
    unittest.main(argv=[__file__, *remaining])
