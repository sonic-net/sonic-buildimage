"""Generate the VS installer's template, service and package-registration effects.

Inputs are the same source templates, explicit build values, ordered installation
TARs and selected OCI layouts used by the product. Static files are installed by
static_configuration through tar.bzl; this action writes only derived files.
"""
import copy
from contextlib import ExitStack
import io
import json
from pathlib import Path, PurePosixPath
import re
import tarfile

from tools.bazel.image.vs.metadata.generate import render as render_container_metadata
from jinja2 import DictLoader, Environment
from merge_layers import Tree, apply_layer, split_path
from sonic_oci.files import read_files
from sonic_oci.layout import validate_layout


def _manifest_defaults(manifest):
    """Materialize defaults from sonic_package_manager.manifest.ManifestSchema."""
    result = copy.deepcopy(manifest)
    package = result.setdefault("package", {})
    for key, value in {"description": "", "init-cfg": {}, "debug-dump": ""}.items():
        package.setdefault(key, value)
    service = result.setdefault("service", {})
    for key in ("requires", "requisite", "wanted-by", "after", "before", "dependent", "dependent-of"):
        service.setdefault(key, [])
    for key, value in {"asic-service": False, "host-service": True, "delayed": False,
                       "check_up_status": False, "type": "", "stop-time": "",
                       "post-start-action": "", "pre-shutdown-action": ""}.items():
        service.setdefault(key, value)
    service.setdefault("syslog", {}).setdefault("support-rate-limit", False)
    for kind in ("warm-shutdown", "fast-shutdown"):
        service.setdefault(kind, {}).setdefault("after", [])
        service[kind].setdefault("before", [])
    container = result.setdefault("container", {})
    for key in ("cap-add", "security-opt", "ulimits", "devices", "volumes", "mounts", "tmpfs"):
        container.setdefault(key, [])
    container.setdefault("environment", {})
    result.setdefault("processes", [])
    cli = result.setdefault("cli", {})
    for key in ("show", "config", "clear"):
        value = cli.setdefault(key, [])
        if isinstance(value, str):
            cli[key] = [value] if value else []
    return result


class Generated:
    def __init__(self):
        self.entries = {}

    def file(self, path, content, mode=0o644, uid=0, gid=0):
        if isinstance(content, str):
            content = content.encode()
        self.entries[self.path(path)] = ("file", content, mode, uid, gid)

    def symlink(self, path, target):
        self.entries[self.path(path)] = ("link", target, 0o777, 0, 0)

    @staticmethod
    def path(path):
        value = PurePosixPath(path)
        if value.is_absolute() or ".." in value.parts or str(value) == ".":
            raise ValueError("invalid generated installation path: " + path)
        return str(value)

    def write(self, output):
        directories = set()
        for path in self.entries:
            directories.update(str(parent) for parent in PurePosixPath(path).parents if str(parent) != ".")
        with tarfile.open(output, "w", format=tarfile.PAX_FORMAT) as archive:
            for path in sorted(directories):
                info = tarfile.TarInfo(path)
                info.type, info.mode = tarfile.DIRTYPE, 0o755
                archive.addfile(info)
            for path, (kind, content, mode, uid, gid) in sorted(self.entries.items()):
                info = tarfile.TarInfo(path)
                info.mode, info.uid, info.gid = mode, uid, gid
                if kind == "link":
                    info.type, info.linkname = tarfile.SYMTYPE, content
                    archive.addfile(info)
                else:
                    info.size = len(content)
                    archive.addfile(info, io.BytesIO(content))


def _root_file(tree, path, required=True):
    parts = split_path(path)
    node = tree.lookup(parts)
    seen = set()
    while True:
        if node is None:
            if required:
                raise ValueError("required installed host input is absent: " + path)
            return None
        if not hasattr(node, "src"):
            raise ValueError("host configuration input is not a regular file: " + path)
        if not node.is_symlink():
            break
        location = tuple(parts)
        if location in seen:
            raise ValueError("host configuration input contains a symlink cycle: " + path)
        seen.add(location)
        target = node.linkname
        parts = split_path(target) if target.startswith("/") else parts[:-1] + target.split("/")
        node = tree.lookup(parts)
    if node.src is None:
        raise ValueError("host configuration input is not a regular file: " + path)
    if isinstance(node.src, bytes):
        return node.src
    archive, member = node.src
    return archive.extractfile(member).read()


def _enable(out, unit, text, installed):
    section = None
    values = {"WantedBy": [], "RequiredBy": [], "Alias": []}
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("["):
            section = stripped
        if section != "[Install]" or "=" not in stripped or stripped.startswith(("#", ";")):
            continue
        key, value = stripped.split("=", 1)
        if key in values:
            if not value:
                values[key] = []
            else:
                values[key].extend(value.split())
    for key, suffix in [("WantedBy", ".wants"), ("RequiredBy", ".requires")]:
        for target in values[key]:
            out.symlink("etc/systemd/system/" + target + suffix + "/" + unit, installed)
    for alias in values["Alias"]:
        out.symlink("etc/systemd/system/" + alias, installed)


def _shutdown_order(manifests, kind):
    services = {m["service"]["name"] for m in manifests}
    graph = {}
    for manifest in manifests:
        service = manifest["service"]
        after = set(service[kind + "-shutdown"]["after"]) & services
        before = set(service[kind + "-shutdown"]["before"]) & services
        if not after and not before:
            continue
        graph.setdefault(service["name"], set()).update(after)
        for name in before:
            graph.setdefault(name, set()).add(service["name"])
    for dependency in set().union(*graph.values()) if graph else []:
        graph.setdefault(dependency, set())
    result = []
    while graph:
        ready = sorted(name for name, dependencies in graph.items() if not dependencies)
        if not ready:
            raise ValueError("cyclic " + kind + " shutdown dependencies")
        result.extend(ready)
        graph = {name: dependencies - set(ready) for name, dependencies in graph.items() if name not in ready}
    return " ".join(result)


def build(spec):
    metadata = json.loads(Path(spec["containers"]).read_text())
    if metadata.get("schema") != 1:
        raise ValueError("unsupported VS product metadata schema")
    configuration = metadata["configuration"]
    if (configuration["platform"], configuration["architecture"], configuration["distribution"],
            configuration["fips"], configuration["dash"]) != ("vs", "amd64", "trixie", True, True):
        raise ValueError("host configuration requires the declared VS AMD64 Trixie FIPS DASH profile")
    selected = {repo: entry for repo, entry in metadata["containers"].items() if entry["installed"]}
    context = dict(spec["context"])
    if spec.get("build_identity"):
        identity = json.loads(Path(spec["build_identity"]).read_text())
        required_identity = ("commit_id", "branch", "build_date", "build_number", "built_by")
        for key in required_identity:
            if not isinstance(identity.get(key), str) or not identity[key]:
                raise ValueError("missing declared build identity field: " + key)
        identity.update(context)
        context = identity
    for key in ("commit_id", "branch", "build_date", "build_number", "built_by", "sonic_os_version", "username"):
        if key not in context:
            raise ValueError("missing explicit host build value: " + key)
    version = Path(spec["image_version"]).read_text().strip()
    if not version or "\n" in version:
        raise ValueError("image version must be one nonempty line")
    image_repositories = spec["image_repositories"]
    raw_manifests = {repo: render_container_metadata(metadata, repo, version, image_repositories[repo] != repo)[0] for repo in selected}
    manifests = {repo: _manifest_defaults(manifest) for repo, manifest in raw_manifests.items()}
    kernel = json.loads(Path(spec["kernel_metadata"]).read_text())
    if kernel.get("schema") != 1 or kernel.get("architecture") != "amd64":
        raise ValueError("host kernel metadata must describe the selected AMD64 producer")
    context.update(build_version=version, kernel_version=kernel["kernel_release"], asic_type="vs", asic_subtype="vs")
    services = []
    templates = {name: Path(path).read_text() for name, path in spec["templates"].items()}
    environment = Environment(loader=DictLoader(templates), extensions=["jinja2.ext.do"], keep_trailing_newline=True)
    environment.filters["json"] = json.dumps
    out = Generated()

    def render(template, values=None):
        return environment.get_template(template).render(context | (values or {}))

    def rendered(path, template, values=None, mode=0o644):
        value = render(template, values)
        if path.endswith(".json"):
            json.loads(value)
        out.file(path, value, mode)
        return value

    features = {m["service"]["name"] for m in manifests.values()}
    for service, flag in {"teamd": "include_teamd", "radv": "include_router_advertiser", "lldp": "include_lldp",
                          "snmp": "include_snmp", "sflow": "include_sflow", "gnmi": "include_system_gnmi",
                          "otel": "include_system_otel", "eventd": "include_system_eventd", "bmp": "include_system_bmp",
                          "macsec": "include_macsec", "mux": "include_mux", "nat": "include_nat",
                          "p4rt": "include_p4rt", "mgmt-framework": "include_mgmt_framework", "dash-ha": "include_dash_ha"}.items():
        context[flag] = "y" if service in features else "n"
    for repository, manifest in manifests.items():
        name = manifest["service"]["name"]
        if selected[repository]["local_package"]:
            continue
        service_context = dict(docker_container_name=name, docker_image_name=image_repositories[repository],
                               docker_image_run_opt=selected[repository]["run_options"], mount_default_tmpfs="y", multi_instance="false")
        rendered("usr/bin/" + name + ".sh", "files/build_templates/docker_image_ctl.j2", service_context, 0o755)
        for directory, suffix, multi in [("", ".service", "false"), ("per_namespace/", ".service", "false"),
                                          ("per_namespace/", "@.service", "true"), ("share_image/", "-chassis.service", "false")]:
            template = "files/build_templates/" + directory + name + ".service.j2"
            if template in templates:
                rendered("usr/lib/systemd/system/" + name + suffix, template, service_context | {"multi_instance": multi})
                if suffix != ".service" or "files/build_templates/per_namespace/" + name + ".service.j2" not in templates:
                    services.append(name + suffix)
    context["installer_services"] = " ".join(dict.fromkeys(services))
    init_config = json.loads(render("files/build_templates/init_cfg.json.j2"))
    local_receipts = {}
    multi_features = [name for name, value in init_config["FEATURE"].items() if str(value.get("has_per_asic_scope")).lower() == "true"]
    package_database = {}
    dependencies = {}
    for repository, manifest in manifests.items():
        entry = selected[repository]
        name, package = manifest["service"]["name"], manifest["package"]
        package_name = package["name"]
        if not entry["local_package"]:
            package_database[package_name] = {"repository": image_repositories[repository],
                "description": "SONiC " + package_name + " package", "default-reference": package["version"],
                "installed-version": package["version"], "built-in": True, "installed": True}
            continue
        if repository not in spec["local_images"]:
            raise ValueError("local package requires its selected OCI layout: " + repository)
        layout = validate_layout(Path(spec["local_images"][repository]), "linux/amd64")
        image_manifest = json.loads(layout.config["config"]["Labels"]["com.azure.sonic.manifest"])
        if image_manifest != raw_manifests[repository]:
            raise ValueError("local OCI manifest differs from product selection: " + repository)
        image_id = layout.manifest["config"]["digest"]
        image_repository = image_repositories[repository]
        image_reference = image_repository + ":latest"
        package_database[package_name] = {"repository": image_repository, "description": package["description"],
            "default-reference": None, "installed-version": package["version"], "built-in": False, "installed": True,
            "image-id": image_id, "docker-image-reference": image_reference}
        service_context = {"manifest": manifest, "multi_instance_services": multi_features}
        source = "sonic-utilities-data/templates/"
        for multi, suffix in [(False, ".service")] + ([(True, "@.service")] if manifest["service"]["asic-service"] else []):
            rendered("usr/lib/systemd/system/" + name + suffix, source + "sonic.service.j2",
                     service_context | {"source": "/usr/share/sonic/templates/sonic.service.j2", "multi_instance": multi})
            services.append(name + suffix)
        rendered("usr/local/bin/" + name + ".sh", source + "service_mgmt.sh.j2",
                 service_context | {"source": "/usr/share/sonic/templates/service_mgmt.sh.j2"}, 0o755)
        rendered("usr/bin/" + name + ".sh", "files/build_templates/docker_image_ctl.j2",
                 {"docker_container_name": name, "docker_image_id": image_id, "docker_image_name": image_repository,
                  "docker_image_reference": image_reference, "docker_image_run_opt": entry["run_options"],
                  "stop_time": manifest["service"]["stop-time"]}, 0o755)
        out.file("etc/sonic/" + name + "_reconcile", " ".join(p["name"] for p in manifest["processes"] if p["reconciles"]))
        if package["debug-dump"]:
            rendered("usr/local/bin/debug-dump/" + name, source + "dump.sh.j2", service_context, 0o755)
        for dependent in manifest["service"]["dependent-of"]:
            for enabled, suffix in [(manifest["service"]["host-service"], "_dependent"),
                                    (manifest["service"]["asic-service"], "_multi_inst_dependent")]:
                if enabled:
                    dependencies.setdefault(dependent + suffix, set()).add(name)
        feature = {"state": "enabled" if entry["default_feature_state_enabled"] == "y" else "disabled",
                   "auto_restart": "enabled", "high_mem_alert": "disabled", "set_owner": entry["default_feature_owner"] or "local"}
        feature.update(init_config.setdefault("FEATURE", {}).get(name, {}))
        feature.update({"has_per_asic_scope": str(manifest["service"]["asic-service"]),
                        "has_global_scope": str(manifest["service"]["host-service"]),
                        "delayed": str(manifest["service"]["delayed"]), "check_up_status": str(manifest["service"]["check_up_status"]),
                        "support_syslog_rate_limit": str(manifest["service"]["syslog"]["support-rate-limit"])})
        init_config["FEATURE"][name] = feature
        auto_state = init_config.get("AUTO_TECHSUPPORT", {}).get("GLOBAL", {}).get("state")
        if auto_state:
            init_config.setdefault("AUTO_TECHSUPPORT_FEATURE", {})[name] = {"state": auto_state, "rate_limit_interval": "600", "available_mem_threshold": "10.0"}
        if manifest["service"]["syslog"]["support-rate-limit"]:
            init_config.setdefault("SYSLOG_CONFIG_FEATURE", {})[name] = {"rate_limit_interval": "300", "rate_limit_burst": "20000"}
        wanted = {path.lstrip("/") for command in ("show", "config", "clear") for path in manifest["cli"][command]}
        plugins = read_files(layout.layers, wanted)
        for command in ("show", "config", "clear"):
            for index, path in enumerate(manifest["cli"][command]):
                content, member = plugins[path.lstrip("/")]
                module = package_name + ("_" + str(index) if index else "") + ".py"
                out.file(context["python_site_packages"].strip("/") + "/" + command + "/plugins/" + module,
                         content, member.mode, member.uid, member.gid)
        local_receipts[repository] = {"image_id": image_id, "manifest": entry["manifest"]}
    out.file("etc/sonic/init_cfg.json", json.dumps(init_config, indent=4) + "\n")
    out.file("var/lib/sonic-package-manager/packages.json", json.dumps(package_database, indent=4) + "\n")
    for name, values in dependencies.items():
        out.file("etc/sonic/" + name, "\n".join(sorted(values)))
    for kind in ("fast", "warm"):
        out.file("etc/sonic/" + kind + "-reboot_order", _shutdown_order(manifests.values(), kind))
    critical = {"database", "swss", "syncd", "teamd", "bgp"}
    for name, feature in init_config["FEATURE"].items():
        state = feature.get("state")
        if (name in critical and state in ("disabled", "always_disabled")) or (name not in critical and state not in ("enabled", "always_enabled")):
            out.symlink("etc/systemd/system/" + name + ".service", "/dev/null")
    out.symlink("etc/systemd/system/nslcd.service", "/dev/null")
    out.symlink("etc/systemd/system-generators/systemd-ssh-generator", "/dev/null")
    for name in ("bridge-midplane.netdev", "dummy-midplane.netdev"):
        out.symlink("etc/systemd/network/" + name, "/dev/null")
    for name in ["dash-ha.service"] + ["dash-ha@dpu" + str(i) + ".service" for i in range(8)]:
        out.symlink("etc/systemd/system/" + name, "/dev/null")
    for path, template in {"usr/lib/systemd/system/config-setup.service": "files/build_templates/config-setup.service.j2",
                           "usr/lib/systemd/system/config-chassisdb.service": "files/build_templates/config-chassisdb.service.j2",
                           "etc/sonic/constants.yml": "files/build_templates/constants.yml.j2",
                           "etc/rsyslog.d/00-sonic.conf": "files/image_config/rsyslog/rsyslog.d/00-sonic.conf.j2",
                           "etc/dhcp/dhclient.conf": "files/dhcp/dhclient.conf.j2",
                           "etc/initramfs-tools/scripts/init-bottom/union-mount": "files/initramfs-tools/union-mount.j2",
                           "etc/initramfs-tools/scripts/init-premount/arista-convertfs": "files/initramfs-tools/arista-convertfs.j2"}.items():
        rendered(path, template, mode=0o755 if path.startswith("etc/initramfs-tools/scripts/") else 0o644)
    with ExitStack() as stack:
        tree = Tree()
        for path in spec["base_layers"]:
            archive = stack.enter_context(tarfile.open(path, "r:*"))
            apply_layer(tree, archive, path)
        context["debian_version"] = _root_file(tree, "etc/debian_version").decode().strip()
        shadow = _root_file(tree, "etc/shadow").decode().splitlines()
        users = [line.split(":") for line in shadow if line.split(":", 1)[0] == context["username"]]
        if len(users) != 1:
            raise ValueError("selected admin account must exist exactly once in installed shadow")
        context.update(username=context["username"], password=users[0][1], password_expire="true" if context.get("CHANGE_DEFAULT_PASSWORD") == "y" else "false")
        rendered("etc/sonic/default_users.json", "files/build_templates/default_users.json.j2", mode=0o600)
        # Debian's shadow group is supplied by the account producer.
        groups = _root_file(tree, "etc/group").decode().splitlines()
        shadow_groups = [line.split(":")[2] for line in groups if line.startswith("shadow:")]
        if len(shadow_groups) != 1:
            raise ValueError("installed shadow group is missing or duplicated")
        kind, content, mode, uid, _ = out.entries["etc/sonic/default_users.json"]
        out.entries["etc/sonic/default_users.json"] = (kind, content, mode, uid, int(shadow_groups[0]))
        rendered("etc/sonic/sonic_version.yml", "files/build_templates/sonic_version.yml.j2")
        rendered("etc/sonic/bmc_config.json", "files/build_templates/bmc_config.json.j2")
        modules = _root_file(tree, "etc/initramfs-tools/modules").decode()
        out.file("etc/initramfs-tools/modules", modules + Path(spec["modules"]).read_text())
        edits = {"etc/default/kexec": [(r"LOAD_KEXEC=true", "LOAD_KEXEC=false")],
                 "etc/systemd/networkd.conf": [(r"#ManageForeignRoutingPolicyRules=yes", "ManageForeignRoutingPolicyRules=no"),
                                               (r"#ManageForeignNextHops=yes", "ManageForeignNextHops=no")],
                 "usr/lib/systemd/system/rc-local.service.d/debian.conf": [(r"(?m)^After=network-online.target\n", "")],
                 "etc/init.d/nslcd": [(r"(?m)^(# Required-Start:.*)$", r"\1 networking.service")]}
        for path, changes in edits.items():
            content = _root_file(tree, path, required=path != "usr/lib/systemd/system/rc-local.service.d/debian.conf")
            if content is not None:
                text = content.decode()
                for pattern, replacement in changes:
                    text = re.sub(pattern, replacement, text)
                out.file(path, text, 0o755 if path.startswith("etc/init.d/") else 0o644)
        ssh = _root_file(tree, "etc/ssh/sshd_config").decode()
        for key, value in {"UseDNS": "no", "ClientAliveInterval": "300", "ClientAliveCountMax": "0", "MaxAuthTries": "3",
                           "Banner": "/etc/issue", "LogLevel": "VERBOSE", "PermitEmptyPasswords": "no",
                           "AllowAgentForwarding": "no", "PerSourcePenalties": "authfail:0"}.items():
            ssh = re.sub(r"(?m)^" + key + r"\s+.*\n?", "", ssh)
            ssh += "\n" + key + " " + value + "\n"
        ssh = ssh.replace("#ListenAddress 0.0.0.0", "ListenAddress 0.0.0.0").replace("#ListenAddress ::", "ListenAddress ::")
        out.file("etc/ssh/sshd_config", ssh)
        units = {name: Path(path).read_text() for name, path in spec["units"].items()}
        for path, (kind, content, _, _, _) in out.entries.items():
            if kind == "file" and path.startswith("usr/lib/systemd/system/") and path.endswith(".service"):
                units[path.rsplit("/", 1)[-1]] = content.decode()
        enable = ["sed-pw-tpm-recovery.service", "chrony.service", "set-epoch.timer", "rasdaemon.timer", "fstrim.timer",
                  "config-setup.service", "config-chassisdb.service", "systemd-networkd-wait-online.service", "sonic.target"]
        for unit in enable:
            text = units.get(unit)
            if text is None:
                text = _root_file(tree, "usr/lib/systemd/system/" + unit).decode()
            if unit == "chrony.service":
                text += "\n" + Path(spec["chrony_override"]).read_text()
            _enable(out, unit, text, "/usr/lib/systemd/system/" + unit)
    out.file("etc/sonic/generated_services.conf", "\n".join(dict.fromkeys(spec["generated_services"] + services)) + "\n")
    out.file("etc/fips/fips_enable", "0\n")
    out.file("etc/modules-load.d/modules.conf", "psample\nact_sample\n")
    out.file("etc/initramfs-tools/conf.d/driver-policy", "MODULES=most\n")
    out.file("etc/tmpfiles.d/tmpfs-monit.conf", "d\t/dev/shm/monit/ 0755 root root\n")
    out.file("etc/tmpfiles.d/tmpfs-logrotate.conf", "d\t/dev/shm/logrotate/ 0755 root root\n")
    out.file("etc/tmpfiles.d/tmp.conf", "q\t/tmp 1777 root root -\nq\t/var/tmp 1777 root root -\n")
    out.write(spec["out"])
    Path(spec["audit"]).write_text(json.dumps({"configuration": configuration, "image_version": version,
        "kernel_release": kernel["kernel_release"], "debug": spec["debug"], "local_packages": local_receipts,
        "generated_paths": sorted(out.entries)}, indent=2) + "\n")


if __name__ == "__main__":
    import sys
    build(json.loads(Path(sys.argv[1]).read_text()))
