"""Compose mega's docker_image_ctl.j2 branch and patch every other build
template that references the folded features.

Implements the ``patch-templates`` to-do from the
``generic_mega-container_merge_script`` plan (sections 4, 9--12):

  - **docker_image_ctl.j2**: compose a ``mega`` branch from the ctl_parser
    output -- concatenate per-hook blocks from selected features, pick
    database vs. generic lifecycle, derive the max stop timeout.
  - **sonic_debian_extension.j2**: service alias symlinks for folded
    features → ``mega.service``.
  - **init_cfg.json.j2**: delete folded features' FEATURE rows, add
    ``mega`` row.
  - **syncd.service.j2**: repoint ``After=swss`` → ``After=mega`` (etc.)
    when ``swss`` is folded into mega.
  - **rules/config**: set ``INCLUDE_*`` flags to ``n`` for folded features
    that have one (the mega .mk installs mega unconditionally; we
    don't want the individual docker images installed too).
  - **Each folded feature's .mk**: comment out
    ``SONIC_INSTALL_DOCKER_IMAGES`` lines so the individual docker isn't
    installed alongside mega.
  - **Generic fan-in scan**: grep all ``.service``/``.service.j2`` under
    ``files/build_templates/`` for ``After=``, ``Requires=``, ``BindsTo=``,
    ``Wants=``, ``PartOf=`` referencing any folded feature → repoint those
    references to ``mega``.

This module does **not** write files itself — it returns a ``PatchResult``
containing every file's new text (keyed by path relative to sonic_root),
plus the mega's own generated files (``mega.service.j2``,
``docker_image_ctl.j2`` branch). ``mega_gen_cli.py --apply`` (or a test
harness) is responsible for the actual writes — same pattern as every other
``gen_*``/``*_merge`` module in this package.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, FrozenSet, List, Optional, Sequence, Set, Tuple

from mega_gen.discovery import FeatureSpec
from mega_gen.gen_mega_mk import MEGA_CONTAINER_NAME_DEFAULT

# ---------------------------------------------------------------------------
# Constants.
# ---------------------------------------------------------------------------

# Systemd unit dependency directives that reference other services.
_SYSTEMD_DEP_DIRECTIVES = ("After", "Requires", "BindsTo", "Wants", "PartOf")

# Feature names that are always core (unconditionally in init_cfg.json.j2).
# The FEATURE table uses the container_name directly.
_CORE_FEATURES_IN_INIT_CFG = frozenset({"bgp", "database", "pmon", "swss", "syncd"})

# Mapping from feature name → the INCLUDE_* build var that gates it in
# init_cfg.json.j2 (only for features that have one; core features don't).
# Kept here because init_cfg.json.j2 uses lowercase Jinja vars like
# ``include_lldp`` while rules/config uses uppercase ``INCLUDE_LLDP``.
# This mapping comes straight from reading init_cfg.json.j2 (verified).
_INIT_CFG_INCLUDE_VAR = {
    "radv": "include_router_advertiser",
    "lldp": "include_lldp",
    "snmp": "include_snmp",
    "teamd": "include_teamd",
    "gnmi": "include_system_gnmi",
    "eventd": "include_system_eventd",
}

# The ``rules/config`` uppercase counterpart (from auto_registry).
_RULES_CONFIG_INCLUDE_FLAG = {
    "radv": "INCLUDE_ROUTER_ADVERTISER",
    "lldp": "INCLUDE_LLDP",
    "snmp": "INCLUDE_SNMP",
    "teamd": "INCLUDE_TEAMD",
    "gnmi": "INCLUDE_SYSTEM_GNMI",
    "eventd": "INCLUDE_SYSTEM_EVENTD",
}


# ---------------------------------------------------------------------------
# Output types.
# ---------------------------------------------------------------------------


@dataclass
class PatchResult:
    """Everything ``patch_templates.py`` produces.

    ``file_patches`` maps ``path-relative-to-sonic-root → new file text``
    for every file this module patches or creates. The caller writes them.
    """

    file_patches: Dict[str, str] = field(default_factory=dict)

    mega_service_text: str = ""
    """Generated ``files/build_templates/mega.service.j2`` body."""

    warnings: List[str] = field(default_factory=list)


# ---------------------------------------------------------------------------
# 1. docker_image_ctl.j2 composition.
# ---------------------------------------------------------------------------


def _patch_shared_ctl(
    text: str,
    specs: Sequence[FeatureSpec],
    container_name: str,
) -> Tuple[str, List[str]]:
    """Patch the shared ``files/build_templates/docker_image_ctl.j2`` to add
    mega-specific conditional branches at each hook point, instead of
    generating a standalone file.

    The build system renders a per-container ``.sh`` from this shared template
    with ``docker_container_name`` set to the container name.  Mega needs its
    own ``{%- elif docker_container_name == "mega" %}`` branches at every
    hook point (preStartAction, postStartAction, docker create, lifecycle).

    Returns ``(patched_text, warnings)``.
    """
    warnings: List[str] = []
    selected_names = {s.container_name for s in specs}
    has_database = "database" in selected_names
    has_swss = "swss" in selected_names
    has_teamd = "teamd" in selected_names
    has_bgp = "bgp" in selected_names
    has_pmon = "pmon" in selected_names

    # -----------------------------------------------------------------------
    # 1. preStartAction: insert mega elif before the final {%- else %} in
    #    the preStartAction function.
    # -----------------------------------------------------------------------
    pre_start_block = _mega_pre_start_block(has_database)
    text = _insert_elif_before_else_in_function(
        text, "preStartAction", container_name, pre_start_block, warnings
    )

    # -----------------------------------------------------------------------
    # 2. postStartAction: insert mega elif before the final {%- else %} in
    #    the postStartAction function.
    # -----------------------------------------------------------------------
    post_start_block = _mega_post_start_block(has_database, has_swss, specs)
    text = _insert_elif_before_else_in_function(
        text, "postStartAction", container_name, post_start_block, warnings
    )

    # -----------------------------------------------------------------------
    # 3. Lifecycle: start/wait/stop/kill — change "database" to
    #    ["database", "mega"] for the direct docker start/stop/kill path.
    # -----------------------------------------------------------------------
    # start() — existing container restart path
    text = text.replace(
        '            {%- if docker_container_name == "database" %}\n'
        '            echo "Starting existing ${DOCKERNAME} container"',
        '            {%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
        '            echo "Starting existing ${DOCKERNAME} container"',
        1,
    )
    # start() — create path
    text = text.replace(
        '    {%- if docker_container_name == "database" %}\n'
        "    docker start $DOCKERNAME\n"
        "    {%- else %}\n"
        "    /usr/local/bin/container start ${DOCKERNAME}\n"
        "    {%- endif %}\n"
        "    postStartAction\n"
        "}",
        '    {%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
        "    docker start $DOCKERNAME\n"
        "    {%- else %}\n"
        "    /usr/local/bin/container start ${DOCKERNAME}\n"
        "    {%- endif %}\n"
        "    postStartAction\n"
        "}",
        1,
    )

    # wait()
    old_wait = (
        "wait() {\n"
        '    {%- if docker_container_name == "database" %}\n'
        "    /usr/bin/docker-rs wait $DOCKERNAME"
    )
    new_wait = (
        "wait() {\n"
        '    {%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
        '    {%- if docker_container_name == "' + container_name + '" %}\n'
    )
    if has_swss:
        new_wait += "    /bin/systemctl start syncd$DEV || true\n"
    new_wait += (
        "    {%- endif %}\n"
        "    /usr/bin/docker-rs wait $DOCKERNAME"
    )
    text = text.replace(old_wait, new_wait, 1)

    # stop()
    #
    # NOTE: mega is routed through the same branch as "database" here (bare
    # `docker stop $DOCKERNAME`, no `-t <timeout>`, since "database" itself
    # never uses one) -- so a plain `-t 60`-timeout elif added further down
    # in the OTHER (non-database) branch of this same if/else (as teamd's
    # and swss+asan's own elif-chain is) would be dead code for mega, same
    # class of bug as finding #4's REDIS_MNT elif. Instead, bake the 60s
    # timeout directly into mega's own `docker stop` call here, gated on
    # whichever folded features actually need it.
    stop_timeout_conditions = []
    if has_teamd:
        stop_timeout_conditions.append('docker_container_name == "' + container_name + '"')
    if has_swss:
        stop_timeout_conditions.append(
            'docker_container_name == "' + container_name + '" and enable_asan == "y"'
        )
    old_stop = (
        "stop() {\n"
        '    {%- if docker_container_name == "database" %}\n'
        "    docker stop $DOCKERNAME"
    )
    new_stop = (
        "stop() {\n"
        '    {%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
    )
    if has_swss:
        new_stop += (
            '    {%- if docker_container_name == "' + container_name + '" %}\n'
            "    if systemctl is-active --quiet syncd$DEV 2>/dev/null; then\n"
            '        echo "Co-stopping syncd$DEV"\n'
            "        systemctl stop syncd$DEV || true\n"
            "    fi\n"
            "    {%- endif %}\n"
        )
    if stop_timeout_conditions:
        new_stop += (
            '    {%- if ' + " or ".join(stop_timeout_conditions) + " %}\n"
            "    docker stop -t 60 $DOCKERNAME\n"
            "    {%- else %}\n"
            "    docker stop $DOCKERNAME\n"
            "    {%- endif %}"
        )
    else:
        new_stop += "    docker stop $DOCKERNAME"
    text = text.replace(old_stop, new_stop, 1)

    # kill()
    old_kill = (
        "kill() {\n"
        '    {%- if docker_container_name == "database" %}\n'
        "    docker kill $DOCKERNAME"
    )
    new_kill = (
        "kill() {\n"
        '    {%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
    )
    if has_swss:
        new_kill += (
            '    {%- if docker_container_name == "' + container_name + '" %}\n'
            "    docker kill syncd$DEV 2>/dev/null || true\n"
            "    {%- endif %}\n"
        )
    new_kill += "    docker kill $DOCKERNAME"
    text = text.replace(old_kill, new_kill, 1)

    # -----------------------------------------------------------------------
    # 5. HWSKU / DOCKERMOUNT: mega hosts database (like the "database"
    #    container) but ALSO hosts swss/orchagent when swss is folded in, and
    #    orchagent needs the real HWSKU + /usr/share/sonic/hwsku mount (it
    #    reads port_config.ini/sai.profile/buffer templates from there). So,
    #    unlike plain "database", mega must NOT take the empty-HWSKU branch --
    #    it must compute a real HWSKU/MOUNTPATH like every other non-database
    #    container (left as the `{%- else %}` of the untouched conditional
    #    below). See item 11 for the corresponding docker-create-flags elif
    #    that emits both `$DB_OPT` and the hwsku mount for mega.
    # -----------------------------------------------------------------------
    if has_swss:
        # Extend the swss-only "make the hwsku mount rw if hwsku-init exists"
        # check so it also applies when mega is standing in for swss.
        text = text.replace(
            '        HWSKU_MOUNT_MODE="ro"\n'
            '        {%- if docker_container_name == "swss" %}',
            '        HWSKU_MOUNT_MODE="ro"\n'
            '        {%- if docker_container_name in ["swss", "' + container_name + '"] %}',
            1,
        )

    # -----------------------------------------------------------------------
    # 6. Creating new container echo: database path for mega.
    # -----------------------------------------------------------------------
    text = text.replace(
        '    {%- if docker_container_name == "database" %}\n'
        "\n"
        '    echo "Creating new ${DOCKERNAME} container"',
        '    {%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
        "\n"
        '    echo "Creating new ${DOCKERNAME} container"',
        1,
    )

    # -----------------------------------------------------------------------
    # 7. swss pre-create: extend swss asic_table rendering to cover mega.
    # -----------------------------------------------------------------------
    text = text.replace(
        '    {%- if docker_container_name == "swss" %}\n'
        "    # Generate the asic_table.json",
        '    {%- if docker_container_name in ["swss", "' + container_name + '"] %}\n'
        "    # Generate the asic_table.json",
        1,
    )

    # -----------------------------------------------------------------------
    # 8. REDIS_MNT: unreachable for mega and removed. The whole
    #    "database"/"dash-ha"/else block above is a single if/elif/else
    #    chain, and item 9 (below) extends the *first* branch's condition to
    #    also match mega, so mega always takes the `$DB_OPT` branch and never
    #    reaches this elif/else -- inserting a mega-specific elif here would
    #    just be dead code. `$REDIS_MNT` staying unset for mega is correct:
    #    mega hosts its own redis-server locally (unix socket, no host bind
    #    mount needed), same as why plain "database" doesn't set it either.
    # -----------------------------------------------------------------------

    # -----------------------------------------------------------------------
    # 9. DB_OPT / chassis: mega needs the database path for DB_OPT.
    # -----------------------------------------------------------------------
    text = text.replace(
        '    {%- if docker_container_name == "database" %}\n'
        "    start_chassis_db=0",
        '    {%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
        "    start_chassis_db=0",
        1,
    )

    # -----------------------------------------------------------------------
    # 10. Network mode: non-database containers use NET="container:database$DEV"
    #     but mega hosts database itself, so it needs NET="bridge" like database.
    #     Extend the database-specific branches in the else (multi-ASIC) block.
    # -----------------------------------------------------------------------
    text = text.replace(
        "        {%- if docker_container_name == \"database\" %}\n"
        '        NET="bridge"',
        '        {%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
        '        NET="bridge"',
        1,
    )

    # In the host-mode block: extend database-specific sections
    # dash-ha/database/else chain for multi-ASIC redis mounts
    text = text.replace(
        '        {%- elif docker_container_name != "database" %}',
        '        {%- elif docker_container_name not in ["database", "' + container_name + '"] %}',
        1,
    )

    # -----------------------------------------------------------------------
    # 11. docker create flags: add mega-specific caps/mounts/env inline.
    #
    #     Only truly *additional* ctl-template-level flags belong here --
    #     everything a folded feature's own `.mk` already contributes is
    #     already present in mega's generated `_RUN_OPT` (unioned by
    #     gen_mega_mk.merge_run_opt and rendered via `docker_image_run_opt`
    #     further up in the same `docker create` command), so repeating it
    #     here would give docker a "Duplicate mount point" error. The only
    #     flags that live *exclusively* in this shared template (never in any
    #     `.mk`) are: swss's `-e ASIC_VENDOR`, bgp's frr mount (confirmed via
    #     `rules/docker-fpm-frr.mk` -- it has no `/etc/frr` mount of its own,
    #     only the shared ctl template does), the asan `/var/log/asan` mount,
    #     and pmon's firmware/bmc mounts -- so we extend those existing
    #     per-container-name blocks in place instead of hardcoding a parallel,
    #     duplicate-prone list for mega.
    # -----------------------------------------------------------------------
    mega_create_flags = _mega_docker_create_flags(container_name, has_swss, has_bgp)
    # Insert after the bgp frr mount block (unconditional insertion point --
    # the block itself is a no-op unless bgp is folded in, in which case the
    # anchor's own `{%- if docker_container_name == "bgp" %}` already covers
    # it; mega's flags are gated on its own container name, independently).
    text = text.replace(
        '{%- if docker_container_name == "bgp" %}\n'
        "        -v /etc/sonic/frr/$DEV:/etc/frr:rw \\\n"
        "{%- endif %}",
        '{%- if docker_container_name == "bgp" %}\n'
        "        -v /etc/sonic/frr/$DEV:/etc/frr:rw \\\n"
        "{%- endif %}\n"
        + mega_create_flags,
        1,
    )

    # 11a. swss's ASIC_VENDOR env is already keyed off `docker_container_name
    #      == "swss"` and unconditional text, no patch needed there -- it's
    #      emitted directly by `_mega_docker_create_flags` when `has_swss`.

    # 11b. ASAN mount: extend to mega whenever swss is folded in (mega hosts
    #      orchagent in that case, same as the real swss container). Note the
    #      matching `mkdir -p /var/log/asan` at container-create time is
    #      nested inside the swss-only block that item 7 already extends to
    #      cover mega, so no separate patch is needed for that half.
    if has_swss:
        text = text.replace(
            '{%- if docker_container_name in ["swss", "syncd"] and enable_asan == "y" %}\n'
            "        -v /var/log/asan/:/var/log/asan \\\n"
            "{%- endif -%}",
            '{%- if docker_container_name in ["swss", "syncd", "' + container_name + '"] and enable_asan == "y" %}\n'
            "        -v /var/log/asan/:/var/log/asan \\\n"
            "{%- endif -%}",
            1,
        )

    # 11c. pmon's firmware/bmc mounts: extend to mega whenever pmon is folded
    #      in (these live only in the shared template, never in a `.mk`).
    if has_pmon:
        text = text.replace(
            '{%- if docker_container_name == "pmon" %}\n'
            "    -v /usr/share/sonic/firmware:/usr/share/sonic/firmware:rw \\\n"
            '    $(if [ -d /host/bmc ]; then echo "-v /host/bmc:/host/bmc:rw"; fi) \\\n'
            "    $(get_pmon_device_mounts) \\\n"
            "{%- endif %}",
            '{%- if docker_container_name in ["pmon", "' + container_name + '"] %}\n'
            "    -v /usr/share/sonic/firmware:/usr/share/sonic/firmware:rw \\\n"
            '    $(if [ -d /host/bmc ]; then echo "-v /host/bmc:/host/bmc:rw"; fi) \\\n'
            "    $(get_pmon_device_mounts) \\\n"
            "{%- endif %}",
            1,
        )

    # 11d. DB_OPT / hwsku mount: mega hosts the database (needs `$DB_OPT`'s
    #      chassis/multi-ASIC redis wiring, same as "database") but, unlike
    #      "database", also needs the real hwsku mount whenever swss is
    #      folded in (orchagent reads port_config.ini/sai.profile/buffer
    #      templates from it). Add a dedicated mega elif instead of letting
    #      it fall into either branch verbatim.
    mega_db_opt_lines = ['        $DB_OPT \\']
    if has_swss:
        mega_db_opt_lines.append(
            "        -v /usr/share/sonic/device/$PLATFORM/$HWSKU/$DEV:/usr/share/sonic/hwsku:$HWSKU_MOUNT_MODE \\"
        )
    text = text.replace(
        '{%- if docker_container_name == "database" %}\n'
        "        $DB_OPT \\\n"
        "{%- else %}\n"
        "        -v /var/run/redis-chassis:/var/run/redis-chassis:ro \\\n"
        "        -v /usr/share/sonic/device/$PLATFORM/$HWSKU/$DEV:/usr/share/sonic/hwsku:$HWSKU_MOUNT_MODE \\\n"
        "{%- endif %}",
        '{%- if docker_container_name == "database" %}\n'
        "        $DB_OPT \\\n"
        '{%- elif docker_container_name == "' + container_name + '" %}\n'
        + "\n".join(mega_db_opt_lines) + "\n"
        "{%- else %}\n"
        "        -v /var/run/redis-chassis:/var/run/redis-chassis:ro \\\n"
        "        -v /usr/share/sonic/device/$PLATFORM/$HWSKU/$DEV:/usr/share/sonic/hwsku:$HWSKU_MOUNT_MODE \\\n"
        "{%- endif %}",
        1,
    )

    # -----------------------------------------------------------------------
    # 12. Main body: mega needs the chassisdb/dpudb DEV logic (database path).
    # -----------------------------------------------------------------------
    text = text.replace(
        '{%- if docker_container_name == "database" %}\n'
        'if [ "$DEV" == "chassisdb" ]; then',
        '{%- if docker_container_name in ["database", "' + container_name + '"] %}\n'
        'if [ "$DEV" == "chassisdb" ]; then',
        1,
    )

    # -----------------------------------------------------------------------
    # 13. Stop timeout: handled directly in the `stop()` patch above (mega
    #     is routed through the "database" branch's bare `docker stop`, so
    #     an elif in this OTHER, non-database branch of the same if/else
    #     would be unreachable dead code for mega -- see the comment next
    #     to `stop_timeout_conditions` above).
    # -----------------------------------------------------------------------

    return text, warnings


def _mega_pre_start_block(has_database: bool) -> str:
    """Return the preStartAction body for the mega elif branch."""
    lines = []
    if has_database:
        lines.extend([
            "    WARM_DIR=/host/warmboot$DEV",
            '    if [[ ("$BOOT_TYPE" == "warm" || "$BOOT_TYPE" == "fastfast" || "$BOOT_TYPE" == "express" || "$BOOT_TYPE" == "fast") && -f $WARM_DIR/dump.rdb ]]; then',
            "        docker cp $WARM_DIR/dump.rdb ${DOCKERNAME}:/var/lib/redis/dump.rdb",
            "    else",
            "        echo -n > /tmp/mega_empty_dump.rdb",
            "        docker cp /tmp/mega_empty_dump.rdb ${DOCKERNAME}:/var/lib/redis/dump.rdb",
            "    fi",
        ])
    return "\n".join(lines)


def _mega_post_start_block(
    has_database: bool,
    has_swss: bool,
    specs: Sequence[FeatureSpec],
) -> str:
    """Return the postStartAction body for the mega elif branch."""
    lines = []
    selected_names = {s.container_name for s in specs}

    if has_database:
        lines.extend([
            "    WARM_DIR=/host/warmboot$DEV",
            "",
            "    until [[ ($(docker exec -i ${DOCKERNAME} pgrep -x -c supervisord) -gt 0) && ($($SONIC_DB_CLI PING | grep -c PONG) -gt 0) &&",
            "             ($(docker exec -i ${DOCKERNAME} sonic-db-cli PING | grep -c PONG) -gt 0) ]]; do",
            "        sleep 1",
            "    done",
            "",
            '    if [[ ("$BOOT_TYPE" == "warm" || "$BOOT_TYPE" == "fastfast" || "$BOOT_TYPE" == "express" || "$BOOT_TYPE" == "fast") && -f $WARM_DIR/dump.rdb ]]; then',
            "        mv $WARM_DIR/dump.rdb $WARM_DIR/dump.rdb.old",
            "    else",
            '        if [ -r /etc/sonic/config_db$DEV.json ]; then',
            '            if [ -r /etc/sonic/init_cfg.json ]; then',
            '                $SONIC_CFGGEN -j /etc/sonic/init_cfg.json -j /etc/sonic/config_db$DEV.json --write-to-db',
            "            else",
            '                $SONIC_CFGGEN -j /etc/sonic/config_db$DEV.json --write-to-db',
            "            fi",
            "        fi",
            "",
            '        if [[ "$BOOT_TYPE" == "fast" ]]; then',
            "            $SONIC_DB_CLI ASIC_DB FLUSHDB",
            '            $SONIC_DB_CLI STATE_DB SET "FAST_REBOOT|system" "1" "EX" "180"',
            "        fi",
            "    fi",
            "",
            '    if [ -e /etc/sonic/pending_config_migration ] || [ -e /etc/sonic/pending_config_initialization ]; then',
            '        $SONIC_DB_CLI CONFIG_DB SET "CONFIG_DB_INITIALIZED" "0"',
            "    else",
            '        $SONIC_DB_CLI CONFIG_DB SET "CONFIG_DB_INITIALIZED" "0"',
            '        if [[ -x /usr/local/bin/db_migrator.py ]]; then',
            "            /usr/local/bin/db_migrator.py -o migrate",
            "        fi",
            '        $SONIC_DB_CLI CONFIG_DB SET "CONFIG_DB_INITIALIZED" "1"',
            "    fi",
            "",
            '{%- if sonic_asic_platform != "vs" %}',
            "    ebtables_config",
            "{%- endif %}",
            "",
            '    REDIS_SOCK="/var/run/redis${DEV}/redis.sock"',
            '    chgrp -f redis $REDIS_SOCK && chmod -f 0760 $REDIS_SOCK',
        ])

    if has_swss:
        lines.extend([
            "",
            "    docker exec ${DOCKERNAME} rm -f /ready",
            '    if [[ "$BOOT_TYPE" == "fast" ]] && [[ -d /host/fast-reboot ]]; then',
            "        test -e /host/fast-reboot/fdb.json && docker cp /host/fast-reboot/fdb.json ${DOCKERNAME}:/",
            "        test -e /host/fast-reboot/arp.json && docker cp /host/fast-reboot/arp.json ${DOCKERNAME}:/",
            "        test -e /host/fast-reboot/default_routes.json && docker cp /host/fast-reboot/default_routes.json ${DOCKERNAME}:/",
            "        test -e /host/fast-reboot/media_config.json && docker cp /host/fast-reboot/media_config.json ${DOCKERNAME}:/",
            "        rm -fr /host/fast-reboot",
            "    fi",
            "    docker exec ${DOCKERNAME} touch /ready",
        ])

    if "snmp" in selected_names:
        lines.extend([
            "",
            "    if decode-syseeprom -s > /dev/null 2>&1; then",
            "        $SONIC_DB_CLI STATE_DB HSET 'DEVICE_METADATA|localhost' chassis_serial_number $(decode-syseeprom -s)",
            "    else",
            '        echo "Cannot fetch system eeprom information. Setting chassis serial number to N/A."',
            "        $SONIC_DB_CLI STATE_DB HSET 'DEVICE_METADATA|localhost' chassis_serial_number \"N/A\"",
            "    fi",
        ])

    if "eventd" in selected_names:
        lines.extend([
            "",
            '    HOST_RSYSLOG_PLUGIN_CONF_FILE="/etc/rsyslog.d/host_events.conf"',
            "    if [ ! -f ${HOST_RSYSLOG_PLUGIN_CONF_FILE} ]; then",
            "        for f in $(docker exec -i ${DOCKERNAME} ls /etc/rsyslog.d/rsyslog_plugin_conf 2>/dev/null); do docker cp ${DOCKERNAME}:/etc/rsyslog.d/rsyslog_plugin_conf/$f /etc/rsyslog.d/; done",
            "        systemctl reset-failed rsyslog",
            "        systemctl restart rsyslog",
            "    fi",
        ])

    return "\n".join(lines)


def _mega_docker_create_flags(container_name: str, has_swss: bool, has_bgp: bool) -> str:
    """Return the docker create flags block for the mega container.

    Deliberately *not* a copy of `_RUN_OPT`'s caps/security-opts/generic bind
    mounts (`-t`, `--cap-add=...`, `--security-opt=...`, `/etc/sonic`,
    `/etc/localtime`, `/etc/network/interfaces[.d]`, `/host/machine.conf`,
    `/var/log/swss`, `/zmq_swss`, `/var/run/dbus`, `/var/run/gnmi`,
    `/:/mnt/host`, `/tmp:/mnt/host/tmp`, `/var/tmp:/mnt/host/var/tmp`) --
    every one of those is already unioned into mega's own `_RUN_OPT` by
    `gen_mega_mk.merge_run_opt` from the folded features' own `.mk` files,
    and `docker_image_run_opt` (which already includes the full `_RUN_OPT`
    union) is rendered earlier in the same `docker create` command. Repeating
    any `-v SRC:DST[:MODE]` mount here would give docker a "Duplicate mount
    point" error at container-create time; repeating a non-mount flag
    (`-t`/`--cap-add`/`--security-opt`) wouldn't error, but is still
    pointless duplication.

    The only flags that live *exclusively* in this shared ctl template (never
    in any feature's own `.mk`, so never unioned) are handled here or as
    separate, reusable per-container-name extensions in `_patch_shared_ctl`:
    the ASAN `/var/log/asan` mount (item 11b), pmon's firmware/bmc mounts
    (item 11c), and the `$DB_OPT`/hwsku mount pairing (item 11d). bgp's own
    `/etc/frr` mount is emitted here (NOT reused from the real "bgp" anchor,
    which never matches `docker_container_name == "mega"`) -- with the
    correct `/etc/sonic/frr/$DEV` path (the original hardcoded copy of this
    had a typo, `/etc/sonic/frr$DEV`, missing the slash before `$DEV`).
    """
    lines = ['{%- if docker_container_name == "' + container_name + '" %}']
    if has_swss:
        lines.append("        -e ASIC_VENDOR={{ sonic_asic_platform }} \\")
    if has_bgp:
        lines.append("        -v /etc/sonic/frr/$DEV:/etc/frr:rw \\")
    lines.append("{%- endif %}")
    return "\n".join(lines)


def _insert_elif_before_else_in_function(
    text: str,
    func_name: str,
    container_name: str,
    body: str,
    warnings: List[str],
) -> str:
    """In the named function, insert a ``{%- elif docker_container_name == "mega" %}``
    branch with ``body`` just before the ``{%- else %}`` that precedes ``{%- endif %}``.
    """
    # Find the function
    func_pat = re.compile(rf"^function {re.escape(func_name)}\(\)\s*$", re.MULTILINE)
    func_m = func_pat.search(text)
    if not func_m:
        warnings.append(
            f"docker_image_ctl.j2: could not find 'function {func_name}()' "
            f"— mega branch NOT inserted"
        )
        return text

    # From the function start, find the last {%- else %} ... {%- endif %} pair
    # before the closing } of the function.  The pattern is:
    #   {%- else %}
    #       : # nothing
    #   {%- endif %}
    #       updateSyslogConf  (preStartAction) or /etc/resolvconf/... (postStartAction) or just }
    func_start = func_m.start()
    # Find the closing brace of the function (first line starting with } after func)
    close_pat = re.compile(r"^\}", re.MULTILINE)
    close_m = close_pat.search(text, func_start + 1)
    if not close_m:
        warnings.append(
            f"docker_image_ctl.j2: could not find closing '}}' for "
            f"'function {func_name}()' — mega branch NOT inserted"
        )
        return text

    func_region = text[func_start:close_m.end()]

    # Find the LAST {%- else %} in this function region
    else_offsets = [m.start() for m in re.finditer(r"\{%-?\s*else\s*-?%\}", func_region)]
    if not else_offsets:
        warnings.append(
            f"docker_image_ctl.j2: no '{{% else %}}' found in "
            f"'function {func_name}()' — mega branch NOT inserted"
        )
        return text

    last_else_offset = else_offsets[-1]
    abs_else = func_start + last_else_offset

    # Insert the elif block before the else
    if body.strip():
        elif_block = (
            '{%- elif docker_container_name == "' + container_name + '" %}\n'
            + body + "\n"
        )
    else:
        elif_block = (
            '{%- elif docker_container_name == "' + container_name + '" %}\n'
            "    : # nothing\n"
        )

    text = text[:abs_else] + elif_block + text[abs_else:]

    return text


# ---------------------------------------------------------------------------
# 2. init_cfg.json.j2 patching.
# ---------------------------------------------------------------------------


def _patch_init_cfg(
    text: str,
    folded_features: FrozenSet[str],
    container_name: str,
) -> Tuple[str, List[str]]:
    """Delete folded features from the ``features`` list and the
    ``FEATURE`` table; add ``mega`` as ``always_enabled``.

    Plan section 11: "folded features' tuples deleted; ``mega`` added as
    ``("mega", "always_enabled", false, "enabled")``."

    Two-pass algorithm:
      Pass 1 — identify lines to delete:
        a. Core feature tuples inside ``set features = [...]``: inline
           removal from the list literal (single-line or continuation-line).
        b. Toggled features: find every ``features.append(("FEAT",`` line
           for a folded feature, then walk outward to find the enclosing
           ``{%- if %}``/``{%- endif %}`` pair and mark the whole range.
      Pass 2 — emit surviving lines, add mega's append.
    """
    warnings: List[str] = []
    lines = text.split("\n")
    deleted_features: Set[str] = set()
    lines_to_delete: Set[int] = set()

    # --- Pass 1a: core features in ``set features = [...]`` ---
    # These are tuples inside the initial list literal. They can be on the
    # ``set features`` line itself (bgp) or on continuation lines.

    # Regex that matches a complete (name, state, delayed, autorestart) tuple.
    # State/delayed fields may contain nested Jinja ``{% ... %}`` blocks.
    def _tuple_pat(feat: str) -> re.Pattern:
        return re.compile(
            rf'\("{re.escape(feat)}",\s*'
            rf'"(?:[^"]*(?:\{{% [^%]*%\}}[^"]*)*)",\s*'  # state (may have Jinja)
            rf'(?:false|true|"(?:[^"]*(?:\{{% [^%]*%\}}[^"]*)*)")\s*,\s*'  # delayed
            rf'"[^"]*"'  # autorestart
            rf'\)\s*,?'
        )

    for i, line in enumerate(lines):
        if "set features" not in line and not re.match(r"^\s+\(", line):
            continue
        for feat in folded_features:
            if feat in deleted_features:
                continue
            pat = _tuple_pat(feat)
            m = pat.search(line)
            if m is None:
                continue
            if "set features" in line:
                # On the set-features line: remove inline (keep the line).
                lines[i] = pat.sub("", line).rstrip()
                # Clean up leftover empty list: ``[  ] %}`` → ``[] %}``
                lines[i] = re.sub(r"\[\s*\]", "[]", lines[i])
            else:
                # Continuation line: mark entire line for deletion.
                lines_to_delete.add(i)
            deleted_features.add(feat)

    # --- Pass 1b: toggled features (``{% do features.append(...) %}``) ---
    # For each folded feature not yet handled, find the ``features.append``
    # line(s) and walk outward to find the enclosing ``{%- if … %}`` /
    # ``{%- endif %}`` pair.

    for feat in folded_features:
        if feat in deleted_features:
            continue
        append_lines = [
            i for i, line in enumerate(lines)
            if f'features.append(("{feat}"' in line
        ]
        if not append_lines:
            continue

        for al in append_lines:
            # One-liner: if/do/endif all on one line.
            line = lines[al]
            if (re.search(r"\{%-?\s*if\s+", line) and
                    re.search(r"\{%-?\s*endif\s*-?%\}", line)):
                lines_to_delete.add(al)
                deleted_features.add(feat)
                continue

            # Multi-line: walk backward to find the opening ``{%- if %}``,
            # and forward to find the closing ``{%- endif %}``.
            block_start = al
            for j in range(al - 1, max(al - 10, -1), -1):
                if re.search(r"\{%-?\s*if\s+", lines[j]):
                    block_start = j
                    break
            block_end = al
            depth = 0
            for j in range(block_start, min(block_start + 20, len(lines))):
                if re.search(r"\{%-?\s*if\s+", lines[j]):
                    depth += 1
                if re.search(r"\{%-?\s*endif\s*-?%\}", lines[j]):
                    depth -= 1
                    if depth <= 0:
                        block_end = j
                        break
            for j in range(block_start, block_end + 1):
                lines_to_delete.add(j)
            deleted_features.add(feat)

    # --- Pass 2: emit surviving lines ---
    out_lines = [
        line for i, line in enumerate(lines) if i not in lines_to_delete
    ]

    for feat in folded_features:
        if feat not in deleted_features:
            warnings.append(
                f"init_cfg.json.j2: feature '{feat}' not found in the "
                f"features list -- it may not be present in this template "
                f"(e.g. sysmgr has no FEATURE row by default)"
            )

    new_text = "\n".join(out_lines)

    # --- Add mega as a core (always_enabled) feature ---
    feature_key_pat = re.compile(r'^(\s*"FEATURE":\s*\{)', re.MULTILINE)
    m = feature_key_pat.search(new_text)
    if m:
        mega_append_line = (
            f'{{% do features.append(("{container_name}", "always_enabled", false, "enabled")) %}}\n'
        )
        insert_pos = m.start()
        new_text = new_text[:insert_pos] + mega_append_line + new_text[insert_pos:]
    else:
        warnings.append(
            "init_cfg.json.j2: could not locate the '\"FEATURE\":' key to "
            "insert mega's tuple before it -- mega feature row NOT added"
        )

    return new_text, warnings


# ---------------------------------------------------------------------------
# 3. syncd.service.j2 patching.
# ---------------------------------------------------------------------------


def _patch_syncd_service(
    text: str,
    folded_features: FrozenSet[str],
    container_name: str,
) -> Tuple[str, List[str]]:
    """Repoint systemd dependency directives in ``syncd.service.j2`` from
    folded feature names to ``mega``.

    F18 fix: use ``Wants=`` instead of ``Requires=`` for syncd→mega.
    The reference PoC discovered that ``Requires=`` causes every mega
    restart to dependency-stop syncd, suppressing ``Restart=`` and tripping
    ``StartLimit`` → syncd stuck dead, ASIC unprogrammed.  ``Wants=`` +
    ``Restart=always`` + ``StartLimitIntervalSec=0`` lets syncd cleanly
    co-restart.

    F19 fix: deduplicate ``After=mega.service`` lines that arise from
    both ``swss`` and ``database`` being repointed to ``mega``.
    """
    warnings: List[str] = []
    new_text = _repoint_service_refs(text, folded_features, container_name, warnings)

    # F18: Requires=mega → Wants=mega (soft dependency)
    new_text = re.sub(
        r"^Requires=" + re.escape(container_name),
        f"Wants={container_name}",
        new_text,
        flags=re.MULTILINE,
    )

    # F18: ensure StartLimitIntervalSec=0 is present (prevents startup
    # throttling after a mega restart cascade)
    if "StartLimitIntervalSec" not in new_text:
        new_text = re.sub(
            r"^(\[Unit\]\n(?:.*\n)*?)(Description=)",
            rf"\g<1>StartLimitIntervalSec=0\n\g<2>",
            new_text,
            count=1,
        )
        # Simpler fallback: just add after [Unit]
        if "StartLimitIntervalSec" not in new_text:
            new_text = new_text.replace(
                "[Unit]\n",
                "[Unit]\nStartLimitIntervalSec=0\n",
                1,
            )

    return new_text, warnings


# ---------------------------------------------------------------------------
# 4. rules/config patching.
# ---------------------------------------------------------------------------


def _patch_rules_config(
    text: str,
    folded_features: Sequence[FeatureSpec],
    container_name: str,
) -> Tuple[str, List[str]]:
    """Set ``INCLUDE_*`` flags to ``n`` for every folded feature that has
    one. The individual docker images must not be installed alongside mega.
    """
    warnings: List[str] = []
    new_text = text

    for spec in folded_features:
        flag = spec.include_flag
        if flag is None:
            continue
        # Pattern: ``INCLUDE_LLDP ?= y`` or ``INCLUDE_LLDP = y``
        pat = re.compile(
            rf"^({re.escape(flag)}\s*\??=\s*).*$",
            re.MULTILINE,
        )
        new_text, n = pat.subn(rf"\g<1>n", new_text)
        if n == 0:
            warnings.append(
                f"rules/config: could not find '{flag}' to set to 'n' -- "
                f"it may already be absent or use an unexpected format"
            )

    return new_text, warnings


# ---------------------------------------------------------------------------
# 5. Per-feature .mk commenting.
# ---------------------------------------------------------------------------


def _patch_feature_mk(
    text: str,
    spec: FeatureSpec,
) -> Tuple[str, List[str]]:
    """Comment out ``SONIC_INSTALL_DOCKER_IMAGES`` lines in a folded
    feature's own ``.mk`` so its individual docker image is not installed.

    Also comments out ``SONIC_INSTALL_DOCKER_DBG_IMAGES`` lines for the
    debug image (same reasoning).
    """
    warnings: List[str] = []
    docker_var = spec.registry_entry.docker_var

    for list_name in ("SONIC_INSTALL_DOCKER_IMAGES", "SONIC_INSTALL_DOCKER_DBG_IMAGES"):
        pat = re.compile(
            rf"^(\s*{re.escape(list_name)}\s*\+=\s*\$\({re.escape(docker_var)}(?:_DBG)?\))\s*$",
            re.MULTILINE,
        )
        def _comment_line(m: re.Match) -> str:
            return f"# (mega-gen: folded into mega) # {m.group(1)}"
        text, n = pat.subn(_comment_line, text)
        if n == 0 and list_name == "SONIC_INSTALL_DOCKER_IMAGES":
            # It might be inside an ifeq block -- try commenting the
            # inner SONIC_INSTALL_DOCKER_IMAGES line anyway.
            pass

    return text, warnings


# ---------------------------------------------------------------------------
# 6. Generic fan-in scan.
# ---------------------------------------------------------------------------


def _repoint_service_refs(
    text: str,
    folded_features: FrozenSet[str],
    container_name: str,
    warnings: List[str],
) -> str:
    """Rewrite systemd unit dependency lines: any reference to a folded
    feature's ``.service`` is repointed to ``<container_name>.service``.

    Handles both plain and multi-instance (``@%i``) service names, and
    Jinja2-templated variants (``{%- if ... %}@%i{%- endif %}``).

    After repointing, deduplicates repeated ``mega.service`` references
    within each directive line (e.g. ``After=mega.service mega.service
    syncd.service`` → ``After=mega.service syncd.service``).
    """
    new_text = text
    for feat in sorted(folded_features):
        # Replace the feature name with the container name everywhere
        # it appears as a service reference.
        pat = re.compile(
            rf"(?<![a-zA-Z0-9_-]){re.escape(feat)}"
            rf"((?:@%i|(?:\{{% if .*?%\}}@%i\{{% endif %\}}))?\.service)"
        )
        def _repl(m: re.Match, _cn: str = container_name) -> str:
            return f"{_cn}{m.group(1)}"
        new_text = pat.sub(_repl, new_text)

    # Deduplicate repeated service references within each directive line.
    for directive in _SYSTEMD_DEP_DIRECTIVES:
        def _dedup_line(m: re.Match) -> str:
            prefix = m.group(1)
            services_str = m.group(2)
            # Split on whitespace, preserve order, deduplicate.
            services = services_str.split()
            seen: List[str] = []
            for s in services:
                if s not in seen:
                    seen.append(s)
            return f"{prefix}{' '.join(seen)}"
        pat_dedup = re.compile(
            rf"^({re.escape(directive)}=)(.+)$",
            re.MULTILINE,
        )
        new_text = pat_dedup.sub(_dedup_line, new_text)

    # F19 fix: deduplicate identical WHOLE lines (e.g. two separate
    # ``After=mega.service`` lines that arose from database + swss both
    # being repointed to mega).  Preserve first occurrence, drop exact
    # duplicates.
    out_lines: List[str] = []
    seen_lines: set = set()
    for line in new_text.split("\n"):
        stripped = line.strip()
        is_directive = any(stripped.startswith(d + "=") for d in _SYSTEMD_DEP_DIRECTIVES)
        if is_directive and stripped in seen_lines:
            continue
        if is_directive:
            seen_lines.add(stripped)
        out_lines.append(line)
    new_text = "\n".join(out_lines)

    return new_text


def _fan_in_scan(
    sonic_root: Path,
    folded_features: FrozenSet[str],
    container_name: str,
) -> Tuple[Dict[str, str], List[str]]:
    """Scan all ``.service``/``.service.j2`` files under
    ``files/build_templates/`` for systemd dependency references to any
    folded feature. Returns ``{relpath: new_text}`` for files that changed,
    plus warnings.
    """
    warnings: List[str] = []
    patches: Dict[str, str] = {}

    templates_dir = sonic_root / "files" / "build_templates"
    service_files = sorted(templates_dir.rglob("*.service.j2")) + sorted(
        templates_dir.rglob("*.service")
    )

    for svc_path in service_files:
        text = svc_path.read_text(errors="replace")
        has_ref = False
        for feat in folded_features:
            if f"{feat}.service" in text or f"{feat}@" in text:
                has_ref = True
                break
        if not has_ref:
            continue

        new_text = _repoint_service_refs(text, folded_features, container_name, warnings)
        if new_text != text:
            relpath = str(svc_path.relative_to(sonic_root))
            patches[relpath] = new_text

    return patches, warnings


# ---------------------------------------------------------------------------
# 7. sonic_debian_extension.j2 patching.
# ---------------------------------------------------------------------------


def _patch_sonic_debian_extension(
    text: str,
    folded_features: FrozenSet[str],
    container_name: str,
) -> Tuple[str, List[str]]:
    """Add service alias symlinks and a service-registration filter for
    features folded into mega in ``sonic_debian_extension.j2``.

    F17 fix: in addition to the alias symlinks, wrap the installer_services
    loop with a keep-list filter so folded features' ``.service`` units are
    NOT registered alongside mega (the aliases handle backward-compat refs).
    The reference PoC does this with a ``mega_keep_services`` set.

    Symlinks + filter are injected just before the
    ``# PLATFORM-SPECIFIC SERVICE FILTERING`` comment at the end.
    """
    warnings: List[str] = []

    # F17: add a keep-list filter around the service registration loop.
    # The stock template has:
    #   {% for service in installer_services.split(' ') -%}
    #   if [ -f {{service}} ]; then ...
    # We inject a {% if service.strip('"') in mega_keep_services %} guard.
    keep_services = sorted({"syncd.service", "syncd@.service",
                            f"{container_name}.service"} |
                           {s for s in ["pmon.service"] if s.split(".")[0] not in folded_features})
    keep_set_line = (
        '{%- set mega_keep_services = ['
        + ', '.join(f"'{s}'" for s in sorted(keep_services))
        + '] -%}'
    )

    # Find the service registration loop and inject the guard
    loop_marker = "{% for service in installer_services.split(' ') -%}"
    loop_idx = text.find(loop_marker)
    endfor_marker = "{% endfor %}"

    if loop_idx >= 0:
        # Find the endfor that closes this loop (first one after the loop)
        endfor_idx = text.find(endfor_marker, loop_idx)
        if endfor_idx >= 0:
            # Insert the keep-set definition before the loop
            # Insert {% if %} right after the for line, and {% endif %} before endfor
            new_text = (
                text[:loop_idx]
                + "{# mega-gen: only register surviving services; folded ones get alias symlinks below #}\n"
                + keep_set_line + "\n"
                + loop_marker + "\n"
                + "{% if service.strip('\"') in mega_keep_services -%}\n"
                + text[loop_idx + len(loop_marker) + 1:endfor_idx]
                + "{% endif -%}\n"
                + endfor_marker
                + text[endfor_idx + len(endfor_marker):]
            )
        else:
            new_text = text
            warnings.append(
                "sonic_debian_extension.j2: found the service loop but not "
                "its closing endfor -- service filter NOT applied"
            )
    else:
        new_text = text
        warnings.append(
            "sonic_debian_extension.j2: could not find the service "
            "registration loop -- service filter NOT applied"
        )

    # Build the alias symlink block
    alias_lines = [
        "",
        "# --- mega-gen: service alias symlinks for features folded into mega ---",
        "# These ensure systemd references to folded features resolve to mega.service.",
    ]
    # Only alias database + swss (the two that host-infra units depend on);
    # the other folded features have their INCLUDE_* set to n so their
    # services are never generated in the first place.
    core_aliases = {"database", "swss"} & folded_features
    for feat in sorted(core_aliases):
        alias_lines.append(
            f'sudo ln -sf {container_name}.service '
            f'$FILESYSTEM_ROOT_USR_LIB_SYSTEMD_SYSTEM/{feat}.service'
        )
    alias_lines.append("")

    alias_block = "\n".join(alias_lines)

    # Insert before the PLATFORM-SPECIFIC SERVICE FILTERING section
    marker = "# PLATFORM-SPECIFIC SERVICE FILTERING"
    marker_idx = new_text.find(marker)
    if marker_idx >= 0:
        # Walk back to include the leading comment fence
        fence_start = new_text.rfind("#####", 0, marker_idx)
        if fence_start >= 0:
            insert_pos = fence_start
        else:
            insert_pos = marker_idx
        new_text = new_text[:insert_pos] + alias_block + "\n" + new_text[insert_pos:]
    else:
        # Fallback: append to end
        new_text = new_text + alias_block
        warnings.append(
            "sonic_debian_extension.j2: could not find "
            "'PLATFORM-SPECIFIC SERVICE FILTERING' marker -- "
            "alias symlinks appended to end of file"
        )

    return new_text, warnings


# ---------------------------------------------------------------------------
# 8. mega.service.j2 generation.
# ---------------------------------------------------------------------------


def _generate_mega_service(
    folded_features: FrozenSet[str],
    container_name: str,
    has_database: bool,
) -> str:
    """Generate ``files/build_templates/mega.service.j2``.

    Dependencies: ``database.service`` (unless mega IS database), plus
    ``config-setup.service``, and all core requirements.
    """
    lines = [
        "[Unit]",
        f"Description={container_name} container (mega - auto-generated)",
    ]

    deps = ["config-setup.service"]
    after = ["config-setup.service"]

    if has_database:
        deps.insert(0, "docker.service")
        after.insert(0, "docker.service")
        after.append("rc-local.service")
    else:
        deps.insert(0, "database.service")
        after.insert(0, "database.service")

    lines.append(f"Requires={' '.join(deps)}")
    lines.append(f"After={' '.join(after)}")
    lines.append("BindsTo=sonic.target")
    lines.append("After=sonic.target")
    lines.append("StartLimitIntervalSec=0")
    lines.append("")
    lines.append("[Service]")
    lines.append("User=root")
    lines.append(f"Environment=sonic_asic_platform={{{{ sonic_asic_platform }}}}")
    lines.append(f"ExecStartPre=/usr/bin/{container_name}.sh start")
    lines.append(f"ExecStart=/usr/bin/{container_name}.sh wait")
    lines.append(f"ExecStop=/usr/bin/{container_name}.sh stop")
    lines.append("Restart=always")
    lines.append("RestartSec=10")
    lines.append("")
    lines.append("[Install]")
    lines.append("WantedBy=sonic.target")
    lines.append("")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Public entry point.
# ---------------------------------------------------------------------------


def generate_patches(
    specs: Sequence[FeatureSpec],
    sonic_root: Path,
    container_name: str = MEGA_CONTAINER_NAME_DEFAULT,
) -> PatchResult:
    """Generate all template patches for the mega container.

    ``specs`` is the already-discovered list of features being folded into
    mega (see ``mega_gen.discovery.discover_features``).

    Returns a ``PatchResult`` with ``file_patches`` keyed by path relative
    to ``sonic_root``.
    """
    result = PatchResult()
    folded_names: FrozenSet[str] = frozenset(s.container_name for s in specs)
    has_database = "database" in folded_names

    # --- 1. docker_image_ctl.j2 --- patch the SHARED template ---
    ctl_path = sonic_root / "files" / "build_templates" / "docker_image_ctl.j2"
    if ctl_path.is_file():
        ctl_text = ctl_path.read_text(errors="replace")
        new_ctl_text, ctl_warnings = _patch_shared_ctl(ctl_text, specs, container_name)
        result.warnings.extend(ctl_warnings)
        result.file_patches["files/build_templates/docker_image_ctl.j2"] = new_ctl_text
    else:
        result.warnings.append(
            "docker_image_ctl.j2 not found at "
            "files/build_templates/docker_image_ctl.j2 — mega branches "
            "NOT inserted"
        )

    # --- 2. init_cfg.json.j2 ---
    init_cfg_path = sonic_root / "files" / "build_templates" / "init_cfg.json.j2"
    if init_cfg_path.is_file():
        text = init_cfg_path.read_text(errors="replace")
        new_text, w = _patch_init_cfg(text, folded_names, container_name)
        result.warnings.extend(w)
        result.file_patches["files/build_templates/init_cfg.json.j2"] = new_text

    # --- 3. syncd.service.j2 ---
    syncd_path = sonic_root / "files" / "build_templates" / "per_namespace" / "syncd.service.j2"
    if syncd_path.is_file():
        text = syncd_path.read_text(errors="replace")
        new_text, w = _patch_syncd_service(text, folded_names, container_name)
        result.warnings.extend(w)
        if new_text != text:
            result.file_patches[
                "files/build_templates/per_namespace/syncd.service.j2"
            ] = new_text

    # --- 4. rules/config ---
    config_path = sonic_root / "rules" / "config"
    if config_path.is_file():
        text = config_path.read_text(errors="replace")
        new_text, w = _patch_rules_config(text, specs, container_name)
        result.warnings.extend(w)
        if new_text != text:
            result.file_patches["rules/config"] = new_text

    # --- 5. Per-feature .mk ---
    for spec in specs:
        mk_path = sonic_root / spec.mk_file
        if not mk_path.is_file():
            result.warnings.append(
                f"feature .mk: {spec.mk_file} not found -- skipping"
            )
            continue
        text = mk_path.read_text(errors="replace")
        new_text, w = _patch_feature_mk(text, spec)
        result.warnings.extend(w)
        if new_text != text:
            result.file_patches[spec.mk_file] = new_text

    # --- 6. Generic fan-in scan ---
    fan_in_patches, fan_in_warnings = _fan_in_scan(
        sonic_root, folded_names, container_name
    )
    result.warnings.extend(fan_in_warnings)
    result.file_patches.update(fan_in_patches)

    # --- 7. sonic_debian_extension.j2 ---
    ext_path = sonic_root / "files" / "build_templates" / "sonic_debian_extension.j2"
    if ext_path.is_file():
        text = ext_path.read_text(errors="replace")
        new_text, w = _patch_sonic_debian_extension(text, folded_names, container_name)
        result.warnings.extend(w)
        if new_text != text:
            result.file_patches[
                "files/build_templates/sonic_debian_extension.j2"
            ] = new_text

    # --- 8. mega.service.j2 ---
    result.mega_service_text = _generate_mega_service(
        folded_names, container_name, has_database
    )
    result.file_patches[
        f"files/build_templates/{container_name}.service.j2"
    ] = result.mega_service_text

    return result


# ---------------------------------------------------------------------------
# Standalone smoke test.
#
#   python3 -m mega_gen.patch_templates \
#       --sonic-root /path/to/sonic-buildimage \
#       --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    import argparse
    import sys

    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from mega_gen.auto_registry import discover_registry, resolve_features
    from mega_gen.discovery import discover_features as _discover_features

    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sonic-root", required=True, type=Path)
    ap.add_argument(
        "--features",
        default="database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr",
    )
    ap.add_argument("--container-name", default=MEGA_CONTAINER_NAME_DEFAULT)
    ap.add_argument("--out-dir", type=Path, default=None)
    args = ap.parse_args()

    registry = discover_registry(args.sonic_root)
    resolved = resolve_features(args.features.split(","), registry)
    if resolved.unknown or resolved.excluded:
        print(
            f"ERROR: unknown={resolved.unknown} excluded={resolved.excluded}",
            file=sys.stderr,
        )
        sys.exit(1)

    _specs = _discover_features(args.sonic_root, resolved.resolved, registry)
    _result = generate_patches(
        _specs, args.sonic_root, container_name=args.container_name
    )

    print(f"=== file_patches ({len(_result.file_patches)}) ===")
    for relpath in sorted(_result.file_patches):
        text = _result.file_patches[relpath]
        print(f"  {relpath} ({len(text)} bytes, {len(text.splitlines())} lines)")

    print(f"\n=== warnings ({len(_result.warnings)}) ===")
    for w in _result.warnings:
        print(f"  WARNING: {w}")

    if args.out_dir:
        for relpath, text in _result.file_patches.items():
            out_path = args.out_dir / relpath
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_text(text)
            print(f"wrote {out_path}")
    else:
        print(
            "\n(dry run: nothing written. Pass --out-dir to write "
            "generated files.)"
        )
