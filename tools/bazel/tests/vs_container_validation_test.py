"""Exercise VS consumer integrity using in-memory reference OCI descriptors and real archive bytes."""

from contextlib import nullcontext
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import vs_container_probe, vs_probe_policy, vs_standalone_probes
from tools.bazel.ci import vs_container_validation as validate


def digest(content):
    return hashlib.sha256(content).hexdigest()


class VSContainerValidationTest(unittest.TestCase):
    image = "docker-teamd"
    revision = "1" * 40
    version = "HEAD.0-1111111"

    def setUp(self):
        root = tempfile.TemporaryDirectory()
        self.addCleanup(root.cleanup)
        self.directory = Path(root.name)
        self.bundle = {
            "image": self.image, "revision": self.revision, "architecture": "amd64",
            "debug_enabled": False, "supports_debug": True, "build_version": self.version,
            "archives": {}, "fixtures": {},
        }
        self.add_image("runtime", [b"base", b"runtime"])

    def add_image(self, variant, layers, settings=None):
        name = self.image + ("-dbg" if variant == "debug" else "")
        config = {
            "rootfs": {"type": "layers", "diff_ids": ["sha256:" + digest(blob) for blob in layers]},
            "config": {"Labels": {"Tag": self.version}, "Entrypoint": ["/usr/bin/docker-teamd-init.sh"],
                       "Env": ["DEBIAN_FRONTEND=noninteractive"]},
            "os": "linux", "architecture": "amd64",
        }
        if settings is not None:
            config["config"] = settings
        raw = json.dumps(config).encode()
        blobs = {f"blobs/sha256/{digest(blob)}": blob for blob in layers}
        manifest = {"Layers": list(blobs), "RepoTags": [name + ":latest"], "Config": digest(raw) + ".json"}
        blobs[manifest["Config"]] = raw
        blobs["manifest.json"] = json.dumps([manifest]).encode()
        output = io.BytesIO()
        with tarfile.open(fileobj=output, mode="w") as archive:
            for path, value in blobs.items():
                entry = tarfile.TarInfo(path)
                entry.size = len(value)
                archive.addfile(entry, io.BytesIO(value))
        exported = gzip.compress(output.getvalue(), mtime=0)
        path = self.directory / (name + ".gz")
        path.write_bytes(exported)
        descriptors = [{"digest": "sha256:" + digest(blob), "size": len(blob)} for blob in layers]
        spec = {
            "file": path.name, "sha256": digest(exported), "config_digest": "sha256:" + digest(raw),
            "layers": descriptors, "rootfs": config["rootfs"],
        }
        self.bundle["archives"][variant] = spec
        oci = {"config": config, "config_digest": spec["config_digest"], "layers": descriptors, "files": {}}
        metadata = self.directory / (variant + ".oci.json.gz")
        metadata.write_bytes(gzip.compress(json.dumps(oci).encode()))
        self.bundle["fixtures"][metadata.name] = digest(metadata.read_bytes())

    def validate(self, debug=False, revision=None):
        (self.directory / "bundle.json").write_text(json.dumps(self.bundle))
        return validate.validate_artifacts(self.directory, self.image, revision or self.revision, debug, self.version)

    def test_runtime_only_needs_no_debug_inputs(self):
        self.assertEqual(set(self.validate()["archives"]), {"runtime"})

    def test_startup_contract_covers_each_final_dockerfile_stage(self):
        """The runner overrides the entrypoint, so validate every service's normal command separately."""
        families = set(vs_probe_policy.FAMILIES) | vs_standalone_probes.STANDALONE_IMAGES
        self.assertLessEqual(set(validate.ENTRYPOINTS), families)
        for image, executable in validate.ENTRYPOINTS.items():
            with self.subTest(image=image):
                dockerfile, = [file for base in (ROOT / "dockers", ROOT / "platform/vs")
                               if (file := base / image / "Dockerfile.j2").is_file()]
                final_stage = dockerfile.read_text().split("\nFROM ")[-1].splitlines()
                declared = [json.loads(line.split(" ", 1)[1]) for line in final_stage if line.startswith("ENTRYPOINT ")]
                self.assertEqual(declared, [[executable]] if executable else [])
                self.assertFalse(any(line.startswith(("CMD ", "EXPOSE ")) for line in final_stage))

    def test_runtime_only_rejects_broken_default_startup_even_when_shell_probes_would_run(self):
        settings = {"Labels": {"Tag": self.version}, "Entrypoint": ["/usr/bin/docker-teamd-init.sh"],
                    "Env": ["DEBIAN_FRONTEND=noninteractive"]}
        for changes, error in (({"Entrypoint": ["/bin/false"]}, "entrypoint"), ({"Cmd": ["--broken"]}, "command"),
                               ({"Env": []}, "environment"), ({"ExposedPorts": {"123/tcp": {}}}, "exposed ports")):
            with self.subTest(changes=changes):
                self.add_image("runtime", [b"runtime"], settings | changes)
                with self.assertRaisesRegex(ValueError, error):
                    self.validate()
        self.add_image("runtime", [b"runtime"], settings | {"Cmd": []})
        self.validate()  # An empty OCI command supplies no arguments to Teamd.

    def test_unknown_family_keeps_the_common_archive_checks(self):
        self.image = self.bundle["image"] = "docker-future-service"
        self.add_image("runtime", [b"runtime"], {"Labels": {"Tag": self.version}, "Entrypoint": ["/future"]})
        self.validate()
        self.bundle["archives"]["runtime"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "checksum"):
            self.validate()

    def test_service_inheritance_compares_all_parent_files_and_declared_replacements(self):
        """TAR ownership permits specific overrides, while changing an unrelated parent executable always fails."""
        base_file = {"kind": "file", "mode": 0o644, "uid": 0, "gid": 0, "sha256": digest(b"original")}
        changed = dict(base_file, sha256=digest(b"overwritten"))
        inherited = {name: {"kind": "symlink", "linkname": target} for name, target in
                     {"bin": "usr/bin", "sbin": "usr/sbin", "lib": "usr/lib", "var/run": "/run"}.items()}
        inherited.update(dict.fromkeys(("usr/bin/cat", "etc/group", "etc/passwd", "etc/gshadow",
                                        "etc/service.conf", "var/lib/dpkg/status"), base_file))
        (self.directory / "parent.group").write_text("root:x:0:\n")
        packages = {"libswsscommon": {"Version": "1.0"}, "sonic-supervisord-utilities-rs": {"Version": "2.0"}}
        labels = {"com.azure.sonic.versions." + name: item["Version"] for name, item in packages.items()}
        (self.directory / "config-engine.packages.json").write_text(json.dumps({"dependency_check": {"packages": packages}}))

        def write_inventory(variant, files, layers):
            data = {"files": files, "config": {"rootfs": {"diff_ids": layers}, "config": {"Labels": labels}}}
            (self.directory / (variant + ".oci.json.gz")).write_bytes(gzip.compress(json.dumps(data).encode()))

        write_inventory("parent", inherited, ["parent"])
        for image, parent, family in (("docker-swss-layer-trixie", "docker-config-engine-trixie", None),
                                      ("docker-teamd", "docker-swss-layer-trixie", "teamd"),
                                      ("docker-fpm-frr", "docker-swss-layer-trixie", "frr")):
            (self.directory / "parent.json").write_text(json.dumps({"image": parent}))
            for variant in ("runtime", "debug"):
                with self.subTest(image=image, variant=variant):
                    files = dict(inherited, **{"var/lib/dpkg/status": changed})
                    owners = {}
                    if family:
                        files["etc/service.conf"] = changed
                        owners[family + ".files.tar"] = {"etc/service.conf": changed}
                        owners[family + ".debug-tools.tar"] = {}
                    if family == "frr":
                        owners["frr.python.tar"] = {}
                    if family == "teamd" and variant == "debug":
                        files["etc/group"] = changed
                    (self.directory / (variant + ".owners.json")).write_text(json.dumps({"owners": owners}))
                    write_inventory(variant, files, ["parent", "service"])
                    groups = "root:x:0:\n" + ("_ssh:x:104:\n" if variant == "debug" else "")
                    with patch.object(vs_standalone_probes, "run", return_value=groups):
                        vs_standalone_probes.verify_full_parent(self.directory, {"image": image}, variant, "probe")
                    write_inventory(variant, files | {"usr/bin/cat": changed}, ["parent", "service"])
                    with self.assertRaisesRegex(ValueError, "changed inherited payload: usr/bin/cat"):
                        vs_standalone_probes.verify_full_parent(self.directory, {"image": image}, variant, "probe")

    def test_supervised_services_allow_only_their_declared_parent_file_replacements(self):
        original = {"kind": "file", "mode": 0o644, "uid": 0, "gid": 0, "sha256": digest(b"original")}
        changed = dict(original, sha256=digest(b"replacement"))
        inherited = {"usr/bin/cat": original, "etc/rsyslog.conf": original}
        inherited.update({name: {"kind": "symlink", "linkname": target} for name, target in
                          {"bin": "usr/bin", "sbin": "usr/sbin", "lib": "usr/lib", "var/run": "/run"}.items()})
        def write_inventory(variant, files):
            data = {"files": files, "config": {"rootfs": {"diff_ids": ["parent"]}}}
            (self.directory / (variant + ".oci.json.gz")).write_bytes(gzip.compress(json.dumps(data).encode()))
        write_inventory("parent", inherited)
        (self.directory / "parent.json").write_text(json.dumps({"image": "docker-config-engine-trixie"}))
        for image in vs_standalone_probes.SUPERVISED:
            for variant in ("runtime", "debug"):
                with self.subTest(image=image, variant=variant):
                    owners = {"service.files.tar": {"etc/rsyslog.conf": changed}}
                    (self.directory / (variant + ".owners.json")).write_text(json.dumps({"owners": owners}))
                    files = inherited | {"etc/rsyslog.conf": changed}
                    write_inventory(variant, files)
                    vs_standalone_probes.verify_full_parent(self.directory, {"image": image}, variant, "unused")
                    write_inventory(variant, files | {"usr/bin/cat": changed})
                    with self.assertRaisesRegex(ValueError, "changed inherited payload: usr/bin/cat"):
                        vs_standalone_probes.verify_full_parent(self.directory, {"image": image}, variant, "unused")

    def test_enabled_debug_extends_the_published_runtime(self):
        self.bundle["debug_enabled"] = True
        self.add_image("debug", [b"base", b"runtime", b"debug tools"])
        self.assertEqual(set(self.validate(True)["archives"]), {"runtime", "debug"})
        self.add_image("debug", [b"base", b"other runtime", b"debug tools"])
        with self.assertRaisesRegex(ValueError, "does not extend"):
            self.validate(True)

    def test_selected_debug_cannot_be_missing_or_silently_downgraded(self):
        with self.assertRaisesRegex(ValueError, "debug setting"):
            self.validate(True)
        self.bundle["debug_enabled"] = True
        with self.assertRaisesRegex(ValueError, "selected variants"):
            self.validate(True)
        self.bundle["supports_debug"] = False  # An explicitly runtime-only parent.
        self.assertEqual(set(self.validate(True)["archives"]), {"runtime"})

    def test_extra_debug_rejected_when_disabled(self):
        self.add_image("debug", [b"base", b"runtime", b"debug tools"])
        with self.assertRaisesRegex(ValueError, "selected variants"):
            self.validate()

    def test_nat_installer_profile_reuses_its_make_manifest_and_existing_probe(self):
        """Only selected NAT archives use the original Make settings and installed iptables/service probe."""
        self.image = self.bundle["image"] = "docker-nat"
        self.revision = self.bundle["revision"] = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
        manifests = self.directory / "make"
        expected = []
        for variant, name in (("runtime", self.image), ("debug", self.image + "-dbg")):
            self.bundle["debug_enabled"] = variant == "debug"
            manifest = {"service": {"name": name}}
            source = manifests / name / "manifest.json"
            source.parent.mkdir(parents=True)
            source.write_text(json.dumps(manifest))
            fixtures = vs_probe_policy.prepare(self.image, self.version, variant, None, {}, self.directory,
                                               make_manifests=manifests)
            for filename, path in fixtures.items():
                (self.directory / filename).write_bytes(path.read_bytes())
                self.bundle["fixtures"][filename] = digest(path.read_bytes())
            settings = {"Entrypoint": ["/usr/local/bin/supervisord"], "Env": ["DEBIAN_FRONTEND=noninteractive"],
                        "Labels": {"Tag": self.version, "com.azure.sonic.manifest": json.dumps(manifest)}}
            layers = [b"runtime"] if variant == "runtime" else [b"runtime", b"debug"]
            self.add_image(variant, layers, settings)
            bundle = self.validate(self.bundle["debug_enabled"])
            report = vs_container_probe.runtime_report(self.directory, bundle)
            expected.append(name)
            with patch.object(validate, "loaded_image", side_effect=lambda *args: nullcontext("fixture-tag")), \
                    patch.object(vs_standalone_probes, "run") as run:
                vs_standalone_probes.standalone(self.image, self.directory, bundle, report)
            self.assertEqual([call.args[2] for call in run.call_args_list], [["-s", "--", tag] for tag in expected])
            script = (ROOT / "dockers/docker-nat/verify-installed.sh").read_text()
            self.assertTrue(all(call.kwargs["stdin"] == script for call in run.call_args_list))

        for changed, message in ((dict(settings, Entrypoint=["/bin/false"]), "entrypoint"),
                                 (dict(settings, Cmd=["false"]), "command"),
                                 (dict(settings, Env=["DEBIAN_FRONTEND=readline"]), "environment")):
            with self.subTest(message=message):
                self.add_image("debug", layers, changed)
                with self.assertRaisesRegex(ValueError, message):
                    vs_container_probe.runtime_report(self.directory, self.validate(True))

    def test_pmon_probes_the_selected_installer_archives_and_checks_their_ancestry(self):
        image = validate.PMON
        (self.directory / "parent.json").write_text(json.dumps({
            "image": "docker-config-engine-trixie", "diff_ids": ["parent"], "files": {},
        }))
        archives = {}
        images = {}
        for variant, suffix, layers in (("runtime", "", ["parent", "pmon"]),
                                        ("debug", "-dbg", ["parent", "pmon", "debug-tools"])):
            name = image + suffix
            archives[variant] = {"file": name + ".gz"}
            images[name] = {"native": {"usr/bin/sensors": {}}}
            inventory = {"config": {"rootfs": {"diff_ids": layers}}, "files": {}}
            (self.directory / (variant + ".oci.json.gz")).write_bytes(gzip.compress(json.dumps(inventory).encode()))
            for enabled, arg in ((True, "y"), (False, "n")):
                bundle = {"image": image, "architecture": "amd64", "archives": archives, "pddf_enabled": enabled}
                with patch.object(validate, "loaded_image", side_effect=lambda *args: nullcontext("fixture-tag")), \
                        patch.object(vs_standalone_probes, "run") as run:
                    vs_standalone_probes.standalone(image, self.directory, bundle, {"images": images, "runtime_native": {}})
                    self.assertEqual([call.args[2][-2:] for call in run.call_args_list], [[v, arg] for v in archives])
                    self.assertIn("sonic-cfggen", run.call_args.kwargs["stdin"])
        (self.directory / "parent.json").write_text(json.dumps({
            "image": "docker-config-engine-trixie", "diff_ids": ["wrong-parent"], "files": {},
        }))
        with patch.object(validate, "loaded_image", side_effect=lambda *args: nullcontext("fixture-tag")), \
                self.assertRaisesRegex(ValueError, "did not inherit"):
            vs_standalone_probes.standalone(image, self.directory, bundle, {"images": images, "runtime_native": {}})

    def test_supervised_probes_follow_the_selected_runtime_and_debug_archives(self):
        """A normal installer job never tries to load or inspect an unselected debug companion."""
        for image in vs_standalone_probes.SUPERVISED:
            for debug in (False, True):
                with self.subTest(image=image, debug=debug):
                    variants = {"runtime": {"file": image + ".gz"}}
                    if debug:
                        variants["debug"] = {"file": image + "-dbg.gz"}
                    bundle = {"image": image, "archives": variants, "architecture": "amd64"}
                    report = {"images": {row["file"].removesuffix(".gz"): {} for row in variants.values()},
                              "runtime_native": {}}
                    output = json.dumps({"image": image, "status": "passed"})
                    with patch.object(validate, "loaded_image", side_effect=lambda *args: nullcontext("fixture")), \
                            patch.object(vs_standalone_probes, "verify_full_parent"), \
                            patch.object(vs_standalone_probes, "verify_watchdog_symbols", return_value={}) as symbols, \
                            patch.object(vs_standalone_probes, "run", return_value=output) as run:
                        vs_standalone_probes.standalone(image, self.directory, bundle, report)
                    self.assertEqual(run.call_count, len(variants))
                    self.assertEqual(symbols.call_count, int(debug and image in vs_standalone_probes.WATCHDOGS))
                    for row in variants.values():
                        receipt = self.directory / (row["file"].removesuffix(".gz") + ".runtime.json")
                        self.assertEqual(json.loads(receipt.read_text())["status"], "passed")

    def test_wrong_revision_or_release_fails_before_probing(self):
        with self.assertRaisesRegex(ValueError, "checkout"):
            self.validate(revision="2"*40)
        self.bundle["build_version"] = "some-other-build"
        with self.assertRaisesRegex(ValueError, "wrong VS release"):
            self.validate()

    def test_rejects_archive_with_wrong_transport_hash(self):
        self.bundle["archives"]["runtime"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "VS archive checksum"):
            self.validate()

    def test_checks_layers_against_the_producer_layout(self):
        spec = self.bundle["archives"]["runtime"]
        oci = validate.read_oci(self.directory, "runtime")
        spec["layers"].reverse()
        oci["layers"] = spec["layers"]
        encoded = gzip.compress(json.dumps(oci).encode())
        (self.directory / "runtime.oci.json.gz").write_bytes(encoded)
        self.bundle["fixtures"]["runtime.oci.json.gz"] = digest(encoded)
        with self.assertRaisesRegex(ValueError, "Docker archive layer differs"):
            self.validate()

    def test_rejects_swapped_fixture_and_references_outside_artifact(self):
        fixture = self.directory / "test-redis.tar"
        fixture.write_bytes(b"new version")
        self.bundle["fixtures"][fixture.name] = digest(b"original version")
        with self.assertRaisesRegex(ValueError, "fixture checksum"):
            self.validate()
        del self.bundle["fixtures"][fixture.name]
        self.bundle["fixtures"]["../other-test-redis.tar"] = digest(b"")
        with self.assertRaisesRegex(ValueError, "Invalid VS artifact filename"):
            self.validate()

    def test_releases_private_docker_tag_on_probe_failure(self):
        spec = self.bundle["archives"]["runtime"]
        config = validate.read_oci(self.directory, "runtime")["config"]
        inspected = {"Id": spec["config_digest"], "Config": config["config"], "Os": "linux",
                     "Architecture": "amd64", "RootFS": {"Type": "layers", "Layers": config["rootfs"]["diff_ids"]}}
        with patch.object(validate.validate_runtime, "load_archive", return_value=(inspected, config, "")), \
                patch.object(validate.subprocess, "run") as cleanup:
            with self.assertRaisesRegex(RuntimeError, "service failed"):
                with validate.loaded_image(self.directory, spec, self.bundle) as tag:
                    self.assertTrue(tag.startswith("sonic-vs-probe:"))
                    raise RuntimeError("service failed")
            self.assertEqual(cleanup.call_args.args[0], ["docker", "image", "rm", tag])


if __name__ == "__main__":
    unittest.main()
