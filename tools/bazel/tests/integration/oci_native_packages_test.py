#!/usr/bin/env python3
"""Check native package validation with a real linked ELF and detached symbols.

Run directly with Python; the test uses the host C compiler and binutils and
creates no Debian packages.
"""

import copy
import hashlib
import io
import json
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.validation_inputs import validation_input_cache
from tools.bazel.oci import native_packages as subject
from tools.bazel.oci import package_payloads as validate_payloads
from tools.bazel.oci import package_contract as source_packages
from tools.bazel.tests.oci_contract_fixture import identity, owner_policies, package_policy, source_contract
from tools.bazel.tests.oci_base_fixture import digest, oci_files, tar_entries, write_layout


@unittest.skipUnless(platform.machine() == "x86_64" and all(shutil.which(name) for name in ("cc", "readelf", "objcopy")),
                     "native AMD64 C compiler and binutils are required")
class ValidateNativePackagesTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="oci-native-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.policy = package_policy()
        self.source_contract = source_contract()
        self.original, self.runtime, self.symbols, self.identifier = self.compile(42, "first")
        _, self.ssh_runtime, _, _ = self.compile(0, "ssh", executable=True)
        self.library_path = "usr/lib/x86_64-linux-gnu/libsample.so.0.0.0"
        self.soname_path = "usr/lib/x86_64-linux-gnu/libsample.so.0"
        self.debug_path = "usr/lib/debug/.build-id/" + self.identifier[:2] + "/" + self.identifier[2:] + ".debug"
        self.runtime_manifest, self.debug_manifest = self.handoffs()

    def compile(self, value, stem, *, executable=False, soname="libsample.so.0", function=None):
        source = self.root / (stem + ".c")
        function = function or ("main" if executable else "sample")
        source.write_text(f"int {function}(void) {{ return {value}; }}\n")
        original = self.root / (stem + ".unstripped")
        flags = [] if executable else ["-shared", "-fPIC", "-Wl,-soname," + soname]
        subprocess.run(["cc", *flags, "-g", "-O0", "-Wl,--build-id=sha1",
                        str(source), "-o", str(original)], capture_output=True, check=True)
        notes = subprocess.check_output(["readelf", "-n", str(original)], text=True)
        identifier = re.search(r"Build ID: ([0-9a-f]+)", notes)[1]
        symbols = self.root / stem / (identifier[2:] + ".debug")
        symbols.parent.mkdir()
        subprocess.run(["objcopy", "--only-keep-debug", str(original), str(symbols)], capture_output=True, check=True)
        runtime = self.root / (stem + ".runtime")
        subprocess.run(["objcopy", "--strip-debug", "--add-gnu-debuglink=" + str(symbols), str(original), str(runtime)],
                       capture_output=True, check=True)
        return original, runtime, symbols, identifier

    def handoff(self, variant, package, entries, runtime=None):
        directory = self.root / (variant + "-handoff")
        directory.mkdir(parents=True, exist_ok=True)
        payload = directory / "payload.tar"
        def tar_bytes(members):
            stream = io.BytesIO()
            with tarfile.open(fileobj=stream, mode="w", format=tarfile.GNU_FORMAT) as archive:
                for name, data, link in members:
                    entry = tarfile.TarInfo(name)
                    if link is not None:
                        entry.type = tarfile.SYMTYPE
                        entry.linkname = link
                        entry.mode = 0o777
                        archive.addfile(entry)
                    else:
                        entry.size = len(data)
                        entry.mode = 0o755 if name.startswith("usr/bin/") else 0o644
                        archive.addfile(entry, io.BytesIO(data))
            return stream.getvalue()
        packages = [(package, "1.0", entries)]
        if variant == "runtime":
            packages.append(("openssh-client", "1.0+fips", [("usr/bin/ssh", self.ssh_runtime.read_bytes(), None)]))
        records, combined = [], []
        for name, version, members in packages:
            data = tar_bytes(members)
            records.append({"package": name, "version": version, "architecture": "amd64",
                "source_deb": name + "_" + version + "_amd64.deb", "source_size": 1,
                "source_sha256": hashlib.sha256(name.encode()).hexdigest(),
                "control_sha256": hashlib.sha256((name + " control").encode()).hexdigest(),
                "control_fields": {"Package": name, "Version": version, "Architecture": "amd64"},
                "payload_sha256": hashlib.sha256(data).hexdigest(), "payload_size": len(data), "payload_members": len(members)})
            combined.extend(members)
        payload.write_bytes(tar_bytes(combined))
        digest = hashlib.sha256(payload.read_bytes()).hexdigest()
        value = {"schema": 1, "image": self.policy["image"], "variant": variant, "architecture": "amd64",
                 "distribution": "trixie", "features": dict(self.policy["features"]),
                 "required_packages": [record["package"] for record in records], "debug_apt_packages": [], "packages": records,
                 "payload": {"path": "payload.tar", "sha256": digest, "size": payload.stat().st_size, "members": len(combined)}}
        if runtime:
            value["runtime_manifest_sha256"] = hashlib.sha256(runtime.read_bytes()).hexdigest()
            value["debug_apt_packages"] = ["gdb", "gdbserver", "sshpass", "strace", "vim"]
        manifest = directory / "manifest.json"
        manifest.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
        return manifest

    def handoffs(self, *, library=None, symbols=None, symlink=True, package="libsample"):
        runtime_entries = [(self.library_path, (library or self.runtime).read_bytes(), None)]
        if symlink:
            runtime_entries.append((self.soname_path, None, "libsample.so.0.0.0"))
        runtime = self.handoff("runtime", package, runtime_entries)
        debug_entries = ([(self.debug_path, (symbols or self.symbols).read_bytes(), None)] if symbols is not False else
                         [("usr/share/dummy/data", b"no symbols\n", None)])
        debug = self.handoff("debug", package + "-dbgsym", debug_entries, runtime)
        return runtime, debug

    def validate(self, *, required={"libsample"}, sonames={"libsample": "libsample.so.0"}, gaps=set(), **kwargs):
        return subject.validate_native(self.runtime_manifest, self.debug_manifest,
                                       policy=self.policy, required_packages=required, required_sonames=sonames,
                                       gap_packages=gaps | {"openssh-client"}, fixture=True, **kwargs)

    def forwarding_fixture(self, *, corrupt_stub=False, wrong_provider=False, no_provider_dwarf=False, missing_needed=False):
        """Link an empty executable whose actual main lives in a shared library."""
        original, provider, provider_symbols, provider_id = self.compile(0, "forward_main", function="main")
        launcher = self.root / "launcher"
        subprocess.run(["cc", "-Wl,--build-id=sha1", "-Wl,--no-as-needed", str(original), "-o", str(launcher)],
                       check=True, capture_output=True)
        launcher_id = re.search(r"Build ID: ([0-9a-f]+)", subprocess.check_output(
            ["readelf", "-n", str(launcher)], text=True))[1]
        launcher_symbols = self.root / (launcher_id[2:] + ".debug")
        subprocess.run(["objcopy", "--only-keep-debug", str(launcher), str(launcher_symbols)], check=True, capture_output=True)
        subprocess.run(["objcopy", "--strip-debug", "--add-gnu-debuglink=" + str(launcher_symbols), str(launcher)],
                       check=True, capture_output=True)
        if missing_needed:
            subprocess.run(["objcopy", "--remove-section=.dynamic", str(launcher)], check=True, capture_output=True)
        if corrupt_stub:
            launcher_symbols.write_bytes(launcher_symbols.read_bytes() + b"corrupt")
        if wrong_provider:
            provider, provider_symbols, provider_id = self.runtime, self.symbols, self.identifier
        if no_provider_dwarf:
            subprocess.run(["objcopy", "--strip-debug", str(provider_symbols)], check=True, capture_output=True)
        provider_debug = "usr/lib/debug/.build-id/" + provider_id[:2] + "/" + provider_id[2:] + ".debug"
        self.runtime_manifest = self.handoff("runtime", "libsample", [
            (self.library_path, provider.read_bytes(), None),
            (self.soname_path, None, "libsample.so.0.0.0"),
            ("usr/bin/launcher", launcher.read_bytes(), None)])
        self.debug_manifest = self.handoff("debug", "libsample-dbgsym", [
            (provider_debug, provider_symbols.read_bytes(), None),
            ("usr/lib/debug/.build-id/" + launcher_id[:2] + "/" + launcher_id[2:] + ".debug",
             launcher_symbols.read_bytes(), None)], self.runtime_manifest)
        self.policy["native"]["debug_forwarding"] = {"usr/bin/launcher": {
            "provider": self.soname_path, "symbol": "main", "source": "forward_main.c"}}

    def test_source_free_launcher_has_explicit_matching_provider_symbols(self):
        self.forwarding_fixture()
        pairs = self.validate()["paired_symbols"]
        launcher = next(item for item in pairs if item["path"] == "usr/bin/launcher")
        self.assertTrue(launcher["has_debuglink"])
        self.assertEqual(launcher["debug_forwarding"]["symbol"], "main")
        self.assertEqual(launcher["debug_forwarding"]["provider"], self.soname_path)
        self.assertEqual(len(pairs), 2)

    def test_source_free_launcher_requires_the_explicit_contract(self):
        self.forwarding_fixture()
        self.policy["native"].pop("debug_forwarding")
        with self.assertRaisesRegex(ValueError, "lacks DWARF"):
            self.validate()

    def test_forwarding_keeps_launcher_crc_validation(self):
        self.forwarding_fixture(corrupt_stub=True)
        with self.assertRaisesRegex(ValueError, "debuglink CRC differs"):
            self.validate()

    def test_forwarding_requires_a_real_definition_in_the_provider(self):
        self.forwarding_fixture(wrong_provider=True)
        with self.assertRaisesRegex(ValueError, "provider does not define"):
            self.validate()

    def test_forwarding_requires_source_dwarf_in_the_provider(self):
        self.forwarding_fixture(no_provider_dwarf=True)
        with self.assertRaisesRegex(ValueError, "provider symbols.*lack DWARF"):
            self.validate()

    def test_forwarding_requires_the_imported_function(self):
        self.forwarding_fixture()
        self.policy["native"]["debug_forwarding"]["usr/bin/launcher"]["symbol"] = "missing"
        with self.assertRaisesRegex(ValueError, "launcher does not import"):
            self.validate()

    def test_forwarding_requires_a_linker_declared_provider_dependency(self):
        self.forwarding_fixture(missing_needed=True)
        with self.assertRaisesRegex(ValueError, "not a declared DT_NEEDED SONAME"):
            self.validate()

    def source_archives(self, *, symbols_mutation=None, overlap=False):
        """Build the declared owner tar and receipt around a real compiled sample ELF."""
        records = copy.deepcopy(self.source_contract["packages"])
        runtime_entries, debug_entries = [], []
        source_dir = self.root / "source"
        source_dir.mkdir()

        def write_tar(path, entries):
            with tarfile.open(path, "w") as archive:
                for name, data, link in entries:
                    member = tarfile.TarInfo(name)
                    member.mode = 0o644 if link is None else 0o777
                    if link is not None:
                        member.type, member.linkname = tarfile.SYMTYPE, link
                        archive.addfile(member)
                    else:
                        member.size = len(data)
                        archive.addfile(member, io.BytesIO(data))

        for index, record in enumerate(records):
            package = record["package"]
            soname = package + ".so.0"
            _, runtime, symbols, identifier = self.compile(100 + index, package, soname=soname)
            name = "usr/lib/x86_64-linux-gnu/" + soname + ".0.0"
            if overlap and index == 0:
                name = self.library_path
            symbol_bytes = symbols.read_bytes()
            if symbols_mutation and index == 0:
                symbol_bytes = symbols_mutation(symbol_bytes)
            runtime_members = [(name, runtime.read_bytes(), None),
                               ("usr/lib/x86_64-linux-gnu/" + soname, None, Path(name).name)]
            debug_members = [("usr/lib/debug/.build-id/" + identifier[:2] + "/" + identifier[2:] + ".debug",
                              symbol_bytes, None)]
            runtime_path, debug_path = source_dir / (package + ".tar"), source_dir / (package + "-debug.tar")
            write_tar(runtime_path, runtime_members)
            write_tar(debug_path, debug_members)
            record.pop("required_paths")
            record.update(input_tar_sha256=subject.sha(runtime_path), files=source_packages.inventory(runtime_path))
            runtime_entries.extend(runtime_members)
            debug_entries.extend(debug_members)
        runtime_tar, debug_tar = source_dir / "runtime.tar", source_dir / "debug.tar"
        write_tar(runtime_tar, runtime_entries)
        write_tar(debug_tar, debug_entries)
        receipt = {**identity(self.policy), "schema": source_packages.RECEIPT_SCHEMA,
                   "kind": "bazel_source", "packages": records,
                   "contract_sha256": hashlib.sha256(json.dumps(self.source_contract).encode()).hexdigest(),
                   "base_manifest_digest": "sha256:" + "b" * 64,
                   "module_file_sha256": {name: "a" * 64 for name in {record["source"]["module"] for record in records}}}
        receipt["payload"] = {"sha256": subject.sha(runtime_tar), "size": runtime_tar.stat().st_size,
                              "members": len(source_packages.inventory(runtime_tar))}
        receipt_path = source_dir / "receipt.json"
        receipt_path.write_text(json.dumps(receipt))
        return {"source_runtime_tar": runtime_tar, "source_debug_tar": debug_tar, "source_receipt": receipt_path}

    def source_validate(self, inputs):
        required = {"libsample", *source_packages.package_names(self.source_contract)}
        sonames = {name: name + ".so.0" for name in source_packages.package_names(self.source_contract)}
        return self.validate(required=required, sonames={"libsample": "libsample.so.0", **sonames}, **inputs)

    def test_source_libraries_keep_owner_provenance_and_matching_symbols(self):
        """The reused owner outputs join Make ELFs without losing source or symbol identity."""
        inputs = self.source_archives()
        result = self.source_validate(inputs)
        source_pairs = [item for item in result["paired_symbols"] if item.get("origin") == "bazel_source"]
        self.assertEqual({item["package"] for item in source_pairs}, set(source_packages.package_names(self.source_contract)))
        self.assertEqual(result["source_receipt_sha256"], subject.sha(inputs["source_receipt"]))
        for pair in source_pairs:
            self.assertEqual(pair["source"]["commit"], next(item["source"]["commit"] for item in
                             result["source_packages"]["packages"] if item["package"] == pair["package"]))

    def test_source_symbol_crc_is_checked_even_with_a_matching_receipt(self):
        """A receipt with updated hashes cannot disguise mismatched compiler debug output."""
        inputs = self.source_archives(symbols_mutation=lambda value: value + b"changed bytes")
        with self.assertRaisesRegex(ValueError, "debuglink CRC differs"):
            self.source_validate(inputs)

    def test_source_symbols_require_the_same_runtime_build_id(self):
        """An unrelated symbol ELF cannot satisfy a reused library's build-ID path."""
        inputs = self.source_archives(symbols_mutation=lambda value: self.symbols.read_bytes())
        with self.assertRaisesRegex(ValueError, "does not match runtime build ID"):
            self.source_validate(inputs)

    def test_source_tar_hash_and_make_path_ownership_are_checked(self):
        """Reject tampered owner archives and source paths already supplied by a Make package."""
        inputs = self.source_archives(overlap=True)
        with self.assertRaisesRegex(ValueError, "source package overlaps a Make payload"):
            self.source_validate(inputs)
        inputs["source_runtime_tar"].write_bytes(inputs["source_runtime_tar"].read_bytes() + b"changed")
        with self.assertRaisesRegex(ValueError, "source receipt archive hash differs"):
            self.source_validate(inputs)

    def test_production_make_only_validation_needs_no_source_contract(self):
        """Validate real imported ELF/debug pairs without inventing source metadata for a Make-only image."""
        self.policy["native"].update(require_source=False, require_imported_symbols=False)
        result = subject.validate_native(self.runtime_manifest, self.debug_manifest,
            policy=self.policy, required_packages={"libsample"},
            required_sonames={"libsample": "libsample.so.0"}, gap_packages={"openssh-client"})
        self.assertEqual(result["elf_count"], 2)
        self.assertEqual(result["paired_symbols"][0]["build_id"], self.identifier)
        self.assertIsNone(result["source_packages"])
        self.assertIsNone(result["source_receipt_sha256"])

    def test_source_receipt_in_production_still_requires_its_contract(self):
        """An optional source feature cannot bypass contract binding once source archives are supplied."""
        self.policy["native"].update(require_source=False, require_imported_symbols=False)
        inputs = self.source_archives()
        with self.assertRaisesRegex(ValueError, "requires.*source contract"):
            subject.validate_native(self.runtime_manifest, self.debug_manifest, policy=self.policy, **inputs)

    def test_production_native_validation_requires_all_source_inputs(self):
        """Complete validation cannot accidentally report only the remaining Make packages."""
        with self.assertRaisesRegex(ValueError, "native validation requires.*source"):
            subject.validate_native(self.runtime_manifest, self.debug_manifest, policy=self.policy)
        with self.assertRaisesRegex(ValueError, "must be supplied together"):
            self.validate(source_runtime_tar=self.runtime)

    def inherited_base_fixture(self, *, wrong_alternate_id=False):
        """Construct an OCI base ELF and its real debuglink/DWZ companion relationship."""
        _, _, dwz, dwz_id = self.compile(91, "dwz")
        dwz_name = "usr/lib/debug/.dwz/x86_64-linux-gnu/libexample.debug"
        alternate = self.root / "alternate-link"
        alternate.write_bytes(b"../../.dwz/x86_64-linux-gnu/libexample.debug\0" +
                              bytes.fromhex("0" * 40 if wrong_alternate_id else dwz_id))
        linked = self.symbols.with_suffix(".linked")
        subprocess.run(["objcopy", "--add-section", ".gnu_debugaltlink=" + str(alternate), str(self.symbols), str(linked)],
                       capture_output=True, check=True)
        linked.replace(self.symbols)
        subprocess.run(["objcopy", "--strip-debug", "--add-gnu-debuglink=" + str(self.symbols),
                        str(self.original), str(self.runtime)], capture_output=True, check=True)
        data = tar_entries([(self.library_path, self.runtime.read_bytes(), 0o644)])
        base_tar = self.root / "base.tar"
        base_tar.write_bytes(data)
        base = self.root / "base.oci"
        config = {"architecture": "amd64", "os": "linux", "rootfs": {"type": "layers", "diff_ids": [digest(data)]}}
        write_layout(base, oci_files(json.dumps(config).encode(), [data]))
        selected = self.root / "base-symbols.tar"
        selected.write_bytes(tar_entries([(self.debug_path, self.symbols.read_bytes(), 0o644),
                                          (dwz_name, dwz.read_bytes(), 0o644)]))
        receipt = {"schema": 1, "expected_platform": "linux/amd64",
                   "runtime_manifest": json.loads((base / "index.json").read_bytes())["manifests"][0]["digest"],
                   "output": {"sha256": subject.sha(selected), "size": selected.stat().st_size},
                   "selected_paths": [self.debug_path, dwz_name], "sources": [],
                   "pairs": [{"runtime_path": self.library_path, "runtime_sha256": subject.sha(self.runtime),
                              "build_id": self.identifier, "debug_path": self.debug_path,
                              "debug_sha256": subject.sha(self.symbols), "source": 0,
                              "supplements": [{"path": dwz_name, "build_id": dwz_id, "sha256": subject.sha(dwz)}]}]}
        receipt_path = self.root / "base-symbols.json"
        receipt_path.write_text(json.dumps(receipt))
        return base, selected, receipt_path

    def test_inherited_base_symbol_pair_keeps_its_dwz_companion(self):
        """The dynamic receipt retains a complete inherited pair with no pinned package snapshot."""
        base, selected, receipt = self.inherited_base_fixture()
        pairs = subject.validate_inherited_base(base, selected, receipt, self.root,
                                                policy=self.policy, readelf="readelf", objcopy="objcopy", required_paths=[self.library_path])
        self.assertEqual(len(pairs), 1)
        self.assertEqual(pairs[0]["build_id"], self.identifier)
        self.assertEqual(pairs[0]["origin"], "inherited_base")
        self.assertTrue(pairs[0]["supplements"][0]["build_id"])

    def test_inherited_base_symbols_reject_a_wrong_dwz_link(self):
        """Even valid archive hashes and debuglink CRC cannot admit an unrelated DWZ identifier."""
        base, selected, receipt = self.inherited_base_fixture(wrong_alternate_id=True)
        with self.assertRaisesRegex(ValueError, "alternate link differs"):
            subject.validate_inherited_base(base, selected, receipt, self.root,
                                             policy=self.policy, readelf="readelf", objcopy="objcopy", required_paths=[self.library_path])

    def test_inherited_symbol_receipt_is_bound_to_runtime_and_selected_bytes(self):
        """Reject an old-runtime receipt or a modified archive before trusting its selected paths."""
        base, selected, receipt_path = self.inherited_base_fixture()
        receipt = json.loads(receipt_path.read_bytes())
        with self.assertRaisesRegex(ValueError, "different runtime"):
            subject.imported_symbol_receipt(selected, receipt_path, "sha256:" + "0" * 64, policy=self.policy)
        selected.write_bytes(selected.read_bytes() + b"changed")
        with self.assertRaisesRegex(ValueError, "archive differs"):
            subject.imported_symbol_receipt(selected, receipt_path, receipt["runtime_manifest"], policy=self.policy)

    def test_inherited_symbols_cannot_hide_a_required_dwz_supplement(self):
        """A self-consistent receipt and TAR still fail when ELF references missing DWZ data."""
        base, selected, receipt_path = self.inherited_base_fixture()
        receipt = json.loads(receipt_path.read_bytes())
        receipt["pairs"][0]["supplements"] = []
        receipt["selected_paths"] = [self.debug_path]
        selected.write_bytes(tar_entries([(self.debug_path, self.symbols.read_bytes(), 0o644)]))
        receipt["output"] = {"sha256": subject.sha(selected), "size": selected.stat().st_size}
        receipt_path.write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "unrecorded DWZ"):
            subject.validate_inherited_base(base, selected, receipt_path, self.root,
                                             policy=self.policy, readelf="readelf", objcopy="objcopy", required_paths=[self.library_path])

    def test_imported_symbols_keep_original_deb_provenance(self):
        """Bind selected symbols to the importer's exact candidate TAR and original DEB record."""
        _, selected, receipt_path = self.inherited_base_fixture()
        receipt = json.loads(receipt_path.read_bytes())
        receipt["sources"] = [{"index": 0, "sha256": "a" * 64, "size": 12345}]
        receipt_path.write_text(json.dumps(receipt))
        package_manifest = self.root / "original-import.json"
        imported = {"schema": 1, "architecture": "amd64", "payload": {"sha256": "a" * 64, "size": 12345},
                    "packages": [{"package": "libexample-dbgsym", "architecture": "amd64",
                                  "control_fields": {"Package": "libexample-dbgsym"}, "source_sha256": "b" * 64}]}
        package_manifest.write_text(json.dumps(imported))
        verified, _ = subject.imported_symbol_receipt(selected, receipt_path, receipt["runtime_manifest"], package_manifest, policy=self.policy)
        self.assertEqual(verified["imported_packages"][0]["source_sha256"], "b" * 64)
        self.assertEqual(verified["package_manifest_sha256"], subject.sha(package_manifest))
        imported["payload"]["sha256"] = "c" * 64
        package_manifest.write_text(json.dumps(imported))
        with self.assertRaisesRegex(ValueError, "original Debian import"):
            subject.imported_symbol_receipt(selected, receipt_path, receipt["runtime_manifest"], package_manifest, policy=self.policy)

    def test_real_runtime_and_symbols_match(self):
        """Verify a real native ELF against its matching build ID, SONAME and detached debug file."""
        result = self.validate()
        self.assertEqual(result["elf_count"], 2)
        self.assertEqual(len(result["paired_symbols"]), 1)
        self.assertEqual(result["paired_symbols"][0]["build_id"], self.identifier)
        self.assertEqual(result["paired_symbols"][0]["soname"], "libsample.so.0")
        self.assertIn("openssh-client", self.policy["native"]["gap_packages"])
        self.assertEqual([item["package"] for item in result["preserved_make_debug_gaps"]], ["openssh-client"])
        ssh = result["preserved_make_debug_gaps"][0]
        self.assertFalse(ssh["has_dwarf"])
        self.assertTrue(ssh["has_debuglink"])
        self.assertIn("no matching symbol package", ssh["reason"])

    def test_missing_soname_link_is_rejected(self):
        """Require the runtime library link promised by the declared package ABI."""
        self.runtime_manifest, self.debug_manifest = self.handoffs(symlink=False)
        with self.assertRaisesRegex(ValueError, "missing library or debug link target"):
            self.validate()

    def test_different_debug_build_id_is_rejected(self):
        """Reject symbols produced for a different binary even if the archive has the expected path."""
        _, _, other, _ = self.compile(43, "second")
        self.runtime_manifest, self.debug_manifest = self.handoffs(symbols=other)
        with self.assertRaisesRegex(ValueError, "does not match runtime build ID"):
            self.validate()

    def test_debuglink_checksum_is_required(self):
        """A matching build-ID path cannot excuse a mismatched debuglink CRC."""
        altered = self.root / "altered.debug"
        altered.write_bytes(self.symbols.read_bytes() + b"changed debug bytes")
        self.runtime_manifest, self.debug_manifest = self.handoffs(symbols=altered)
        with self.assertRaisesRegex(ValueError, "debuglink CRC differs"):
            self.validate()

    def test_required_symbols_cannot_be_omitted(self):
        """Require matching detached symbols for every package selected for full debug coverage."""
        self.runtime_manifest, self.debug_manifest = self.handoffs(symbols=False)
        with self.assertRaisesRegex(ValueError, "missing matching debug symbols"):
            self.validate()

    def test_embedded_and_existing_make_gap_coverage_are_distinguished(self):
        """Report embedded DWARF separately from explicitly retained gaps in imported packages."""
        self.runtime_manifest, self.debug_manifest = self.handoffs(library=self.original, symbols=False, package="p4lang-pi")
        result = self.validate(required=set(), sonames={}, gaps={"p4lang-pi"})
        self.assertEqual(len(result["embedded_symbols"]), 1)
        self.assertEqual([item["package"] for item in result["preserved_make_debug_gaps"]], ["openssh-client"])
        self.runtime_manifest, self.debug_manifest = self.handoffs(symbols=False, package="p4lang-pi")
        result = self.validate(required=set(), sonames={}, gaps={"p4lang-pi"})
        self.assertEqual({item["package"] for item in result["preserved_make_debug_gaps"]}, {"p4lang-pi", "openssh-client"})
        self.assertEqual(result["embedded_symbols"], [])


class CachedValidateNativePackagesTest(ValidateNativePackagesTest):
    """Run the same success and rejection cases with scoped inventory reuse."""

    def validate(self, **kwargs):
        with validation_input_cache([self.root]):
            return super().validate(**kwargs)


if __name__ == "__main__":
    OWNER_POLICIES, remaining = owner_policies()
    unittest.main(argv=[__file__, *remaining])
