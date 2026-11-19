#!/usr/bin/env python3
# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

import argparse
import json
import logging
import os
import re
import sys
import tempfile

from natsort import natsorted
import yaml


logging.basicConfig()
LOGGER = logging.getLogger("dpb_config_db_migration")
LOGGER.setLevel(logging.INFO)

BREAKOUT_CFG = "BREAKOUT_CFG"
ENABLE_DPB_FIELD = "enable_dpb"
INTERFACES = "interfaces"
PORT = "PORT"

BREAKOUT_MODE_PATTERN = re.compile(
    r"(?P<num_ports>\d{1,6})x(?P<speed>\d{1,6}G)"
    r"(?:\((?P<assigned_lanes>\d{1,6})\))?"
)


def _read_json(path):
    """
    Read a JSON file.

    Args:
        path: Path to the JSON file.

    Returns:
        Decoded JSON object.
    """
    with open(path) as fp:
        return json.load(fp)


def _write_atomic(path, write_data):
    """
    Write a file atomically while preserving existing target metadata.

    Args:
        path: Destination path, which may be a symlink.
        write_data: Callable that writes content to an open file object.
    """
    target_path = os.path.realpath(path)
    directory = os.path.dirname(target_path)
    target_stat = os.stat(target_path)
    fd, tmp_path = tempfile.mkstemp(prefix=".tmp_dpb_migration_", dir=directory)
    try:
        with os.fdopen(fd, "w") as fp:
            write_data(fp)
        if os.geteuid() == 0:
            os.chown(tmp_path, target_stat.st_uid, target_stat.st_gid)
        os.chmod(tmp_path, target_stat.st_mode & 0o7777)
        os.replace(tmp_path, target_path)
    except Exception:
        try:
            os.unlink(tmp_path)
        except FileNotFoundError:
            pass
        raise


def _write_json(path, data):
    """
    Write a JSON file atomically.

    Args:
        path: Destination path.
        data: JSON-serializable object to write.
    """
    def _write(fp):
        json.dump(data, fp, indent=4)
        fp.write("\n")

    _write_atomic(path, _write)


def _read_yaml(path):
    """
    Read a YAML file.

    Args:
        path: Path to the YAML file.

    Returns:
        Decoded YAML value.
    """
    with open(path) as fp:
        return yaml.safe_load(fp)


def _write_yaml(path, data):
    """
    Write a YAML file atomically.

    Args:
        path: Destination path.
        data: YAML-serializable object to write.
    """
    def _write(fp):
        yaml.safe_dump(data, fp, default_flow_style=False, sort_keys=False)

    _write_atomic(path, _write)


def ensure_enable_dpb(config_path, reference_path=None):
    """
    Add enable_dpb only when the target image packaged config enables it.

    Args:
        config_path: Restored platform_cisco_cfg.yaml path.
        reference_path: Target image packaged platform_cisco_cfg.yaml path
            captured before the restored file overwrote it.

    Returns:
        bool: True if config_path was updated.
    """
    config = _read_yaml(config_path)
    if not isinstance(config, dict) or not config:
        raise ValueError(
            "{} does not contain a non-empty YAML mapping".format(config_path))

    if ENABLE_DPB_FIELD in config:
        LOGGER.info("%s already has %s, preserving value %s",
                    config_path, ENABLE_DPB_FIELD, config[ENABLE_DPB_FIELD])
        return False

    if not reference_path or not os.path.isfile(reference_path):
        LOGGER.info("No reference platform_cisco_cfg.yaml; skip adding %s", ENABLE_DPB_FIELD)
        return False

    reference = _read_yaml(reference_path)
    if not isinstance(reference, dict) or reference.get(ENABLE_DPB_FIELD) is not True:
        LOGGER.info("Reference config does not enable DPB; skip adding %s", ENABLE_DPB_FIELD)
        return False

    config[ENABLE_DPB_FIELD] = True
    _write_yaml(config_path, config)
    LOGGER.info("Added %s: true to %s", ENABLE_DPB_FIELD, config_path)
    return True


def is_dpb_enabled(config_path):
    """
    Return whether the active platform configuration explicitly enables DPB.

    Args:
        config_path: Restored platform_cisco_cfg.yaml path.

    Returns:
        bool: True only when enable_dpb is the YAML Boolean true.
    """
    config = _read_yaml(config_path)
    if not isinstance(config, dict) or not config:
        raise ValueError(
            "{} does not contain a non-empty YAML mapping".format(config_path))

    return config.get(ENABLE_DPB_FIELD) is True


def _normalize_lanes(lanes):
    """
    Validate and normalize a comma-separated lane string.

    Args:
        lanes: Lane string from PORT/platform.json, or None when absent.

    Returns:
        Tuple of lane identifiers as strings.

    Raises:
        ValueError: The value is not a comma-separated numeric lane string.
    """
    if lanes is None:
        return ()
    if not isinstance(lanes, str):
        raise ValueError("Lane field must be a string")

    parts = lanes.split(",")
    if any(not part or not part.isdigit() for part in parts):
        raise ValueError("Invalid lane string: {!r}".format(lanes))
    return tuple(parts)


def _lane_sort_key(lane):
    """
    Build a stable sort key for lane identifiers.

    Args:
        lane: Lane identifier.

    Returns:
        Tuple suitable for sorting numeric lanes numerically and other lanes
        lexically.
    """
    lane_str = str(lane).strip()
    try:
        return (0, int(lane_str))
    except ValueError:
        return (1, lane_str)


def _canonical_lane_group(lanes):
    """
    Normalize a group of lanes for shape comparison.

    Args:
        lanes: Iterable of lane identifiers.

    Returns:
        Tuple of lane identifiers sorted into a stable canonical order.
    """
    return tuple(sorted((str(lane).strip() for lane in lanes if str(lane).strip()),
                        key=_lane_sort_key))


def _validate_unique_lanes(lanes, owner):
    """
    Reject duplicate lane identifiers within one lane owner.

    Args:
        lanes: Normalized lane tuple.
        owner: Description used in an error message.

    Raises:
        ValueError: One lane appears more than once.
    """
    if len(lanes) != len(set(lanes)):
        raise ValueError("{} has duplicate lanes {}".format(owner, lanes))


def _normalize_speed(speed):
    """
    Normalize a SONiC speed value to Mbps.

    Args:
        speed: Speed value such as 100000 or 100G.

    Returns:
        Integer speed in Mbps, or None for a missing speed.
    """
    if speed is None:
        return None
    speed_str = str(speed).strip()
    if speed_str.endswith("G"):
        return int(speed_str[:-1]) * 1000
    return int(speed_str)


def _parse_breakout_mode(mode, parent_lane_count):
    """
    Parse a SONiC breakout mode string.

    Args:
        mode: Breakout mode string, for example 4x25G or
            1x25G(1)+1x50G(1).
        parent_lane_count: Number of lanes owned by the parent interface.

    Returns:
        List of tuples containing number of ports, speed in Mbps, and assigned lanes.
    """
    if not isinstance(mode, str):
        raise ValueError("Breakout mode is not a string")

    entries = []
    for component in mode.split("+"):
        match = BREAKOUT_MODE_PATTERN.fullmatch(component)
        if not match:
            raise ValueError("Unsupported breakout mode format: {}".format(mode))

        num_ports = int(match.group("num_ports"))
        speed = _normalize_speed(match.group("speed"))
        assigned_lanes = (int(match.group("assigned_lanes"))
                          if match.group("assigned_lanes") else parent_lane_count)
        if num_ports <= 0 or assigned_lanes <= 0:
            raise ValueError("Invalid breakout mode lane count: {}".format(mode))
        if speed <= 0:
            raise ValueError("Invalid breakout mode speed: {}".format(mode))
        if assigned_lanes % num_ports != 0:
            raise ValueError("Breakout mode lanes are not divisible by ports: {}".format(mode))

        entries.append((num_ports, speed, assigned_lanes))

    return entries


def _expected_port_shapes(parent_props, mode, aliases):
    """
    Build lane and speed shapes expected for a breakout mode.

    Args:
        parent_props: Parent interface properties from platform.json.
        mode: Candidate breakout mode.
        aliases: Child interface aliases for the candidate mode.

    Returns:
        Sorted list of tuples containing child lane tuple and speed in Mbps.
    """
    parent_lanes = _normalize_lanes(parent_props.get("lanes"))
    if not parent_lanes:
        raise ValueError("Parent interface has no lanes")
    _validate_unique_lanes(parent_lanes, "Parent interface")

    mode_entries = _parse_breakout_mode(mode, len(parent_lanes))
    expected_alias_count = sum(num_ports for num_ports, _, _ in mode_entries)
    if not isinstance(aliases, list):
        raise ValueError("Breakout mode aliases are not a list")
    if len(aliases) != expected_alias_count:
        raise ValueError("Breakout mode has {} aliases, expected {}".format(
            len(aliases), expected_alias_count))
    if any(
            not isinstance(alias, str)
            or not alias
            or not alias.isprintable()
            or any(character.isspace() for character in alias)
            for alias in aliases):
        raise ValueError("Breakout mode has an invalid alias")
    if len(aliases) != len(set(aliases)):
        raise ValueError("Breakout mode has duplicate aliases")

    shapes = []
    lane_offset = 0
    for num_ports, speed, assigned_lanes in mode_entries:
        lanes_per_port = assigned_lanes // num_ports
        for _ in range(num_ports):
            child_lanes = parent_lanes[lane_offset: lane_offset + lanes_per_port]
            if len(child_lanes) != lanes_per_port:
                raise ValueError("Breakout mode consumes more lanes than parent owns: {}".format(mode))
            shapes.append((_canonical_lane_group(child_lanes), speed))
            lane_offset += lanes_per_port

    return sorted(shapes)


def _actual_port_shapes(port_table, parent_props):
    """
    Build lane and speed shapes from the preserved PORT table.

    Args:
        port_table: PORT table from preserved config_db.json.
        parent_props: Parent interface properties from platform.json.

    Returns:
        Tuple containing the sorted port shapes and names of matched PORT entries.

    Raises:
        ValueError: Parent or PORT lanes are malformed or duplicated, one lane
            is owned by multiple PORT entries, a PORT overlaps the parent
            without being contained by it, or a represented PORT speed is
            invalid.
    """
    normalized_parent_lanes = _normalize_lanes(parent_props.get("lanes"))
    if not normalized_parent_lanes:
        return [], set()
    parent_lanes = set(normalized_parent_lanes)

    shapes = []
    matched_ports = set()
    lane_owners = {}
    parent_lanes_validated = False
    for port_name, port_attrs in port_table.items():
        if not isinstance(port_attrs, dict):
            continue
        port_lanes = _normalize_lanes(port_attrs.get("lanes"))
        if not port_lanes or not (set(port_lanes) & parent_lanes):
            continue

        _validate_unique_lanes(port_lanes, port_name)
        if not parent_lanes_validated:
            _validate_unique_lanes(normalized_parent_lanes, "Parent interface")
            parent_lanes_validated = True

        port_lane_set = set(port_lanes)
        if not port_lane_set.issubset(parent_lanes):
            raise ValueError("{} lanes {} are not contained by parent".format(port_name, port_lanes))

        for lane in port_lanes:
            if lane in lane_owners:
                raise ValueError("lane {} is owned by both {} and {}".format(
                    lane, lane_owners[lane], port_name))
            lane_owners[lane] = port_name

        try:
            speed = _normalize_speed(port_attrs.get("speed"))
        except (TypeError, ValueError):
            raise ValueError("{} has invalid speed {}".format(
                port_name, port_attrs.get("speed")))
        if speed is None or speed <= 0:
            raise ValueError("{} has invalid speed {}".format(
                port_name, port_attrs.get("speed")))
        shapes.append((_canonical_lane_group(port_lanes), speed))
        matched_ports.add(port_name)

    return sorted(shapes), matched_ports


def _get_table(config_db, table_name):
    """
    Fetch a CONFIG_DB table if present.

    Args:
        config_db: config_db.json dictionary.
        table_name: Table name to fetch.

    Returns:
        Table dictionary, or an empty dict if missing or malformed.
    """
    table = config_db.get(table_name, {})
    if not isinstance(table, dict):
        return {}
    return table


def derive_breakout_cfg(config_db, platform_json):
    """
    Derive BREAKOUT_CFG from preserved PORT layout and platform capabilities.

    Args:
        config_db: Deserialized config_db.json dictionary.
        platform_json: platform.json dictionary from the target image.

    Returns:
        dict: BREAKOUT_CFG table keyed by parent interface.
    """
    port_table = _get_table(config_db, PORT)
    platform_interfaces = platform_json.get(INTERFACES)
    if not port_table:
        return {}
    if not isinstance(platform_interfaces, dict) or not platform_interfaces:
        LOGGER.warning("platform.json has no usable %s section; skip %s derivation",
                       INTERFACES, BREAKOUT_CFG)
        return {}

    breakout_cfg = {}
    represented_ports = set()
    represented_parent_lane_owners = {}
    for parent_name in natsorted(platform_interfaces):
        parent_props = platform_interfaces[parent_name]
        if not isinstance(parent_props, dict):
            continue

        try:
            actual_shapes, parent_ports = _actual_port_shapes(port_table, parent_props)
        except ValueError as err:
            LOGGER.warning("Cannot derive %s for %s: %s", BREAKOUT_CFG, parent_name, err)
            return {}

        if not actual_shapes:
            continue

        parent_lanes = set(_normalize_lanes(parent_props.get("lanes")))
        overlapping_lanes = set(represented_parent_lane_owners) & parent_lanes
        if overlapping_lanes:
            overlaps_by_parent = {}
            for lane in overlapping_lanes:
                previous_parent = represented_parent_lane_owners[lane]
                overlaps_by_parent.setdefault(previous_parent, set()).add(lane)
            for previous_parent in natsorted(overlaps_by_parent):
                LOGGER.warning(
                    "Cannot derive %s: represented parents %s and %s have "
                    "overlapping capability lanes %s",
                    BREAKOUT_CFG,
                    previous_parent,
                    parent_name,
                    natsorted(overlaps_by_parent[previous_parent]),
                )
            return {}
        for lane in parent_lanes:
            represented_parent_lane_owners[lane] = parent_name
        represented_ports.update(parent_ports)

        breakout_modes = parent_props.get("breakout_modes", {})
        if not isinstance(breakout_modes, dict) or not breakout_modes:
            LOGGER.warning("Cannot derive %s: %s has no usable breakout modes",
                           BREAKOUT_CFG, parent_name)
            return {}

        matching_modes = []
        for mode, aliases in breakout_modes.items():
            try:
                if _expected_port_shapes(parent_props, mode, aliases) == actual_shapes:
                    matching_modes.append(mode)
            except (TypeError, ValueError) as err:
                LOGGER.warning("Cannot derive %s for %s: invalid mode %s: %s",
                               BREAKOUT_CFG, parent_name, mode, err)
                return {}

        if len(matching_modes) == 1:
            breakout_cfg[parent_name] = {"brkout_mode": matching_modes[0]}
        elif len(matching_modes) > 1:
            LOGGER.warning("Cannot derive %s for %s: multiple matching breakout modes %s",
                           BREAKOUT_CFG, parent_name, matching_modes)
            return {}
        else:
            LOGGER.warning("Cannot derive %s for %s: no breakout mode matches preserved PORT layout",
                           BREAKOUT_CFG, parent_name)
            return {}

    unrepresented_ports = set(port_table) - represented_ports
    if unrepresented_ports:
        LOGGER.warning("Cannot derive %s: PORT entries %s are not represented in platform.json",
                       BREAKOUT_CFG, natsorted(unrepresented_ports))
        return {}

    return breakout_cfg


def migrate_config_db(config_db_path, platform_json_path):
    """
    Backfill missing BREAKOUT_CFG into a preserved config_db.json file.

    Args:
        config_db_path: config_db.json path to update.
        platform_json_path: Target image platform.json path.

    Returns:
        bool: True if config_db_path was updated.
    """
    if not os.path.isfile(config_db_path):
        LOGGER.info("%s not found; skip DPB migration", config_db_path)
        return False
    if not os.path.isfile(platform_json_path):
        LOGGER.info("%s not found; skip DPB migration", platform_json_path)
        return False

    config_db = _read_json(config_db_path)
    if BREAKOUT_CFG in config_db:
        existing_breakout_cfg = config_db[BREAKOUT_CFG]
        if not isinstance(existing_breakout_cfg, dict):
            LOGGER.warning("%s has malformed %s; skip", config_db_path, BREAKOUT_CFG)
            return False
        if existing_breakout_cfg:
            LOGGER.info("%s already has non-empty %s; skip", config_db_path, BREAKOUT_CFG)
            return False

    platform_json = _read_json(platform_json_path)
    breakout_cfg = derive_breakout_cfg(config_db, platform_json)
    if not breakout_cfg:
        LOGGER.warning("No %s entries derived for %s", BREAKOUT_CFG, config_db_path)
        return False

    # The pre-migration config remains under /etc/sonic/old_config during image migration.
    config_db[BREAKOUT_CFG] = breakout_cfg
    _write_json(config_db_path, config_db)
    LOGGER.info("Added %s entries to %s", len(breakout_cfg), config_db_path)
    return True


def _build_arg_parser():
    """
    Build the CLI argument parser.

    Returns:
        Configured argparse.ArgumentParser instance.
    """
    parser = argparse.ArgumentParser(description="Cisco DPB config migration helper")
    subparsers = parser.add_subparsers(dest="command", required=True)

    enable_parser = subparsers.add_parser("ensure-enable-dpb")
    enable_parser.add_argument("--config", required=True)
    enable_parser.add_argument("--reference")

    state_parser = subparsers.add_parser("get-enable-dpb")
    state_parser.add_argument("--config", required=True)

    migrate_parser = subparsers.add_parser("migrate-config-db")
    migrate_parser.add_argument("--config-db", required=True)
    migrate_parser.add_argument("--platform-json", required=True)

    return parser


def main():
    """
    Run the requested migration helper command.

    """
    parser = _build_arg_parser()
    args = parser.parse_args()

    if args.command == "ensure-enable-dpb":
        ensure_enable_dpb(args.config, args.reference)
    elif args.command == "get-enable-dpb":
        print("true" if is_dpb_enabled(args.config) else "false")
    elif args.command == "migrate-config-db":
        migrate_config_db(args.config_db, args.platform_json)
    else:
        parser.error("Unsupported command {}".format(args.command))


if __name__ == "__main__":
    try:
        main()
    except Exception as err:
        LOGGER.error("DPB migration failed: %s", err)
        sys.exit(1)
