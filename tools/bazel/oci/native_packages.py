#!/usr/bin/env python3
"""Verify OCI package SONAME links and debug symbols against owner expectations."""

import argparse
import json
from pathlib import Path, PurePosixPath
import posixpath
import re
import subprocess
import tarfile
import tempfile

from tools.bazel.ci.artifact_validation import elf_header, elf_info, path_name, require, sha, verify_debuglink
from tools.bazel.oci import package_payloads as validate_payloads
from tools.bazel.oci import package_contract
from tools.bazel.oci.package_contract import identity, read_contract, read_policy, validate_policy

ELF_MACHINES = {"amd64": 62, "arm64": 183, "armhf": 40}


def dynamic_function(path, symbol, *, undefined, readelf):
    """Check an actual exported/imported function, ignoring symbol version suffixes."""
    output = subprocess.check_output([readelf, "--dyn-syms", "--wide", str(path)],
                                     text=True, stderr=subprocess.PIPE, timeout=120)
    for line in output.splitlines():
        fields = line.split()
        if (len(fields) >= 8 and fields[3] == "FUNC" and fields[4] in ("GLOBAL", "WEAK") and
                fields[7].split("@", 1)[0] == symbol and (fields[6] == "UND") == undefined):
            return True
    return False


def debug_forwarding(name, spec, runtime, debug, info, *, readelf, objcopy):
    """Prove a source-free launcher imports its implementation from a packaged DSO."""
    require(isinstance(spec, dict) and set(spec) == {"provider", "symbol", "source"},
            "invalid debug forwarding contract: " + name)
    provider_name, symbol, source = spec["provider"], spec["symbol"], spec["source"]
    require(isinstance(provider_name, str) and path_name(provider_name) == provider_name and
            provider_name != name and isinstance(symbol, str) and re.fullmatch(r"[A-Za-z_]\w*", symbol) and
            isinstance(source, str) and source and PurePosixPath(source).name == source,
            "invalid debug forwarding provider, symbol or source: " + name)
    require(dynamic_function(runtime[name]["file"], symbol, undefined=True, readelf=readelf),
            "forwarding launcher does not import its declared function: " + name)
    provider = resolve(runtime, provider_name)
    require(provider.get("file") is not None, "forwarding provider is not a packaged ELF: " + provider_name)
    provider_info = elf_info(provider["file"], readelf=readelf, dynamic=True, timeout=120)
    require(provider_info["soname"] == PurePosixPath(provider_name).name and
            provider_info["soname"] in info["needed"],
            "forwarding provider is not a declared DT_NEEDED SONAME: " + name)
    require(dynamic_function(provider["file"], symbol, undefined=False, readelf=readelf),
            "forwarding provider does not define its declared function: " + provider_name)
    identifier = provider_info["build_id"]
    require(identifier and provider_info["has_debuglink"], "forwarding provider lacks debug identity: " + provider_name)
    debug_name = "usr/lib/debug/.build-id/" + identifier[:2] + "/" + identifier[2:] + ".debug"
    symbols = resolve(debug, debug_name)
    require(symbols.get("file") is not None, "forwarding provider has no matching symbols: " + provider_name)
    symbol_info = elf_info(symbols["file"], readelf=readelf, timeout=120)
    require(symbol_info["build_id"] == identifier and symbol_info["has_dwarf"],
            "forwarding provider symbols do not match its build ID or lack DWARF: " + provider_name)
    verify_debuglink(provider["file"], symbols["file"], provider_name,
                     expected_name=PurePosixPath(debug_name).name, objcopy=objcopy, timeout=120)
    return {**spec, "debug_path": debug_name, "build_id": identifier}


def collect(manifest_path, temporary, prefix):
    manifest = json.loads(manifest_path.read_bytes())
    files = {}
    count = 0
    with tarfile.open(manifest_path.parent / "payload.tar", "r:") as archive:
        members = iter(archive)
        for record in manifest["packages"]:
            for _ in range(record["payload_members"]):
                member = next(members, None)
                require(member is not None, "aggregate package payload ended before its recorded segments")
                normalized = validate_payloads.normalized_member(member)
                if normalized is None:
                    continue
                name = path_name(normalized.name)
                kind = ("file" if member.isfile() else "symlink" if member.issym() else
                        "hardlink" if member.islnk() else "directory" if member.isdir() else "other")
                item = {"kind": kind, "package": record["package"], "path": name}
                if kind in ("symlink", "hardlink"):
                    item["linkname"] = normalized.linkname
                elif kind == "file":
                    stream = archive.extractfile(member)
                    first = stream.read(64)
                    header = elf_header(first, name)
                    if header:
                        output = temporary / (prefix + "-" + str(count))
                        count += 1
                        with output.open("wb") as target:
                            target.write(first)
                            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                                target.write(chunk)
                        item["file"] = output
                        item.update(header)
                files[name] = item
        require(next(members, None) is None, "aggregate package payload has unrecorded members")
    return files


def collect_source(archive_path, receipt, temporary, variant):
    """Extract source ELFs; runtime owners come from the checked runtime receipt.

    Symbols come from the same shared collector used by Orchagent. Their owning
    runtime ELF is established below through build ID, DWARF and debuglink CRC,
    rather than duplicating a symbol inventory for every package in the receipt.
    """
    owners = {}
    if variant == "runtime":
        for package in receipt["packages"]:
            for name, metadata in package["files"].items():
                require(name not in owners or metadata["kind"] == "directory",
                        "source packages share a non-directory path: " + name)
                owners[name] = package
    files = {}
    with tarfile.open(archive_path, "r:") as archive:
        for index, member in enumerate(archive):
            name = path_name(member.name)
            require(variant == "debug" or name in owners, "source archive member has no package owner: " + name)
            kind = ("file" if member.isfile() else "symlink" if member.issym() else
                    "hardlink" if member.islnk() else "directory")
            item = {"kind": kind, "path": name, "origin": "bazel_source"}
            if variant == "runtime":
                package = owners[name]
                item.update(package=package["package"], source=package["source"],
                            source_runtime_input_tar_sha256=package["input_tar_sha256"])
            else:
                item["package"] = "source_symbols"
            if kind in ("symlink", "hardlink"):
                item["linkname"] = member.linkname
            elif kind == "file":
                stream = archive.extractfile(member)
                first = stream.read(64)
                header = elf_header(first, name)
                if header:
                    output = temporary / ("source-" + variant + "-" + str(index))
                    with output.open("wb") as target:
                        target.write(first)
                        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                            target.write(chunk)
                    item["file"] = output
                    item.update(header)
            files[name] = item
    return files


def merge_source(files, additions):
    """Source outputs must have one owner and cannot silently overwrite Make payloads."""
    for name, item in additions.items():
        require(name not in files or (item["kind"] == files[name]["kind"] == "directory"),
                "source package overlaps a Make payload: " + name)
        files[name] = item


def resolve(files, name):
    for _ in range(32):
        item = files.get(name)
        require(item is not None, "missing library or debug link target: " + name)
        if item["kind"] not in ("symlink", "hardlink"):
            return item
        target = item["linkname"]
        if target.startswith("/"):
            name = target.lstrip("/")
        elif item["kind"] == "hardlink":
            name = posixpath.normpath(target)
        else:
            name = posixpath.normpath(posixpath.join(posixpath.dirname(name), target))
        require(name != ".." and not name.startswith("../"), "link leaves the image root")
    raise ValueError("library or debug symlink cycle")


def imported_symbol_receipt(archive_path, receipt_path, runtime_digest, package_manifest=None, *, policy):
    """Bind the shared rule's selected files to this exact runtime and archive."""
    from tools.bazel.oci.oci_inventory import apply_layer
    receipt = json.loads(receipt_path.read_bytes())
    require(receipt.get("schema") == 1 and receipt.get("runtime_manifest") == runtime_digest and
            receipt.get("expected_platform") == "linux/" + policy["architecture"], "imported symbol receipt uses a different runtime")
    require(receipt.get("output") == {"sha256": sha(archive_path), "size": archive_path.stat().st_size},
            "imported symbol archive differs from its receipt")
    if package_manifest is not None:
        imported = json.loads(package_manifest.read_bytes())
        require(imported.get("schema") == 1 and imported.get("architecture") == policy["architecture"] and
                {item.get("package") for item in imported.get("packages", [])} ==
                set(policy.get("native", {}).get("imported_symbol_packages", [])) and
                len(imported.get("packages", [])) == len(policy.get("native", {}).get("imported_symbol_packages", [])), "invalid imported base debug package manifest")
        for package in imported["packages"]:
            require(package.get("control_fields", {}).get("Package") == package["package"] and
                    package.get("architecture") in (policy["architecture"], "all") and
                    re.fullmatch(r"[0-9a-f]{64}", package.get("source_sha256", "")),
                    "imported base symbols have an unexpected Debian package identity")
        require(receipt.get("sources") == [{"index": 0, "sha256": imported["payload"]["sha256"],
                                            "size": imported["payload"]["size"]}],
                "symbol candidates differ from the original Debian import")
        receipt["imported_packages"] = imported["packages"]
        receipt["package_manifest_sha256"] = sha(package_manifest)
    files = {}
    apply_layer(archive_path, files)
    expected = {}
    for pair in receipt["pairs"]:
        expected[pair["debug_path"]] = pair["debug_sha256"]
        for supplement in pair.get("supplements", []):
            expected[supplement["path"]] = supplement["sha256"]
    require(set(files) == set(receipt["selected_paths"]) == set(expected),
            "imported symbol receipt inventory differs")
    for name, digest in expected.items():
        require(files[name].get("kind") == "file" and files[name].get("sha256") == digest,
                "imported symbol file differs from its receipt: " + name)
    return receipt, files


def validate_inherited_base(runtime_path, symbols_tar, receipt_path, temporary, *, policy, readelf, objcopy,
                            package_manifest=None, required_paths=None):
    if required_paths is None:
        required_paths = policy.get("native", {}).get("required_imported_symbol_paths", [])
    """Independently check dynamic matches with binutils against deployed ELFs."""
    from tools.bazel.oci.oci_inventory import apply_layer
    from tools.bazel.oci.oci_layout import validate_layout
    runtime = validate_layout(runtime_path, "linux/" + policy["architecture"])
    receipt, _ = imported_symbol_receipt(symbols_tar, receipt_path, runtime.descriptor["digest"], package_manifest, policy=policy)
    pairs = receipt["pairs"]
    require(set(required_paths).issubset({pair["runtime_path"] for pair in pairs}),
            "required inherited runtime has no imported symbol pair")
    files = {}
    for layer in runtime.layers:
        apply_layer(layer, files)
    symbols = {}
    with tarfile.open(symbols_tar, "r:") as archive:
        for index, member in enumerate(archive):
            name = path_name(member.name)
            require(member.isfile(), "imported symbol is not a regular file")
            path = temporary / ("inherited-symbol-" + str(index))
            with archive.extractfile(member) as stream, path.open("wb") as output:
                for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                    output.write(chunk)
            symbols[name] = path
    result = []
    for index, pair in enumerate(pairs):
        name = pair["runtime_path"]
        require(files.get(name, {}).get("sha256") == pair["runtime_sha256"],
                "inherited runtime differs from the dynamic symbol receipt")
        binary = temporary / ("inherited-runtime-" + str(index))
        for layer in reversed(runtime.layers):
            with tarfile.open(layer, "r:*") as archive:
                matches = [member for member in archive if path_name(member.name) == name]
                if not matches:
                    continue
                member = matches[-1]
                require(member.isfile(), "inherited native runtime is not an ELF file")
                with archive.extractfile(member) as stream, binary.open("wb") as output:
                    for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                        output.write(chunk)
                break
        require(binary.is_file() and sha(binary) == pair["runtime_sha256"], "inherited runtime bytes differ")
        info = elf_info(binary, readelf=readelf, timeout=120)
        debug_name = pair["debug_path"]
        symbol = symbols[debug_name]
        debug_info = elf_info(symbol, readelf=readelf, timeout=120)
        require(info["build_id"] == debug_info["build_id"] == pair["build_id"] and
                info["has_debuglink"] and not info["has_dwarf"] and debug_info["has_dwarf"],
                "inherited runtime and symbols have different build IDs or invalid DWARF")
        verify_debuglink(binary, symbol, name, expected_name=PurePosixPath(debug_name).name,
                         objcopy=objcopy, timeout=120)
        previous = debug_name
        for number, supplement in enumerate(pair.get("supplements", [])):
            alternate = temporary / ("inherited-debugaltlink-" + str(index) + "-" + str(number))
            completed = subprocess.run([objcopy, "--dump-section", ".gnu_debugaltlink=" + str(alternate), str(symbols[previous])],
                                       capture_output=True, text=True, timeout=120)
            require(completed.returncode == 0 and alternate.is_file(), "inherited debug ELF lacks its DWZ link")
            link, identifier_bytes = alternate.read_bytes().split(b"\0", 1)
            target = posixpath.normpath(posixpath.join(posixpath.dirname(previous), link.decode())).lstrip("/")
            dwz_info = elf_info(symbols[supplement["path"]], readelf=readelf, timeout=120)
            require(target == supplement["path"] and identifier_bytes.hex() == dwz_info["build_id"] == supplement["build_id"] and
                    dwz_info["has_dwarf"], "inherited debug alternate link differs from its DWZ path or build ID")
            previous = target
        sections = subprocess.run([readelf, "-SW", str(symbols[previous])], capture_output=True, text=True, timeout=120)
        require(sections.returncode == 0 and ".gnu_debugaltlink" not in sections.stdout,
                "inherited symbols have an unrecorded DWZ supplement")
        result.append({"path": name, "origin": "inherited_base", **pair,
                       "runtime_manifest_digest": runtime.descriptor["digest"],
                       "filtered_payload_sha256": receipt["output"]["sha256"],
                       "input_archives": receipt["sources"],
                       "imported_packages": receipt.get("imported_packages", []),
                       "package_manifest_sha256": receipt.get("package_manifest_sha256")})
    return result


def validate_native(runtime_manifest, debug_manifest, *, policy, source_contract_path=None,
                    readelf="readelf", objcopy="objcopy", required_packages=None, required_sonames=None,
                    gap_packages=None, source_runtime_tar=None,
                    source_debug_tar=None, source_receipt=None, base_path=None, runtime_path=None,
                    base_debug_symbols=None, base_debug_receipt=None, base_debug_package_manifest=None, fixture=False):
    validate_policy(policy)
    expectations = policy.get("native", {})
    required_packages = set(expectations.get("required_packages", []) if required_packages is None else required_packages)
    required_sonames = expectations.get("required_sonames", {}) if required_sonames is None else required_sonames
    gap_packages = set(expectations.get("gap_packages", []) if gap_packages is None else gap_packages)
    architecture = policy["architecture"]
    require(architecture in ELF_MACHINES, "unsupported native ELF architecture: " + architecture)
    source_contract = read_contract(source_contract_path) if source_contract_path is not None else None
    if source_contract is not None:
        require(identity(source_contract) == identity(policy), "source contract and package policy identify different images")
    elif fixture:
        # Synthetic tests can exercise receipts without a checked owner contract.
        source_contract = {"schema": 1, **identity(policy), "packages": []}
    source_inputs = (source_runtime_tar, source_debug_tar, source_receipt)
    has_source = all(value is not None for value in source_inputs)
    require(fixture or source_contract_path is not None or
            not (has_source or expectations.get("require_source", False)),
            "native validation requires its source contract")
    require(has_source or not any(value is not None for value in source_inputs),
            "source runtime tar, debug tar, and receipt must be supplied together")
    require(fixture or not expectations.get("require_source", False) or has_source,
            "native validation requires source runtime tar, debug tar, and receipt")
    inherited_inputs = (runtime_path, base_debug_symbols, base_debug_receipt)
    has_inherited = all(value is not None for value in inherited_inputs)
    require(has_inherited or not any(value is not None for value in inherited_inputs),
            "runtime OCI, imported base symbols, and receipt must be supplied together")
    require(fixture or not expectations.get("require_imported_symbols", False) or (has_inherited and base_debug_package_manifest is not None),
            "native validation requires imported base symbols, package manifest, and runtime OCI")
    receipt = None
    source_debug_sha256 = sha(source_debug_tar) if has_source else None
    if has_source:
        receipt = package_contract.validate_receipt(
            source_receipt, source_runtime_tar, source_debug_tar,
            contract_path=source_contract_path, fixture=fixture)
        if receipt.get("inherited_files"):
            from tools.bazel.oci.oci_layout import validate_layout
            from tools.bazel.oci.oci_inventory import apply_layer
            require(base_path is not None, "inherited source files require the managed OCI base")
            base = validate_layout(base_path, "linux/" + architecture)
            require(base.descriptor["digest"] == receipt["base_manifest_digest"],
                    "inherited source files use a different OCI base")
            base_files = {}
            for layer in base.layers:
                apply_layer(layer, base_files)
            require(all(base_files.get(name) == expected for name, expected in receipt["inherited_files"].items()),
                    "inherited native base files differ from the source receipt")
    retained = policy["retained_source"] == "make"
    require((runtime_manifest is not None and debug_manifest is not None) == retained and
            (retained or runtime_manifest is debug_manifest is None),
            "retained handoff inputs differ from the image policy")
    require(retained or has_source, "native validation requires a retained or source runtime inventory")
    if retained:
        validate_payloads.validate(runtime_manifest, runtime_manifest.parent / "payload.tar", variant="runtime",
                                   policy=policy, source_contract=source_contract)
        validate_payloads.validate(debug_manifest, debug_manifest.parent / "payload.tar", variant="debug",
                                   runtime_manifest=runtime_manifest, policy=policy, source_contract=source_contract)
    with tempfile.TemporaryDirectory(prefix="oci-native-validation-") as temporary_name:
        temporary = Path(temporary_name)
        runtime = collect(runtime_manifest, temporary, "runtime") if retained else {}
        debug = collect(debug_manifest, temporary, "debug") if retained else {}
        if has_source:
            source_runtime = collect_source(source_runtime_tar, receipt, temporary, "runtime")
            source_debug = collect_source(source_debug_tar, receipt, temporary, "debug")
            merge_source(dict(runtime), source_debug)
            merge_source(dict(debug), source_runtime)
            merge_source(runtime, source_runtime)
            merge_source(debug, source_debug)
        require(runtime, "native validation received an empty runtime inventory")
        elfs = {name: item for name, item in runtime.items() if "file" in item}
        forwarding = expectations.get("debug_forwarding", {})
        require(isinstance(forwarding, dict) and set(forwarding).issubset(elfs),
                "debug forwarding contract names an absent runtime ELF")
        require(required_packages.issubset({item["package"] for item in elfs.values()}),
                "a required native runtime package has no ELF")
        basenames = {PurePosixPath(name).name for name in runtime}
        checked, gaps, embedded, paired = [], [], [], []
        observed_sonames = {}
        external_needed = set()
        runtime_ids = set()
        for name, item in sorted(elfs.items()):
            require(item["package"] in required_packages or item["package"] in gap_packages,
                    "unclassified native package ELF: " + item["package"] + ":" + name)
            require(item["elf_machine"] == ELF_MACHINES[architecture] or item["elf_type"] == 1,
                    "unexpected executable or shared library architecture: " + name)
            info = elf_info(item["file"], readelf=readelf, dynamic=True, timeout=120)
            record = {"path": name, "package": item["package"], **info}
            if item.get("origin") == "bazel_source":
                record.update({key: item[key] for key in
                               ("origin", "source", "source_runtime_input_tar_sha256")})
                record["source_debug_tar_sha256"] = source_debug_sha256
                require(info["build_id"] and info["has_debuglink"] and not info["has_dwarf"],
                        "source runtime ELF must be stripped with a build ID and debuglink: " + name)
            if info["soname"]:
                soname_path = str(PurePosixPath(name).parent / info["soname"])
                target = resolve(runtime, soname_path)
                require(target.get("file") is not None and target["file"].read_bytes() == item["file"].read_bytes(),
                        "SONAME path does not resolve to its packaged ELF: " + name)
                observed_sonames.setdefault(item["package"], set()).add(info["soname"])
            for needed in info["needed"]:
                if expectations.get("local_dependency_pattern") and re.match(expectations["local_dependency_pattern"], needed):
                    require(needed in basenames, "missing native runtime dependency " + needed + " for " + name)
                else:
                    external_needed.add(needed)
            identifier = info["build_id"]
            if identifier:
                runtime_ids.add(identifier)
            debug_name = ("usr/lib/debug/.build-id/" + identifier[:2] + "/" + identifier[2:] + ".debug") if identifier else None
            if debug_name and debug_name in debug:
                symbols = resolve(debug, debug_name)
                require(symbols.get("file") is not None, "debug path is not an ELF: " + debug_name)
                if item.get("origin") == "bazel_source":
                    require(symbols.get("origin") == "bazel_source",
                            "source runtime requires symbols from its source collector: " + name)
                symbol_info = elf_info(symbols["file"], readelf=readelf, timeout=120)
                require(symbol_info["build_id"] == identifier and (symbol_info["has_dwarf"] or name in forwarding),
                        "debug ELF does not match runtime build ID or lacks DWARF: " + name)
                if info["has_debuglink"]:
                    verify_debuglink(item["file"], symbols["file"], name,
                                     expected_name=PurePosixPath(debug_name).name, objcopy=objcopy, timeout=120)
                record["debug_path"] = debug_name
                if name in forwarding:
                    require(info["has_debuglink"] and not info["has_dwarf"] and not symbol_info["has_dwarf"],
                            "debug forwarding requires a stripped source-free launcher: " + name)
                    record["debug_forwarding"] = debug_forwarding(
                        name, forwarding[name], runtime, debug, info, readelf=readelf, objcopy=objcopy)
                paired.append(record)
            elif info["has_dwarf"]:
                embedded.append(record)
            else:
                require(item["package"] in gap_packages, "missing matching debug symbols for required runtime ELF: " + name)
                record["reason"] = "the existing Make debug list supplies no matching symbol package"
                gaps.append(record)
            checked.append(record)
        require(set(forwarding) == {record["path"] for record in paired if "debug_forwarding" in record},
                "debug forwarding requires matching launcher symbols")
        for package, soname in required_sonames.items():
            require(soname in observed_sonames.get(package, set()), "required SONAME is absent: " + package + ":" + soname)
        inherited_pairs = validate_inherited_base(runtime_path, base_debug_symbols, base_debug_receipt, temporary,
                                                  readelf=readelf, objcopy=objcopy, policy=policy,
                                                  package_manifest=base_debug_package_manifest) if has_inherited else []
        runtime_ids.update(record["build_id"] for record in inherited_pairs)
        unmatched = []
        for name in debug:
            match = re.fullmatch(r"usr/lib/debug/\.build-id/([0-9a-f]{2})/([0-9a-f]+)\.debug", name)
            if match and match[1] + match[2] not in runtime_ids:
                require(debug[name].get("origin") != "bazel_source", "source debug symbols have no runtime ELF: " + name)
                unmatched.append(name)
        return {"schema": 1, "runtime_manifest_sha256": sha(runtime_manifest) if retained else None,
                "debug_manifest_sha256": sha(debug_manifest) if retained else None,
                "source_receipt_sha256": sha(source_receipt) if has_source else None,
                "source_packages": receipt,
                "inherited_base_symbol_pairs": inherited_pairs,
                "elf_count": len(checked), "paired_symbols": paired, "embedded_symbols": embedded,
                "preserved_make_debug_gaps": gaps, "external_needed": sorted(external_needed),
                "base_debug_symbols_requiring_full_image_validation": sorted(unmatched),
                "remaining_checks": ["resolve external_needed in the actual OCI rootfs", "GDB source and line lookup in the actual debug image"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--policy", required=True, type=Path)
    parser.add_argument("--source-contract", type=Path)
    parser.add_argument("--runtime-manifest", type=Path)
    parser.add_argument("--debug-manifest", type=Path)
    parser.add_argument("--source-runtime-tar", type=Path)
    parser.add_argument("--source-debug-tar", type=Path)
    parser.add_argument("--source-receipt", type=Path)
    parser.add_argument("--base", type=Path)
    parser.add_argument("--runtime", type=Path)
    parser.add_argument("--base-debug-symbols", type=Path)
    parser.add_argument("--base-debug-receipt", type=Path)
    parser.add_argument("--base-debug-package-manifest", type=Path)
    parser.add_argument("--readelf", default="readelf")
    parser.add_argument("--objcopy", default="objcopy")
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    try:
        result = validate_native(args.runtime_manifest, args.debug_manifest, policy=read_policy(args.policy),
                                 source_contract_path=args.source_contract, readelf=args.readelf, objcopy=args.objcopy,
                                 source_runtime_tar=args.source_runtime_tar, source_debug_tar=args.source_debug_tar,
                                 source_receipt=args.source_receipt, base_path=args.base, runtime_path=args.runtime,
                                 base_debug_symbols=args.base_debug_symbols, base_debug_receipt=args.base_debug_receipt,
                                 base_debug_package_manifest=args.base_debug_package_manifest)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError, tarfile.TarError, subprocess.TimeoutExpired) as error:
        parser.exit(1, "OCI native package validation failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
