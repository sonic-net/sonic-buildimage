# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Dependency-aware kernel module unload/reload.

Backs the `nh_module` CLI. Parses /proc/modules to find a module's
dependents automatically and unloads/reloads them in the correct order,
and (for PDDF-managed modules) re-creates PDDF sysfs device nodes afterwards.
"""

import re

from nexthop.pddf_loader import load_pddf_device_config
from nexthop_utils.platform_utils import run_cmd

PROC_MODULES_PATH = "/proc/modules"
PDDF_UTIL_PATH = "/usr/local/bin/pddf_util.py"
MODULE_NAME_RE = re.compile(r"^[A-Za-z0-9_-]+$")


class ModuleReloadError(Exception):
    """Raised when a module unload/reload/device-recreation step fails."""


def _normalize(module_name):
    """Kernel module names in /proc/modules always use underscores."""
    return module_name.replace("-", "_")


def _validate_module_name(module_name):
    """Reject anything that isn't a plain module name.

    module_name is interpolated into a command string that run_cmd() later
    splits with shlex; this keeps malformed input (stray spaces, shell
    metacharacters) from being misparsed into unintended arguments.
    """
    if not MODULE_NAME_RE.match(module_name):
        raise ModuleReloadError(
            f"Invalid module name '{module_name}': must match {MODULE_NAME_RE.pattern}"
        )


def _run_cmd_or_raise(cmd, error_prefix):
    try:
        run_cmd(cmd)
    except Exception as e:
        raise ModuleReloadError(f"{error_prefix}: {e}") from e


def get_loaded_modules():
    """Return {module_name: [direct dependent module names]} from /proc/modules.

    The 4th whitespace-separated field of /proc/modules is the same
    comma-separated "Used by" list that `lsmod` displays: the modules
    currently depending on (using) this module, not the modules it depends on.
    """
    modules = {}
    with open(PROC_MODULES_PATH, "r") as f:
        lines = f.readlines()

    for line in lines:
        fields = line.split()
        if not fields:
            continue
        name = fields[0]
        dependents_field = fields[3] if len(fields) > 3 else "-"
        if dependents_field == "-":
            dependents = []
        else:
            dependents = [d for d in dependents_field.rstrip(",").split(",") if d]
        modules[name] = dependents

    return modules


def is_module_loaded(module_name, loaded_modules=None):
    loaded_modules = loaded_modules if loaded_modules is not None else get_loaded_modules()
    return _normalize(module_name) in loaded_modules


def get_dependents(module_name, loaded_modules=None):
    """Direct dependents (modules currently using module_name)."""
    loaded_modules = loaded_modules if loaded_modules is not None else get_loaded_modules()
    name = _normalize(module_name)
    if name not in loaded_modules:
        raise ModuleReloadError(f"Module '{module_name}' is not loaded")
    return loaded_modules[name]


def _unload_order_for(module_names, loaded_modules):
    """Combined unload order for every loaded module in module_names.

    Each module is unloaded only after all of its (transitive) dependents,
    including dependents shared between more than one module in module_names.
    """
    order = []
    visited = set()

    def visit(mod):
        if mod in visited:
            return
        visited.add(mod)
        for dependent in loaded_modules.get(mod, []):
            visit(dependent)
        order.append(mod)

    for module_name in module_names:
        name = _normalize(module_name)
        if name in loaded_modules:
            visit(name)

    return order


def get_unload_order(module_name, loaded_modules=None):
    """Modules in the order they must be unloaded to remove `module_name`.

    Returns a list ending in `module_name` itself, with every (transitive)
    dependent appearing before it, deepest dependent first.
    """
    loaded_modules = loaded_modules if loaded_modules is not None else get_loaded_modules()
    name = _normalize(module_name)
    if name not in loaded_modules:
        raise ModuleReloadError(f"Module '{module_name}' is not loaded")

    return _unload_order_for([name], loaded_modules)


def get_declared_modules():
    """Return (pddf_kos, custom_kos) declared for the current platform, or ([], []) if unavailable."""
    try:
        config = load_pddf_device_config()
    except (FileNotFoundError, OSError):
        return [], []
    platform = config.get("PLATFORM", {})
    return platform.get("pddf_kos", []), platform.get("custom_kos", [])


def is_pddf_module(module_name):
    """Whether module_name is declared in the current platform's pddf-device.json."""
    pddf_kos, custom_kos = get_declared_modules()
    declared = {_normalize(m) for m in (set(pddf_kos) | set(custom_kos))}
    return _normalize(module_name) in declared


def recreate_pddf_devices():
    """Re-create PDDF sysfs device nodes without touching loaded kernel modules.

    PDDF sysfs nodes are created from the currently loaded PDDF/custom kernel
    modules; they are not automatically re-created when a module backing them
    is reloaded. This shells out to `pddf_util.py recreate-devices`, which
    deletes and re-creates them via the same pddfparse code path used at boot.
    """
    _run_cmd_or_raise(f"{PDDF_UTIL_PATH} recreate-devices", "Failed to recreate PDDF devices")


def reload_all_modules():
    """Reload every PDDF/custom kernel module and recreate PDDF devices.

    Unloads every declared PDDF/custom module in dependency order first:
    `pddf_util.py clean`'s own unload step removes them in a fixed declared
    order and fails if a module still has a loaded dependent (e.g.
    nh_pmbus_core while nh_adm1266 is still loaded on it). With everything
    already unloaded, that step is a no-op, and `pddf_util.py install`
    reloads everything the same way it does at boot.
    """
    pddf_kos, custom_kos = get_declared_modules()
    loaded_modules = get_loaded_modules()
    for mod in _unload_order_for(pddf_kos + custom_kos, loaded_modules):
        _run_cmd_or_raise(f"modprobe -r {mod}", f"Failed to unload {mod}")

    _run_cmd_or_raise(f"{PDDF_UTIL_PATH} clean", "pddf_util.py clean failed")
    _run_cmd_or_raise(f"{PDDF_UTIL_PATH} install", "pddf_util.py install failed")


def unload_module(module_name, recursive=True):
    """Unload module_name, unloading its dependents first if recursive."""
    _validate_module_name(module_name)
    name = _normalize(module_name)
    loaded_modules = get_loaded_modules()
    if name not in loaded_modules:
        return

    if recursive:
        unload_order = get_unload_order(name, loaded_modules)
    else:
        dependents = loaded_modules[name]
        if dependents:
            raise ModuleReloadError(
                f"Module '{module_name}' is in use by {', '.join(dependents)}; "
                "unload those first or pass recursive=True"
            )
        unload_order = [name]

    for mod in unload_order:
        _run_cmd_or_raise(f"modprobe -r {mod}", f"Failed to unload {mod}")


def reload_module(module_name, recreate_devices=None):
    """Unload module_name (and dependents) and load them all back.

    Runs `depmod -a` between unload and reload so a swapped-in .ko file (or
    a rebuilt module) is picked up. If recreate_devices is None, PDDF device
    nodes are recreated automatically when module_name is a PDDF-declared
    module; pass True/False to override.

    module_name does not need to already be loaded (e.g. it may have already
    been unloaded to swap its .ko file); in that case this just modprobes it.
    """
    _validate_module_name(module_name)
    name = _normalize(module_name)
    loaded_modules = get_loaded_modules()

    if name in loaded_modules:
        unload_order = get_unload_order(name, loaded_modules)
        reload_order = list(reversed(unload_order))
    else:
        unload_order = []
        reload_order = [name]

    for mod in unload_order:
        _run_cmd_or_raise(f"modprobe -r {mod}", f"Failed to unload {mod}")

    _run_cmd_or_raise("depmod -a", "depmod failed")

    for mod in reload_order:
        _run_cmd_or_raise(f"modprobe {mod}", f"Failed to load {mod}")

    if recreate_devices is None:
        recreate_devices = is_pddf_module(name)

    if recreate_devices:
        recreate_pddf_devices()
