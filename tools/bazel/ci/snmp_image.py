#!/usr/bin/env python3
"""Build SNMP artifacts, then independently validate the completed OCI outputs."""

import argparse
from functools import partial
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, bazel_commands, build, command_log, resolution
from tools.bazel.ci import source_image_worker
from tools.bazel.oci import image_validation, oci_layout, source_base_validation

OWNER = "//dockers/docker-snmp:"
IMAGES = ("docker-snmp", "docker-snmp-dbg")
TARGETS = {
    "test-redis.tar": OWNER + "test_redis",
    **{name + ".gz": OWNER + name + ".gz" for name in IMAGES},
    "snmp.debug.tar": OWNER + "all_debug_symbols",
    "snmp.packages.json": OWNER + "runtime_package_metadata",
    "snmp.status": OWNER + "installed_packages",
    "snmp-debug.status": OWNER + "debug_installed_packages",
    "snmp-debug.packages.json": OWNER + "debug_package_metadata",
    "base.packages.json": "//dockers/docker-base-trixie:runtime_package_metadata",
    "config-engine.packages.json": "//dockers/docker-config-engine-trixie:runtime_package_metadata",
    "base.source.tar": "//dockers/docker-base-trixie:source_runtime",
    "config-engine.source.tar": "//dockers/docker-config-engine-trixie:source_runtime",
    "openssh.source.tar": OWNER + "debug_client",
    "snmp_install.tar": "@sonic_snmpagent//:snmp_install",
    "asyncsnmp-2.1.0-py3-none-any.whl": "@sonic_snmpagent//:sonic_snmpagent_wheel",
    "sonic_platform_common-1.0-py3-none-any.whl": "@sonic_platform_common//:sonic_platform_common_wheel",
}


def build_images(args, directory):
    check.require(platform.machine() == "x86_64", "SNMP image build is native AMD64 only")
    receipt = {"status": "building", "architecture": "amd64", "commands": [],
               "invocation": {"bazel": args.bazel,
                              "review_registry_revision": os.environ.get("SONIC_BAZEL_REGISTRY_REVISION")}}
    execute = partial(command_log.execute, cwd=ROOT)
    options = ["--jobs=2", "--local_resources=cpu=2", "--local_resources=memory=8000",
               "--//dockers/docker-snmp:image_version=" + args.image_version]
    try:
        receipt["revision"] = execute(["git", "rev-parse", "HEAD"], directory, receipt, "revision").strip()
        headers = subprocess.check_output(["git", "cat-file", "-p", "HEAD"], cwd=ROOT, text=True).split("\n\n", 1)[0]
        receipt["tested_revision_parents"] = [line.split()[1] for line in headers.splitlines() if line.startswith("parent ")]
        receipt["image_version"] = args.image_version
        execute(["make", "-f", "tools/bazel/prepare_manifests.mk",
                 "MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-snmp.mk",
                 "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs",
                 "CONFIGURED_ARCH=amd64", "INCLUDE_FIPS=y", "ENABLE_ASAN=n", "DBG_IMAGE_MARK=dbg", "DOCKERS_PATH=dockers"],
                directory, receipt, "make-manifests")
        # Cached archives need not materialize the parent OCI directory that
        # validation reads. Audit and request every inspected layout explicitly.
        layouts = ["//dockers/docker-config-engine-trixie:docker-config-engine-trixie",
                   *[OWNER + name for name in IMAGES]]
        receipt["action_audit"] = bazel_commands.audit_targets(args.bazel, options, [*TARGETS.values(), *layouts],
            workspace=ROOT, output=directory / "action-audit.json")
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=options)
        execute([args.bazel, "build", *options, *layouts], directory, receipt, "layouts")
        receipt["layouts"] = {name: execute([args.bazel, "cquery", *options, "--output=files", OWNER + name],
            directory, receipt, name + ".layout").strip() for name in IMAGES}
        receipt["base_layout"] = execute([args.bazel, "cquery", *options, "--output=files", "//dockers/docker-config-engine-trixie:docker-config-engine-trixie"], directory, receipt, "base.layout").strip()
        receipt["resolution"] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        receipt["status"] = "built"
    except Exception as error:
        receipt.update(status="failed", error=str(error))
        raise
    finally:
        (directory / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


def wheel_payload(files, path):
    count = 0
    with zipfile.ZipFile(path) as wheel:
        for name in wheel.namelist():
            if name.endswith("/") or name.endswith(".dist-info/RECORD"):
                continue
            destination = "usr/local/lib/python3.13/dist-packages/" + name
            check.require(files.get(destination, {}).get("sha256") == hashlib.sha256(wheel.read(name)).hexdigest(),
                          "image differs from its source wheel: " + destination)
            count += 1
    return {"wheel_sha256": check.sha(path), "verified_files": count}


def validate_images(directory):
    receipt = json.loads((directory / "build-receipt.json").read_text())
    check.require(receipt["status"] == "built", "build did not complete")
    for name, artifact in receipt["artifacts"].items():
        check.require(check.sha(directory / name) == artifact["sha256"], "artifact changed: " + name)
    reports = [json.loads((directory / name).read_text()) for name in
               ("base.packages.json", "config-engine.packages.json", "snmp-debug.packages.json")]
    parent = oci_layout.validate_layout(ROOT / receipt["base_layout"], "linux/amd64")
    base_config = parent.config.get("config", {})
    inventories = {}
    for name in IMAGES:
        layout = oci_layout.validate_layout(ROOT / receipt["layouts"][name], "linux/amd64")
        image_validation.assert_ancestry(parent, layout, "SNMP image does not extend its expected parent: " + name)
        files = base_images.installed_inventory(layout)
        base_images.verify_directory_aliases(files)
        config = layout.config.get("config", {})
        check.require(config.get("Entrypoint") == ["/usr/bin/docker-snmp-init.sh"], "wrong SNMP entrypoint")
        env = dict(value.split("=", 1) for value in config.get("Env", []))
        check.require(all(env.get(key) == value for key, value in {
            "IMAGE_VERSION": receipt["image_version"], "PYTHONOPTIMIZE": "1", "DEBIAN_FRONTEND": "noninteractive"}.items()), "SNMP environment differs")
        check.require(config.get("ExposedPorts", {}) == base_config.get("ExposedPorts", {}), "final-stage exposed ports differ from inherited base")
        labels = config.get("Labels", {})
        check.require(labels.get("Tag") == receipt["image_version"], "Make image version label differs: " + name)
        config_packages = reports[1]["dependency_check"]["packages"]
        for component in ("libswsscommon", "sonic-supervisord-utilities-rs"):
            check.require(labels.get("com.azure.sonic.versions." + component) == config_packages[component]["Version"],
                          "missing or incorrect inherited component label: " + component)
        manifest = json.loads(labels["com.azure.sonic.manifest"])
        expected = json.loads((ROOT / "target/bazel-manifests" / name / "manifest.json").read_text())
        check.require(manifest == expected, "Make manifest labels changed")
        wheel_checks = {name: wheel_payload(files, directory / name) for name in TARGETS if name.endswith('.whl')}
        prefix = 'snmp' if name == 'docker-snmp' else 'snmp-debug'
        check.require(files['var/lib/dpkg/status']['sha256'] == check.sha(directory / (prefix+'.status')), 'installed status differs from checked package metadata')
        check.require(files['usr/share/sonic/bazel/'+prefix+'-packages.json']['sha256'] == check.sha(directory / (prefix+'.packages.json')), 'installed package receipt differs')

        for path, expected in check.payload(directory / "snmp_install.tar").items():
            check.require(files.get(path) == expected, "SNMP install TAR differs: " + path)
        for source, destination in {
            "docker-snmp-init.sh": "usr/bin/docker-snmp-init.sh", "start.sh": "usr/bin/start.sh",
            "snmp_yml_to_configdb.py": "usr/bin/snmp_yml_to_configdb.py", "critical_processes": "etc/supervisor/critical_processes",
            **{path.name: "usr/share/sonic/templates/" + path.name for path in (ROOT / "dockers/docker-snmp").glob("*.j2")},
        }.items():
            check.require(files[destination]["sha256"] == check.sha(ROOT / "dockers/docker-snmp" / source), "startup input changed: " + source)
        check.require(all(files['var/lib/snmp'][k] == v for k, v in {'uid':101,'gid':102,'mode':0o755}.items()), "snmpd state ownership changed")
        selected, origins = source_base_validation.inventory(layout)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source_base_validation.extract_selected(layout, selected, origins, {'etc/passwd','etc/group'}, root)
            users = (root/'etc/passwd').read_text().splitlines()
            groups = (root/'etc/group').read_text().splitlines()
            check.require(any(line.startswith('Debian-snmp:x:101:102:') and line.endswith(':/var/lib/snmp:/bin/false') for line in users), 'snmpd account differs')
            check.require('Debian-snmp:x:102:' in groups, 'snmpd group differs')
        check.require(all(item.get('elf_machine',62) == 62 for item in files.values()), 'non-AMD64 ELF in image')
        result = {'status':'passed','rootfs':layout.config['rootfs'],
                  'config_digest':layout.manifest['config']['digest'],'parent_digest':parent.descriptor['digest'],'wheel_payloads':wheel_checks,
                  'manifest_digest':layout.descriptor['digest']}
        (directory/(name+'.validation.json')).write_text(json.dumps(result,indent=2)+'\n')
        parent = layout
        inventories[name] = files
    runtime, debug = (inventories[name] for name in IMAGES)
    for path, item in runtime.items():
        if 'elf_machine' in item:
            check.require(debug.get(path) == item, 'debug image replaced runtime ELF: '+path)
    native = base_images.verify_source_files(debug, reports, 62)
    pairs = base_images.verify_symbols(native, [directory/name for name in ('base.source.tar','config-engine.source.tar','openssh.source.tar')], directory/'snmp.debug.tar', directory)
    check.require(debug['usr/bin/ssh-agent']['gid'] == 104 and debug['usr/bin/ssh-agent']['mode'] == 0o2755, 'FIPS ssh-agent permissions changed')
    result = {'status':'passed','revision':receipt['revision'],'architecture':'amd64','images':list(IMAGES),
              'source_debug_pairs':pairs,'artifacts':receipt['artifacts']}
    (directory/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'status':'passed','source_elf_debug_pairs':len(pairs)}))


def incremental(args, directory):
    """Check no-op reuse, then edit/restore image and archive reproducibility."""
    baseline = json.loads((directory / 'build-receipt.json').read_text())
    options = ['--jobs=2', '--local_resources=cpu=2', '--local_resources=memory=8000',
               '--//dockers/docker-snmp:image_version=' + baseline['image_version']]
    def output(label):
        value = subprocess.check_output([args.bazel, 'cquery', *options, '--output=files', label], cwd=ROOT, text=True).strip()
        return ROOT / value
    archives = {name: output(OWNER + name + '.gz') for name in IMAGES}
    owners = {name: output(TARGETS[name]) for name in TARGETS if name.endswith('.whl')}
    base = output('//dockers/docker-config-engine-trixie:docker-config-engine-trixie')
    layouts = {name: ROOT / path for name, path in baseline['layouts'].items()}
    def snapshot():
        return {'images': {name: oci_layout.validate_layout(path, 'linux/amd64').descriptor['digest'] for name, path in layouts.items()},
                'archives': {name: check.sha(path) for name, path in archives.items()},
                'owners': {name: check.sha(path) for name, path in owners.items()},
                'base': oci_layout.validate_layout(base, 'linux/amd64').descriptor['digest']}
    # A cached archive need not materialize its intermediate OCI directories.
    # Request every inspected output explicitly, including after restoration.
    inspected_targets = [OWNER + name + suffix for name in IMAGES for suffix in ('.gz', '')]
    inspected_targets += ['//dockers/docker-config-engine-trixie:docker-config-engine-trixie']
    inspected_targets += [TARGETS[name] for name in owners]
    command = [args.bazel, 'build', *options, *inspected_targets]
    subprocess.run(command, cwd=ROOT, check=True)
    original = snapshot()
    check.require(all(original['archives'][name] == baseline['artifacts'][name + '.gz']['sha256'] for name in IMAGES),
                  'incremental baseline differs from the validated archives')
    subprocess.run(command, cwd=ROOT, check=True)
    unchanged = snapshot()
    check.require(unchanged == original, 'unchanged build altered an image, archive or source wheel')
    source = ROOT / 'dockers/docker-snmp/sysDescription.j2'
    saved = source.read_bytes()
    try:
        source.write_bytes(saved + b'\n{# Bazel incremental input probe #}\n')
        subprocess.run(command, cwd=ROOT, check=True)
        changed = snapshot()
        check.require(all(changed[k][name] != original[k][name] for k in ('images','archives') for name in IMAGES),
                      'template edit did not invalidate both images and archives')
        check.require(changed['owners'] == original['owners'] and changed['base'] == original['base'],
                      'Docker template edit changed source wheels or inherited base')
    finally:
        source.write_bytes(saved)
    subprocess.run(command, cwd=ROOT, check=True)
    restored = snapshot()
    check.require(restored == original, 'restoring the template did not reproduce original image/archive/owner bytes')
    result = {'status': 'passed', 'input': 'dockers/docker-snmp/sysDescription.j2',
              'baseline': original, 'unchanged': unchanged, 'changed': changed, 'restored': restored}
    (directory / 'incremental.json').write_text(json.dumps(result, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase',choices=('build','validate','incremental','publish'))
    parser.add_argument('--artifacts',type=Path,required=True)
    parser.add_argument('--bazel',default='bazel')
    parser.add_argument('--image-version',default='bazel-snmp-ci')
    args=parser.parse_args(); directory=args.artifacts.resolve(); directory.mkdir(parents=True,exist_ok=True)
    if args.phase == 'build': build_images(args,directory)
    elif args.phase == 'validate': validate_images(directory)
    elif args.phase == 'incremental': incremental(args,directory)
    else:
        source_image_worker.publish(args)

if __name__=='__main__': main()
