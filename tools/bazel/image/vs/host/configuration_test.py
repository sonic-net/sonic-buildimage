"""Check observable VS installer configuration against its source contracts."""
import hashlib
import copy
import io
import json
import os
from pathlib import Path
import tarfile
import tempfile
import unittest

from configuration import build, _shutdown_order


def layer(path, entries):
    with tarfile.open(path, "w") as archive:
        for name, content in entries.items():
            data = content.encode() if isinstance(content, str) else content
            info = tarfile.TarInfo(name)
            info.mode, info.size = 0o644, len(data)
            archive.addfile(info, io.BytesIO(data))


def layout(directory, manifest, entries):
    directory.mkdir()
    blobs = directory / "blobs/sha256"
    blobs.mkdir(parents=True)

    def blob(data, media):
        digest = hashlib.sha256(data).hexdigest()
        (blobs / digest).write_bytes(data)
        return {"mediaType": media, "digest": "sha256:" + digest, "size": len(data)}

    payload = directory / "layer.tar"
    layer(payload, entries)
    descriptor = blob(payload.read_bytes(), "application/vnd.oci.image.layer.v1.tar")
    config = {"architecture": "amd64", "os": "linux", "rootfs": {"type": "layers", "diff_ids": [descriptor["digest"]]},
              "config": {"Labels": {"com.azure.sonic.manifest": json.dumps(manifest)}}}
    cfg = blob(json.dumps(config).encode(), "application/vnd.oci.image.config.v1+json")
    image = {"schemaVersion": 2, "config": cfg, "layers": [descriptor]}
    image_descriptor = blob(json.dumps(image).encode(), "application/vnd.oci.image.manifest.v1+json")
    (directory / "index.json").write_text(json.dumps({"schemaVersion": 2, "manifests": [image_descriptor]}))
    (directory / "oci-layout").write_text('{"imageLayoutVersion":"1.0.0"}')
    return cfg["digest"]


class ConfigurationTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        source = Path(os.environ.get("VS_HOST_TEST_WORKSPACE", Path(__file__).resolve().parents[5]))
        if "TEST_SRCDIR" in os.environ:
            source = Path(os.environ["TEST_SRCDIR"]) / os.environ["TEST_WORKSPACE"]
        templates = []
        for parent in ["files/build_templates", "files/build_templates/per_namespace", "files/build_templates/share_image"]:
            templates.extend((source / parent).glob("*.j2"))
        for path in ["files/image_config/rsyslog/rsyslog.d/00-sonic.conf.j2", "files/dhcp/dhclient.conf.j2",
                     "files/initramfs-tools/union-mount.j2", "files/initramfs-tools/arista-convertfs.j2"]:
            templates.append(source / path)
        if "TEST_SRCDIR" in os.environ:
            from python.runfiles import runfiles
            resolver = runfiles.Create()
            for name in ("sonic.service.j2", "service_mgmt.sh.j2", "dump.sh.j2"):
                templates.append(Path(resolver.Rlocation("sonic_utilities/sonic-utilities-data/templates/" + name)))
        else:
            templates.extend((source / "src/sonic-utilities/sonic-utilities-data/templates").glob("*.j2"))
        version = self.root / "version"
        version.write_text("test-2026\n")
        kernel = self.root / "kernel.json"
        kernel.write_text(json.dumps({"schema": 1, "architecture": "amd64", "kernel_release": "6.12-test-sonic"}))
        modules = self.root / "modules"
        modules.write_text("overlay\n")
        base = self.root / "base.tar"
        unit = "[Install]\nWantedBy=multi-user.target\n"
        layer(base, {"etc/debian_version": "13.1\n", "etc/shadow": "admin:hash-from-account-producer:1:0:99999:7:::\n",
                     "etc/group": "root:x:0:\nshadow:x:42:\n", "etc/initramfs-tools/modules": "loop\n",
                     "etc/default/kexec": "LOAD_KEXEC=true\n", "etc/systemd/networkd.conf": "#ManageForeignNextHops=yes\n",
                     "etc/init.d/nslcd": "# Required-Start: $remote_fs\n", "etc/ssh/sshd_config": "#ListenAddress ::\nUseDNS yes\n",
                     "usr/lib/systemd/system/systemd-networkd-wait-online.service": unit})
        self.metadata = {"schema": 1, "configuration": {"platform": "vs", "architecture": "amd64", "distribution": "trixie", "fips": True, "dash": True}, "containers": {}}
        for repository, name, local in [("docker-database", "database", False), ("docker-macsec", "macsec", True)]:
            manifest = {"version": "1.0.0", "package": {"name": name, "version": "1.0.0", "depends": []},
                        "service": {"name": name, "asic-service": False, "host-service": True},
                        "container": {"privileged": False, "volumes": [], "tmpfs": []}, "cli": {}}
            if local:
                manifest["service"]["requires"] = ["database"]
                manifest["cli"]["show"] = "/cli/show/plugin.py"
            self.metadata["containers"][repository] = {"manifest": manifest, "installed": True, "local_package": local,
                "run_options": "--cap-add=NET_ADMIN -t", "default_feature_owner": "", "default_feature_state_enabled": "y"}
        metadata = self.root / "containers.json"
        metadata.write_text(json.dumps(self.metadata))
        local_image = self.root / "macsec"
        self.image_id = layout(local_image, self.metadata["containers"]["docker-macsec"]["manifest"], {"cli/show/plugin.py": "# selected plugin\n"})
        context = {"commit_id": "declared-commit", "branch": "declared-branch", "build_date": "2026-11-16", "build_number": "1",
                   "built_by": "declared-builder", "sonic_os_version": "13", "username": "admin", "python_site_packages": "/usr/local/lib/python3.13/dist-packages",
                   "sonic_asic_platform": "vs", "ENABLE_FRR_SNMP_AGENT": "y", "enable_auto_tech_support": "y",
                   "FILESYSTEM_DOCKERFS": "dockerfs.tar.gz", "DOCKERFS_DIR": "docker", "DOCKER_RAMFS_SIZE": "3500M"}
        self.spec = {"containers": str(metadata), "image_version": str(version), "kernel_metadata": str(kernel), "base_layers": [str(base)],
                     "templates": {("sonic-utilities-data/templates/" + p.name if "sonic-utilities-data/templates/" in str(p) else str(p.relative_to(source))): str(p) for p in templates}, "local_images": {"docker-macsec": str(local_image)},
                     "image_repositories": {"docker-database": "docker-database", "docker-macsec": "docker-macsec"},
                     "units": {name: str(source / path) for name, path in {
                         "sed-pw-tpm-recovery.service": "files/image_config/sed_mgmt/sed-pw-tpm-recovery.service",
                         "chrony.service": "files/image_config/chrony/override.conf",
                         "set-epoch.timer": "files/image_config/systemd/set-epoch.timer",
                         "rasdaemon.timer": "files/image_config/rasdaemon/rasdaemon.timer",
                         "fstrim.timer": "files/image_config/fstrim/fstrim.timer", "sonic.target": "files/build_templates/sonic.target"}.items()},
                     "chrony_override": str(source / "files/image_config/chrony/override.conf"),
                     "modules": str(modules), "context": context, "debug": False, "generated_services": ["config-setup.service"],
                     "out": str(self.root / "config.tar"), "audit": str(self.root / "audit.json")}

    def produce(self):
        build(self.spec)
        return tarfile.open(self.spec["out"])

    def test_package_registration_binds_real_image_and_preserves_account_hash(self):
        with self.produce() as archive:
            read = lambda path: archive.extractfile(path).read()
            packages = json.loads(read("var/lib/sonic-package-manager/packages.json"))
            self.assertTrue(packages["database"]["built-in"])
            self.assertFalse(packages["macsec"]["built-in"])
            self.assertEqual(packages["macsec"]["image-id"], self.image_id)
            self.assertEqual(packages["macsec"]["docker-image-reference"], "docker-macsec:latest")
            self.assertEqual(read("usr/local/lib/python3.13/dist-packages/show/plugins/macsec.py"), b"# selected plugin\n")
            self.assertIn(b"Requires=database.service", read("usr/lib/systemd/system/macsec.service"))
            users = json.loads(read("etc/sonic/default_users.json"))
            self.assertEqual(users["admin"]["password"], "hash-from-account-producer")
            self.assertEqual((archive.getmember("etc/sonic/default_users.json").mode, archive.getmember("etc/sonic/default_users.json").gid), (0o600, 42))
            self.assertEqual(read("etc/initramfs-tools/modules"), b"loop\noverlay\n")
            self.assertEqual(archive.getmember("etc/systemd/system/sonic.target.wants/chrony.service").linkname, "/usr/lib/systemd/system/chrony.service")
            self.assertNotIn("etc/systemd/system/multi-user.target.wants/chrony.service", archive.getnames())
            self.assertEqual(archive.getmember("etc/systemd/system-generators/systemd-ssh-generator").linkname, "/dev/null")
            self.assertIn(b"AllowAgentForwarding no", read("etc/ssh/sshd_config"))

    def test_debug_selection_reaches_container_reference_and_version_metadata(self):
        self.spec["debug"] = True
        # one-image.mk selects registered debug archives and retains a runtime
        # when its family has no installed debug archive. docker-dash-engine.mk
        # explicitly keeps its debug companion outside the installer family.
        engine = copy.deepcopy(self.metadata["containers"]["docker-database"])
        engine["manifest"]["package"]["name"] = "dash-engine"
        engine["manifest"]["service"]["name"] = "dash_engine"
        self.metadata["containers"]["docker-dash-engine"] = engine
        Path(self.spec["containers"]).write_text(json.dumps(self.metadata))
        self.spec["image_repositories"] = {
            "docker-database": "docker-database-dbg",
            "docker-dash-engine": "docker-dash-engine",
            "docker-macsec": "docker-macsec-dbg",
        }
        # Debug OCI identity is independently selected, as real container
        # package versions differ from their runtime package versions.
        manifest = copy.deepcopy(self.metadata["containers"]["docker-macsec"]["manifest"])
        manifest["package"]["version"] = "1.0.0+dbg"
        debug_layout = self.root / "macsec-debug"
        layout(debug_layout, manifest, {"cli/show/plugin.py": "# selected plugin\n"})
        self.spec["local_images"]["docker-macsec"] = str(debug_layout)
        with self.produce() as archive:
            packages = json.load(archive.extractfile("var/lib/sonic-package-manager/packages.json"))
            self.assertEqual(packages["database"]["repository"], "docker-database-dbg")
            self.assertEqual(packages["macsec"]["docker-image-reference"], "docker-macsec-dbg:latest")
            self.assertIn(b"docker-database-dbg:latest", archive.extractfile("usr/bin/database.sh").read())
            self.assertEqual(packages["database"]["installed-version"], "1.0.0+dbg")
            self.assertEqual(packages["dash-engine"]["repository"], "docker-dash-engine")
            self.assertEqual(packages["dash-engine"]["installed-version"], "1.0.0")
            engine_script = archive.extractfile("usr/bin/dash_engine.sh").read()
            self.assertIn(b"docker-dash-engine:latest", engine_script)
            self.assertNotIn(b"docker-dash-engine-dbg", engine_script)
            version = archive.extractfile("etc/sonic/sonic_version.yml").read()
            self.assertIn(b"build_version: 'test-2026'", version)
            self.assertIn(b"kernel_version: '6.12-test-sonic'", version)

    def test_declared_build_identity_and_explicit_override_reach_version_file(self):
        identity = self.root / "identity.json"
        identity.write_text(json.dumps({"commit_id": "source-commit", "branch": "source-branch", "build_date": "declared UTC date", "build_number": "7", "built_by": "declared@builder"}))
        self.spec["build_identity"] = str(identity)
        for key in ("commit_id", "branch", "build_date", "build_number", "built_by"):
            self.spec["context"].pop(key)
        self.spec["context"]["branch"] = "explicit-branch"
        with self.produce() as archive:
            version = archive.extractfile("etc/sonic/sonic_version.yml").read()
            self.assertIn(b"commit_id: 'source-commit'", version)
            self.assertIn(b"branch: 'explicit-branch'", version)
            self.assertIn(b"built_by: declared@builder", version)
            users = json.load(archive.extractfile("etc/sonic/default_users.json"))
            self.assertEqual(users["admin"]["password"], "hash-from-account-producer")

    def test_local_package_manifest_mismatch_fails_before_publishing(self):
        self.metadata["containers"]["docker-macsec"]["manifest"]["package"]["version"] = "2.0.0"
        Path(self.spec["containers"]).write_text(json.dumps(self.metadata))
        with self.assertRaisesRegex(ValueError, "local OCI manifest differs"):
            build(self.spec)
        self.assertFalse(Path(self.spec["out"]).exists())

    def test_shutdown_contract_orders_dependencies_and_rejects_cycles(self):
        manifests = [{"service": {"name": name, "fast-shutdown": {"after": after, "before": []}}}
                     for name, after in [("database", []), ("swss", ["database"]), ("bgp", ["swss", "absent"])]]
        self.assertEqual(_shutdown_order(manifests, "fast"), "database swss bgp")
        manifests[0]["service"]["fast-shutdown"]["after"] = ["bgp"]
        with self.assertRaisesRegex(ValueError, "cyclic fast shutdown"):
            _shutdown_order(manifests, "fast")


if __name__ == "__main__":
    unittest.main()
