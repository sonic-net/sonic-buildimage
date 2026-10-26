"""Check inherited source ELFs and their collected symbols in actual OCI layers."""

import ast
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import tarfile
import tempfile
import zlib

from tools.bazel.ci import artifact_validation as validation
from tools.bazel.oci.oci_inventory import apply_layer, resolve_path
from tools.bazel.oci.oci_layout import validate_layout

METADATA_NAMES = ("usr/share/sonic/bazel/base-packages.json",
                  "usr/share/sonic/bazel/config-engine-packages.json")
MACHINES = {"amd64": (62, "x86_64-linux-gnu"), "arm64": (183, "aarch64-linux-gnu")}
DIGEST = re.compile(r"[0-9a-f]{64}")
SOURCE_REVISION = re.compile(r"[0-9a-f]{40}")
FOREIGN_REVISION = re.compile(r"[0-9][A-Za-z0-9.+-]*\.sonic\.[0-9]+")
ROOT_TARGET = re.compile(r"@@//[A-Za-z0-9_./+-]*:[A-Za-z0-9_.+-]+")


def require(value, message):
    validation.require(value, message)


def workspace_identity(package, report, module_path):
    """Bind root-workspace provenance to its declared MODULE and actual TAR.

    The producer deliberately leaves an unversioned root module unstamped.
    Its payload digest and the consumer CI checkout identify those sources;
    an external repository must still have an immutable revision/version.
    """
    target = package.get("target", "")
    require(ROOT_TARGET.fullmatch(target), "workspace source must have a canonical root target")
    package_path = target.removeprefix("@@//").split(":", 1)[0]
    require(not package_path or all(part not in ("", ".", "..") for part in package_path.split("/")),
            "workspace source must have a canonical root target")
    require(module_path is not None, "workspace source requires the declared root MODULE")
    module_path = Path(module_path)
    calls = [node.value for node in ast.parse(module_path.read_text()).body
             if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call)
             and isinstance(node.value.func, ast.Name) and node.value.func.id == "module"]
    require(len(calls) == 1, "workspace source requires one root module declaration")
    fields = {item.arg: ast.literal_eval(item.value) for item in calls[0].keywords
              if item.arg in ("name", "version")}
    require(fields.get("name") == "sonic-buildimage" and not fields.get("version"),
            "workspace source requires the unversioned sonic-buildimage root module")
    identity = report.get("source_modules", {}).get("sonic-buildimage", {})
    require(identity.get("version") == "workspace" and
            identity.get("module_sha256") == validation.sha(module_path),
            "workspace source MODULE identity differs from the declared input")
    return {"package": package["package"], "target": target,
            "module_sha256": identity["module_sha256"],
            "payload_sha256": package["payload_sha256"],
            "checkout_identity": "consumer CI revision recorded separately"}


def inventory(layout):
    files, origins = {}, {}
    for index, layer in enumerate(layout.layers):
        entries = apply_layer(layer, files)
        origins = {name: origin for name, origin in origins.items() if name in files}
        origins.update({name: index for name in entries})
    return files, origins


def extract_selected(layout, files, origins, names, destination):
    """Copy only effective regular files; never materialize image links."""
    by_layer = {}
    for name in names:
        require(files.get(name, {}).get("kind") == "file", "expected a regular image file: " + name)
        require(validation.path_name(name) == name, "noncanonical image path: " + name)
        by_layer.setdefault(origins[name], set()).add(name)
    for index, selected in by_layer.items():
        found = set()
        with tarfile.open(layout.layers[index]) as archive:
            for member in archive:
                name = validation.path_name(member.name)
                if name not in selected:
                    continue
                require(member.isfile(), "selected image entry changed type: " + name)
                target = destination / name
                target.parent.mkdir(parents=True, exist_ok=True)
                with archive.extractfile(member) as source, target.open("wb") as output:
                    shutil.copyfileobj(source, output)
                # Match read permissions, including installed read-only ELFs;
                # inspection must not need to rewrite the input in place.
                target.chmod(files[name]["mode"] & 0o777)
                found.add(name)
        require(found == selected, "selected OCI files were not found in their effective layer")
    for name in names:
        require(validation.sha(destination / name) == files[name]["sha256"],
                "extracted OCI file differs from its inventory: " + name)


def same_file(expected, actual, name):
    require(actual.get("kind") == "file" and all(actual.get(key) == expected[key]
            for key in ("sha256", "mode", "uid", "gid")), "image changed declared source file: " + name)


def elf(path, readelf):
    with path.open("rb") as stream:
        header = stream.read(20)
    require(len(header) == 20 and header[:6] == b"\x7fELF\x02\x01", "expected ELF64: " + str(path))
    machine = struct.unpack_from("<H", header, 18)[0]
    notes = validation.command([readelf, "-n", str(path)])
    identifiers = re.findall(r"Build ID: ([0-9a-f]+)", notes)
    require(len(identifiers) == 1, "expected exactly one ELF build ID: " + str(path))
    sections = validation.command([readelf, "-SW", str(path)])
    dynamic = validation.command([readelf, "-dW", str(path)])
    sonames = re.findall(r"\(SONAME\).*\[([^\]]+)\]", dynamic)
    return {"machine": machine, "build_id": identifiers[0],
            "has_dwarf": ".debug_info" in sections or ".zdebug_info" in sections,
            "has_debuglink": ".gnu_debuglink" in sections,
            "soname": sonames[0] if sonames else None,
            "needed": sorted(set(re.findall(r"\(NEEDED\).*\[([^\]]+)\]", dynamic)))}


def validate_source_base(runtime_path, debug_path, *, metadata_paths, source_debug_tar,
                         architecture, replacing_runtime_tar=None, workspace_module=None,
                         readelf="readelf", objcopy="objcopy"):
    """Validate declared base/config metadata and source symbols in final images.

    Metadata paths are ordered base then config-engine. A replacing TAR may
    contain only the three existing Syncd source libraries. Their current bytes
    are checked here; the caller's native-package validator checks their symbols.
    This validates source provenance and symbol pairing, not FIPS certification.
    """
    require(architecture in MACHINES, "unsupported source-base architecture")
    machine, triplet = MACHINES[architecture]
    require(len(metadata_paths) == 2, "base and config-engine metadata are both required")
    runtime = validate_layout(Path(runtime_path), "linux/" + architecture)
    debug = validate_layout(Path(debug_path), "linux/" + architecture)
    require(debug.manifest["layers"][:len(runtime.layers)] == runtime.manifest["layers"],
            "debug image does not extend this runtime image")
    runtime_files, runtime_origins = inventory(runtime)
    debug_files, debug_origins = inventory(debug)
    native, reports, workspace_sources = {}, [], []
    for name, path in zip(METADATA_NAMES, metadata_paths):
        raw = Path(path).read_bytes()
        digest = hashlib.sha256(raw).hexdigest()
        for files in (runtime_files, debug_files):
            item = files.get(name, {})
            require(resolve_path(name, files) == name, "source metadata path is redirected: " + name)
            require(item.get("kind") == "file" and item.get("sha256") == digest,
                    "embedded source metadata differs from declared input: " + name)
        report = json.loads(raw)
        require(report.get("schema") == 1 and report.get("architecture") == architecture,
                "source metadata schema or architecture differs: " + name)
        require(report.get("dependency_check", {}).get("status") == "satisfied",
                "source metadata has unsatisfied package dependencies")
        for package in report["source_packages"]:
            revision = package.get("source_revision", "")
            require(package.get("package") and package.get("target") and
                    (SOURCE_REVISION.fullmatch(revision) or FOREIGN_REVISION.fullmatch(revision) or
                     revision == "workspace") and
                    DIGEST.fullmatch(package.get("payload_sha256", "")),
                    "source package lacks declared revision/payload provenance")
            if revision == "workspace":
                workspace_sources.append(workspace_identity(package, report, workspace_module))
            for item in package["native_files"]:
                path = item["path"]
                require(validation.path_name(path) == path and path not in native,
                        "unsafe or duplicate source ELF path: " + path)
                require(DIGEST.fullmatch(item.get("sha256", "")), "invalid source ELF hash: " + path)
                native[path] = {**item, "package": package["package"], "target": package["target"],
                                "version": package["version"], "source_revision": package["source_revision"],
                                "payload_sha256": package["payload_sha256"]}
        reports.append({"path": name, "sha256": digest})
    require(native, "metadata contains no source-built ELFs")

    replacements = {}
    if replacing_runtime_tar is not None:
        payload = validation.payload(Path(replacing_runtime_tar))
        allowed = re.compile(r"usr/lib/" + re.escape(triplet) + r"/(?:libswsscommon|libsairedis|libsaimetadata)\.so(?:\.[0-9]+)+")
        for name, item in payload.items():
            if "elf_machine" in item:
                require(allowed.fullmatch(name) is not None and item["elf_machine"] == machine,
                        "replacement is outside the three reviewed source libraries: " + name)
                require(resolve_path(name, runtime_files) == name and resolve_path(name, debug_files) == name,
                        "replacement source path is redirected: " + name)
                same_file(item, runtime_files.get(name, {}), name)
                same_file(item, debug_files.get(name, {}), name)
                replacements[name] = item
        require(replacements, "replacement TAR contains no reviewed source libraries")
    checked, replaced = {}, []
    for name, item in native.items():
        require(resolve_path(name, runtime_files) == name and resolve_path(name, debug_files) == name,
                "source ELF path is redirected: " + name)
        actual = runtime_files.get(name, {})
        if name in replacements and replacements[name]["sha256"] != item["sha256"]:
            replaced.append({"path": name, "inherited_sha256": item["sha256"],
                             "replacement_sha256": replacements[name]["sha256"]})
            continue
        same_file(item, actual, name)
        same_file(item, debug_files.get(name, {}), name)
        require(actual.get("elf_machine") == machine, "source ELF architecture differs: " + name)
        checked[name] = item

    symbols = validation.payload(Path(source_debug_tar))
    symbol_files = set()
    for name, item in symbols.items():
        if item["kind"] == "directory":
            continue
        require(item["kind"] == "file" and
                re.fullmatch(r"usr/lib/debug/\.build-id/[0-9a-f]{2}/[0-9a-f]+\.debug", name),
                "unexpected collected debug entry: " + name)
        require(resolve_path(name, debug_files) == name, "collected symbol path is redirected: " + name)
        same_file(item, debug_files.get(name, {}), name)
        require(item.get("elf_machine") == machine, "collected debug ELF architecture differs: " + name)
        symbol_files.add(name)
    require(symbol_files, "collected source debug TAR contains no ELF files")

    pairs = []
    with tempfile.TemporaryDirectory(prefix="sonic-source-base-") as temporary:
        root = Path(temporary)
        extract_selected(runtime, runtime_files, runtime_origins, checked, root / "runtime")
        extract_selected(debug, debug_files, debug_origins, symbol_files, root / "debug")
        for name, source in sorted(checked.items()):
            binary = root / "runtime" / name
            info = elf(binary, readelf)
            require(info["machine"] == machine and not info["has_dwarf"], "source runtime is unstripped or wrong architecture: " + name)
            identifier = info["build_id"]
            debug_name = "usr/lib/debug/.build-id/" + identifier[:2] + "/" + identifier[2:] + ".debug"
            require(debug_name in symbol_files, "source ELF lacks collected symbols: " + name)
            debug_file = root / "debug" / debug_name
            symbol_info = elf(debug_file, readelf)
            require(symbol_info["build_id"] == identifier and symbol_info["machine"] == machine and symbol_info["has_dwarf"],
                    "source symbols have different build ID/architecture or lack DWARF: " + name)
            sealed = name == "usr/lib/" + triplet + "/libsymcrypt.so.103.13.0"
            if sealed:
                require(source["package"] == "symcrypt-openssl" and source["version"] == "1.11.0" and
                        source["target"] in ("@sonic_fips//symcrypt:runtime", "@@sonic-fips+//symcrypt:runtime") and
                        SOURCE_REVISION.fullmatch(source["source_revision"]),
                        "sealed core lacks the reviewed source identity")
                require(not info["has_debuglink"], "sealed core was modified with a debuglink")
            else:
                require(info["has_debuglink"], "ordinary source ELF lacks a debuglink: " + name)
                link = root / "debuglink"
                validation.command([objcopy, "--dump-section", ".gnu_debuglink=" + str(link), str(binary), str(root / "inspection-copy")])
                raw = link.read_bytes()
                require(b"\0" in raw, "invalid debuglink filename: " + name)
                end = raw.index(0)
                offset = (end + 4) & ~3
                require(raw[:end].decode() == debug_file.name and len(raw) >= offset + 4 and
                        struct.unpack_from("<I", raw, offset)[0] == zlib.crc32(debug_file.read_bytes()),
                        "source debuglink filename or CRC differs: " + name)
            pairs.append({"path": name, "package": source["package"], **info,
                          "debug_path": debug_name, "source_revision": source["source_revision"],
                          "lookup": "build-id only: sealed source SymCrypt" if sealed else "build-id and debuglink CRC"})
    return {"_generated": {"notice": "AUTO-GENERATED. DO NOT EDIT MANUALLY.",
                           "generator": "tools/bazel/oci/source_base_validation.py"},
            "schema": 1, "status": "passed", "architecture": architecture,
            "runtime_manifest_digest": runtime.descriptor["digest"], "debug_manifest_digest": debug.descriptor["digest"],
            "metadata": reports, "workspace_sources": workspace_sources,
            "collected_debug_tar_sha256": validation.sha(Path(source_debug_tar)),
            "source_elf_count": len(native), "elf_count": len(checked), "paired_symbols": pairs,
            "external_needed": sorted({library for pair in pairs for library in pair["needed"]}),
            "replaced_source_files": replaced}
