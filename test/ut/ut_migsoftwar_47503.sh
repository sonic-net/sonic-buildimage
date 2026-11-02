#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO="${REPO:-$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)}"
RULES="$REPO/rules/docker-database.mk"
LAUNCHER="$REPO/files/build_templates/docker_image_ctl.j2"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

fail() {
    echo "[FAIL] $*" >&2
    exit 1
}

pass() {
    echo "[PASS] $*"
}

# The lifecycle cases below source the rendered launcher, which reads host paths under /etc/sonic.
if [[ -d /usr/share/sonic/device ]]; then
    echo "[SKIP] refusing to run launcher lifecycle tests on a SONiC device" >&2
    exit 0
fi

DB_RUN_OPT=$(sed -n 's/^\$(DOCKER_DATABASE)_RUN_OPT += //p' "$RULES" | tr '\n' ' ')

render() {
    python3 - "$LAUNCHER" "$1" "$2" "$3" <<'EOF'
import os, sys, jinja2
tpl, name, platform, run_opt = sys.argv[1:]
env = jinja2.Environment(loader=jinja2.FileSystemLoader(os.path.dirname(tpl)),
                         undefined=jinja2.StrictUndefined, keep_trailing_newline=True)
sys.stdout.write(env.get_template(os.path.basename(tpl)).render(
    docker_container_name=name, docker_image_run_opt=run_opt, docker_image_name='docker-' + name,
    sonic_asic_platform=platform, install_debug_image='n', enable_asan='n'))
EOF
}

for platform in broadcom mellanox marvell-teralynx vs; do
    render database "$platform" "$DB_RUN_OPT" > "$WORK/database.$platform.sh"
    bash -n "$WORK/database.$platform.sh" || fail "rendered database launcher for $platform has a syntax error"
done
pass 'database launcher renders and parses for broadcom, mellanox, marvell-teralynx and vs'

render swss broadcom '-t --privileged' > "$WORK/swss.sh"
grep -Fq 'HostConfig.Privileged' "$WORK/swss.sh" && fail 'privileged-container recreation leaked into a non-database launcher'
grep -Fq 'DOCKERMOUNT=`getMountPoint "$DOCKERCHECK"`' "$WORK/swss.sh" || fail 'non-database HWSKU lifecycle changed'
pass 'non-database launchers keep the shared HWSKU-based lifecycle'

# Drop the trailing command dispatch so start() can be driven with mocked helpers.
sed '/^case "\$1" in/,$d' "$WORK/database.broadcom.sh" > "$WORK/database.sh"

mkdir -p "$WORK/bin"
cat > "$WORK/bin/docker" <<'EOF'
#!/usr/bin/env bash
state=$(cat "$MOCK_STATE")
case "$1" in
    ps)
        echo "ps ${*: -1}" >> "$MOCK_LOG"
        case "$state" in
            absent) ;;
            error)  echo "Cannot connect to the Docker daemon at unix:///var/run/docker.sock." >&2; exit 1 ;;
            *)      echo "0123456789abcdef" ;;
        esac
        ;;
    inspect)
        if [[ "$*" == *State.Pid* ]]; then echo 4242; exit 0; fi
        echo "inspect ${*: -1}" >> "$MOCK_LOG"
        case "$state" in
            inspect-error) echo "Error response from daemon: internal error" >&2; exit 1 ;;
            *)             echo "$state" ;;
        esac
        ;;
    exec)
        echo "exec ${*:2}" >> "$MOCK_LOG"
        case "$*" in
            *" cat "*)        if [[ -n "${MOCK_CAT_FAIL:-}" ]]; then exit 1; fi; cat "$MOCK_SYSCTL_SRC" ;;
            *pgrep*)          echo 1 ;;
            *"sonic-db-cli PING"*) echo PONG ;;
        esac
        ;;
    start)
        echo "start ${*:2}" >> "$MOCK_LOG"
        if [[ -n "${MOCK_START_FAIL:-}" ]]; then exit 1; fi
        ;;
    rm)
        echo "rm ${*:2}" >> "$MOCK_LOG"
        [[ -n "${MOCK_RM_FAIL:-}" ]] && exit 1
        echo absent > "$MOCK_STATE"
        ;;
    create)
        shift
        echo "create $*" >> "$MOCK_LOG"
        echo false > "$MOCK_STATE"
        ;;
    *)
        echo "$*" >> "$MOCK_LOG"
        ;;
esac
EOF
chmod +x "$WORK/bin/docker"

# Usage: run_start <initial-state|-> <dev>; sets RC and LOG.
run_start() {
    LOG="$WORK/log"
    : > "$LOG"
    [[ "$1" == "-" ]] || echo "$1" > "$WORK/state"
    set +e
    PATH="$WORK/bin:$PATH" MOCK_STATE="$WORK/state" MOCK_LOG="$LOG" MOCK_RM_FAIL="${MOCK_RM_FAIL:-}" \
        MOCK_START_FAIL="${MOCK_START_FAIL:-}" PLATFORM=x86_64-ut NUM_ASIC='' \
        bash -c '
            source "$1" start "$2"
            preStartAction() { echo preStartAction >> "$MOCK_LOG"; }
            postStartAction() { echo postStartAction >> "$MOCK_LOG"; }
            start
        ' _ "$WORK/database.sh" "$2" > "$WORK/out" 2>&1
    RC=$?
    set -e
}

ops() {
    awk '{print $1}' "$LOG" | tr '\n' ' ' | sed 's/ $//'
}

create_args() {
    grep '^create ' "$LOG" | sed 's/^create //'
}

# Absent container: created once with unprivileged options.
run_start absent ""
[[ $RC -eq 0 ]] || fail "absent container: start exited $RC: $(cat "$WORK/out")"
[[ "$(ops)" == "ps create preStartAction start postStartAction" ]] || fail "absent container: unexpected lifecycle '$(ops)'"
grep -q '^ps name=^/database\$$' "$LOG" || fail 'absent container: existence query is not an exact name match'
FRESH_ARGS=$(create_args)
pass 'absent database container is created'

# Existing unprivileged container: reused, never removed or recreated.
run_start false ""
[[ $RC -eq 0 ]] || fail "unprivileged container: start exited $RC"
[[ "$(ops)" == "ps inspect preStartAction start postStartAction" ]] || fail "unprivileged container: unexpected lifecycle '$(ops)'"
grep -q '^inspect 0123456789abcdef$' "$LOG" || fail 'unprivileged container: inspect did not use the queried container ID'
pass 'existing unprivileged database container is reused'

# Legacy privileged container: removed and recreated through the normal create path.
run_start true ""
[[ $RC -eq 0 ]] || fail "legacy privileged container: start exited $RC"
[[ "$(ops)" == "ps inspect rm create preStartAction start postStartAction" ]] || fail "legacy privileged container: unexpected lifecycle '$(ops)'"
grep -q '^rm -f database$' "$LOG" || fail 'legacy privileged container: wrong container removed'
[[ "$(create_args)" == "$FRESH_ARGS" ]] || fail 'legacy privileged container: recreated with options different from a fresh create'
pass 'legacy privileged database container is recreated with the same options as a fresh create'

# Repeated start after recreation is idempotent.
run_start - ""
[[ "$(ops)" == "ps inspect preStartAction start postStartAction" ]] || fail "restart after recreation: unexpected lifecycle '$(ops)'"
pass 'restart after recreation reuses the unprivileged container'

# Docker query/inspection failures must fail closed, without touching the container.
run_start error ""
[[ $RC -ne 0 && "$(ops)" == "ps" ]] || fail "docker ps error: unexpected result rc=$RC '$(ops)'"
grep -q 'Failed to query database container' "$WORK/out" || fail 'docker ps error: missing diagnostic'
run_start inspect-error ""
[[ $RC -ne 0 && "$(ops)" == "ps inspect" ]] || fail "inspect error: unexpected result rc=$RC '$(ops)'"
grep -q 'Failed to inspect database container' "$WORK/out" || fail 'inspect error: missing diagnostic'
pass 'docker query and inspect errors fail closed and are not treated as absent or Privileged=false'

run_start garbage ""
[[ $RC -ne 0 && "$(ops)" == "ps inspect" ]] || fail 'unexpected inspect output must fail closed'
pass 'unexpected inspect output fails closed'

MOCK_RM_FAIL=1 run_start true ""
[[ $RC -ne 0 && "$(ops)" == "ps inspect rm" ]] || fail "failed removal must not create a container: '$(ops)'"
pass 'failed removal of a legacy container stops startup'

# docker start failures must not be masked by postStartAction.
MOCK_START_FAIL=1 run_start false ""
[[ $RC -ne 0 && "$(ops)" == "ps inspect preStartAction start" ]] || fail "reuse start failure: rc=$RC '$(ops)'"
MOCK_START_FAIL=1 run_start absent ""
[[ $RC -ne 0 && "$(ops)" == "ps create preStartAction start" ]] || fail "create start failure: rc=$RC '$(ops)'"
pass 'docker start failures stop startup before postStartAction for reused and new containers'

# Rendered docker create options across database instance types.
check_create() {
    local dev=$1 name=$2 net=$3 args
    run_start absent "$dev"
    [[ $RC -eq 0 ]] || fail "DEV='$dev': start exited $RC: $(cat "$WORK/out")"
    grep -q "^ps name=^/$name\\\$\$" "$LOG" || fail "DEV='$dev': queried wrong container"
    args=" $(create_args) "
    [[ "$args" == *" --name=$name "* ]] || fail "DEV='$dev': wrong container name"
    [[ "$args" == *" --net=$net "* ]] || fail "DEV='$dev': expected --net=$net"
    [[ "$args" != *"--privileged"* ]] || fail "DEV='$dev': --privileged rendered"
    [[ "$args" != *"--device"* ]] || fail "DEV='$dev': --device rendered"
    [[ "$args" != *":/dev"* && "$args" != *" /dev/"* ]] || fail "DEV='$dev': host /dev path exposed"
    [[ "$args" != *"--cap-add"* ]] || fail "DEV='$dev': unexpected capability added"
    [[ "$args" != *"systempaths"* ]] || fail "DEV='$dev': masked /proc paths exposed"
    for opt in "-v /etc/sonic:/etc/sonic:ro" "-v /etc/localtime:/etc/localtime:ro"; do
        [[ "$args" == *" $opt "* ]] || fail "DEV='$dev': missing $opt"
    done
}

check_create ""          database         host
[[ " $(create_args) " == *" -v /var/run/redis:/var/run/redis:rw "* ]] || fail 'host database: redis socket volume missing'
check_create 0           database0        bridge
[[ " $(create_args) " == *" -v /var/run/redis0:/var/run/redis:rw "* ]] || fail 'asic0 database: redis socket volume missing'
check_create dpu0        databasedpu0     host
[[ " $(create_args) " == *" -v /var/run/redisdpu0:/var/run/redis:rw "* ]] || fail 'dpu database: redis socket volume missing'
check_create chassisdb   database-chassis host
pass 'docker create options are unprivileged with no added capabilities for host, namespace, DPU and chassis databases'

# Namespace sysctls: /proc/sys is read-only in an unprivileged container, so they must be applied from the host.
cat > "$WORK/bin/ip" <<'EOF'
#!/usr/bin/env bash
if [[ "$1 $2" == "netns exec" && "$4" == sysctl ]]; then
    echo "ip-netns-sysctl $3 ${*:5}" >> "$MOCK_LOG"
    cat > "$MOCK_SYSCTL_IN"
    if [[ -n "${MOCK_SYSCTL_FAIL:-}" ]]; then exit 1; fi
    exit 0
fi
[[ "$1 $2" == "netns pids" ]] && { echo "ip-netns-pids $3" >> "$MOCK_LOG"; exit 1; }
exit 0
EOF
printf '#!/usr/bin/env bash\n[[ "$*" == *PING* ]] && echo PONG\nexit 0\n' > "$WORK/bin/sonic-db-cli"
for cmd in sonic-cfggen mkdir ln chgrp chmod sleep; do printf '#!/bin/sh\nexit 0\n' > "$WORK/bin/$cmd"; done
chmod +x "$WORK/bin/"*
SHIPPED="$REPO/files/image_config/sysctl/sysctl-net.conf"

# Usage: run_post <sysctl-source-file>; runs the real namespace postStartAction, sets RC and LOG.
run_post() {
    LOG="$WORK/log"
    : > "$LOG"
    rm -f "$WORK/sysctl-in"
    set +e
    PATH="$WORK/bin:$PATH" MOCK_STATE="$WORK/state" MOCK_LOG="$LOG" MOCK_SYSCTL_SRC="$1" \
        MOCK_SYSCTL_IN="$WORK/sysctl-in" MOCK_CAT_FAIL="${MOCK_CAT_FAIL:-}" MOCK_SYSCTL_FAIL="${MOCK_SYSCTL_FAIL:-}" \
        PLATFORM=x86_64-ut NUM_ASIC='' \
        bash -c 'source "$1" start 0; postStartAction' _ "$WORK/database.sh" > "$WORK/out" 2>&1
    RC=$?
    set -e
}

run_post "$SHIPPED"
[[ $RC -eq 0 ]] || fail "namespace postStartAction exited $RC: $(cat "$WORK/out")"
grep -q '^ip-netns-sysctl asic0 -e -p -$' "$LOG" || fail 'namespace sysctls not applied from the host into asic0 with -e'
grep -q '^exec .*sysctl --system' "$LOG" && fail 'sysctl still executed inside the unprivileged container'
[[ $(grep -n '^ip-netns-pids' "$LOG" | cut -d: -f1) -lt $(grep -n '^ip-netns-sysctl' "$LOG" | cut -d: -f1) ]] || \
    fail 'namespace must be linked before sysctls are applied'
diff <(tr -d '"' < "$SHIPPED") "$WORK/sysctl-in" > /dev/null || \
    fail 'host-side sysctl input is not exactly the shipped settings with value quotes removed'
grep -qx 'net.ipv4.ip_local_port_range=32768 50001' "$WORK/sysctl-in" || fail 'quoted multi-value setting not normalized'
pass 'namespace sysctls are validated and applied from the host into the namespace'

MOCK_CAT_FAIL=1 run_post "$SHIPPED"
[[ $RC -ne 0 ]] || fail 'sysctl read failure must stop startup'
grep -q '^ip-netns-sysctl' "$LOG" && fail 'sysctl applied after a read failure'
grep -q 'Failed to read' "$WORK/out" || fail 'sysctl read failure: missing diagnostic'
pass 'failure to read the container sysctl file stops startup'

for bad in 'kernel.core_pattern=|/tmp/x' 'net/../kernel/core_pattern=1' 'net.ipv4.conf.all.forwarding=1;id' \
           'net.ipv4.conf.all.forwarding=$(id)' '-net.ipv4.conf.all.forwarding=1' 'net.ipv4.conf.all.forwarding = 1'; do
    { cat "$SHIPPED"; printf '%s\n' "$bad"; } > "$WORK/sysctl-bad"
    run_post "$WORK/sysctl-bad"
    [[ $RC -ne 0 ]] || fail "invalid sysctl line accepted: $bad"
    [[ ! -e "$WORK/sysctl-in" ]] || fail "sysctl partially applied despite invalid line: $bad"
    grep -q 'Rejected invalid' "$WORK/out" || fail "invalid sysctl line: missing diagnostic: $bad"
done
printf '# only a comment\n' > "$WORK/sysctl-empty"
run_post "$WORK/sysctl-empty"
[[ $RC -ne 0 && ! -e "$WORK/sysctl-in" ]] || fail 'sysctl file without net.* settings must be rejected'
pass 'any invalid line rejects the whole sysctl input before anything is applied'

MOCK_SYSCTL_FAIL=1 run_post "$SHIPPED"
[[ $RC -ne 0 ]] || fail 'sysctl application failure must stop startup'
grep -q 'Failed to apply' "$WORK/out" || fail 'sysctl application failure: missing diagnostic'
grep -q '^exec .*pgrep' "$LOG" && fail 'startup continued after sysctl application failure'
pass 'sysctl application failure stops startup'

# Optional: feed the normalized input to a real sysctl (e.g. bookworm procps) in an unprivileged user+net namespace.
if [[ -n "${MIGSOFTWAR_47503_SYSCTL:-}" ]]; then
    run_post "$SHIPPED"
    { cat "$WORK/sysctl-in"; echo 'net.ipv4.no_such_key=1'; } | \
        unshare -Urn bash -c "$MIGSOFTWAR_47503_SYSCTL -e -p - > /dev/null" || \
        fail 'real sysctl rejected the normalized shipped settings or a missing key'
    pass 'real sysctl applies the normalized shipped settings and ignores missing keys'
fi

# Namespace consumers: Docker pins --net=container:databaseN to the database ID at create time.
mkdir -p "$WORK/nsbin"
cat > "$WORK/nsbin/docker" <<'EOF'
#!/usr/bin/env bash
S=$MOCK_NS
case "$1" in
    inspect)
        echo "inspect ${*: -1}" >> "$MOCK_LOG"
        [[ "$(cat "$S/consumer")" == present ]] || exit 1
        if [[ "$*" == *NetworkMode* ]]; then
            if [[ -n "${MOCK_NETMODE_FAIL:-}" ]]; then exit 1; fi
            cat "$S/netmode"
        else
            echo '[{}]'
        fi
        ;;
    ps)
        echo "ps ${*: -1}" >> "$MOCK_LOG"
        if [[ -n "${MOCK_PS_FAIL:-}" ]]; then exit 1; fi
        id=${*: -1}; id=${id#id=}
        grep -qx "$id" "$S/db_ids" && echo "$id"
        exit 0
        ;;
    rm)
        echo "rm ${*:2}" >> "$MOCK_LOG"
        echo absent > "$S/consumer"
        ;;
    create)
        echo "create ${*:2}" >> "$MOCK_LOG"
        echo present > "$S/consumer"
        echo "container:$(tail -1 "$S/db_ids")" > "$S/netmode"
        ;;
    *)
        echo "$*" >> "$MOCK_LOG"
        ;;
esac
EOF
chmod +x "$WORK/nsbin/docker"

render swss broadcom '-t' > "$WORK/swss.rendered.sh"
render pmon broadcom '-t' > "$WORK/pmon.rendered.sh"
sed '/^case "\$1" in/,$d' "$WORK/swss.rendered.sh" > "$WORK/swss.sh"
sed '/^case "\$1" in/,$d' "$WORK/pmon.rendered.sh" > "$WORK/pmon.sh"

# Usage: run_consumer <script> <dev> <consumer-state> <netmode> <live-db-ids...>; sets RC and LOG.
run_consumer() {
    local script=$1 dev=$2
    LOG="$WORK/nslog"
    : > "$LOG"
    mkdir -p "$WORK/ns"
    echo "$3" > "$WORK/ns/consumer"
    echo "$4" > "$WORK/ns/netmode"
    shift 4
    printf '%s\n' "$@" > "$WORK/ns/db_ids"
    run_consumer_again "$script" "$dev"
}

run_consumer_again() {
    : > "$LOG"
    set +e
    PATH="$WORK/nsbin:$PATH" MOCK_NS="$WORK/ns" MOCK_LOG="$LOG" MOCK_PS_FAIL="${MOCK_PS_FAIL:-}" \
        MOCK_NETMODE_FAIL="${MOCK_NETMODE_FAIL:-}" PLATFORM=x86_64-ut HWSKU=ut-sku NUM_ASIC='' \
        bash -c '
            source "$1" start "$2"
            getMountPoint() { echo "$MOUNTPATH"; }
            preStartAction() { echo preStartAction >> "$MOCK_LOG"; }
            postStartAction() { echo postStartAction >> "$MOCK_LOG"; }
            container() { echo "container $*" >> "$MOCK_LOG"; }
            start
        ' _ "$1" "$2" > "$WORK/nsout" 2>&1
    RC=$?
    set -e
}

consumer_ops() {
    awk '{print $1}' "$LOG" | tr '\n' ' ' | sed 's/ $//'
}

grep -Fq '/usr/local/bin/container start ${DOCKERNAME}' "$WORK/swss.sh" || fail 'non-database launcher no longer uses /usr/local/bin/container'
sed -i 's#/usr/local/bin/container start#container start#' "$WORK/swss.sh" "$WORK/pmon.sh"

# Existing unprivileged database and consumer pinned to the current database ID: reused unchanged.
run_consumer "$WORK/swss.sh" 0 present container:dbcurrent dbcurrent
[[ $RC -eq 0 && "$(consumer_ops)" == "inspect inspect ps preStartAction container postStartAction" ]] || \
    fail "consumer of current database: rc=$RC '$(consumer_ops)'"
grep -q '^ps id=dbcurrent$' "$LOG" || fail 'consumer provider lookup is not by the pinned ID'
pass 'namespace consumer pinned to the current database ID is reused'

# Legacy privileged database was recreated: consumer pinned to the removed ID is recreated against the new one.
run_consumer "$WORK/swss.sh" 0 present container:dbold dbnew
[[ $RC -eq 0 && "$(consumer_ops)" == "inspect inspect ps rm create preStartAction container postStartAction" ]] || \
    fail "consumer of removed database: rc=$RC '$(consumer_ops)'"
grep -q '^rm -f swss0$' "$LOG" || fail 'stale consumer: wrong container removed'
grep -q -- '--net=container:database0 ' "$LOG" || fail 'stale consumer not recreated in the database0 namespace'
grep -q 'attached to removed network namespace dbold' "$WORK/nsout" || fail 'stale consumer: missing diagnostic'
pass 'namespace consumer pinned to a removed database ID is recreated with --net=container:database0'

# Subsequent start is idempotent.
run_consumer_again "$WORK/swss.sh" 0
[[ $RC -eq 0 && "$(consumer_ops)" == "inspect inspect ps preStartAction container postStartAction" ]] || \
    fail "consumer restart after recreation: rc=$RC '$(consumer_ops)'"
pass 'subsequent consumer start reuses the recreated container'

# Absent consumer is created normally.
run_consumer "$WORK/swss.sh" 0 absent "" dbcurrent
[[ $RC -eq 0 && "$(consumer_ops)" == "inspect create preStartAction container postStartAction" ]] || \
    fail "absent consumer: rc=$RC '$(consumer_ops)'"
pass 'absent namespace consumer is created'

# Docker query failures must not be treated as a live provider or as a missing one.
MOCK_PS_FAIL=1 run_consumer "$WORK/swss.sh" 0 present container:dbcurrent dbcurrent
[[ $RC -ne 0 && "$(consumer_ops)" == "inspect inspect ps" ]] || fail "provider query failure: rc=$RC '$(consumer_ops)'"
MOCK_NETMODE_FAIL=1 run_consumer "$WORK/swss.sh" 0 present container:dbcurrent dbcurrent
[[ $RC -ne 0 && "$(consumer_ops)" == "inspect inspect" ]] || fail "network-mode inspect failure: rc=$RC '$(consumer_ops)'"
pass 'docker query failures stop consumer startup without removing the container'

# Host-network containers keep the existing HWSKU-only reuse.
run_consumer "$WORK/swss.sh" "" present host dbcurrent
[[ $RC -eq 0 && "$(consumer_ops)" == "inspect inspect preStartAction container postStartAction" ]] || \
    fail "host-network swss: rc=$RC '$(consumer_ops)'"
run_consumer "$WORK/pmon.sh" "" present host dbcurrent
[[ $RC -eq 0 && "$(consumer_ops)" == "inspect inspect preStartAction container postStartAction" ]] || \
    fail "host-network pmon: rc=$RC '$(consumer_ops)'"
pass 'host-network containers keep HWSKU-based reuse with no provider lookup'

# Optional: reproduce the Docker behavior that requires this.
if [[ -n "${MIGSOFTWAR_47503_LIVE_IMAGE:-}" ]]; then
    P=ut-47503-ns-$$
    docker create --name "$P-db" --entrypoint sleep "$MIGSOFTWAR_47503_LIVE_IMAGE" 60 > /dev/null
    docker create --name "$P-c" --net="container:$P-db" --entrypoint sleep "$MIGSOFTWAR_47503_LIVE_IMAGE" 60 > /dev/null
    old=$(docker inspect -f '{{.Id}}' "$P-db")
    mode=$(docker inspect -f '{{.HostConfig.NetworkMode}}' "$P-c")
    docker rm -f "$P-db" > /dev/null
    docker create --name "$P-db" --entrypoint sleep "$MIGSOFTWAR_47503_LIVE_IMAGE" 60 > /dev/null
    live=$(docker ps -a -q --no-trunc --filter "id=${mode#container:}")
    docker rm -f "$P-db" "$P-c" > /dev/null
    [[ "$mode" == "container:$old" ]] || fail "live Docker: NetworkMode '$mode' is not pinned to the provider ID"
    [[ -z "$live" ]] || fail 'live Docker: removed provider ID still resolves'
    pass 'Docker pins container:<name> to the provider ID, and the ID is gone after recreation'
fi

# Optional: validate the rendered options against a real Docker daemon (no container is started).
if [[ -n "${MIGSOFTWAR_47503_LIVE_IMAGE:-}" ]]; then
    for dev in "" 0 dpu0; do
        run_start absent "$dev"
        mapfile -t live_args < <(create_args | tr ' ' '\n' | sed '/^$/d;$d' | sed "s/^--name=.*/--name=ut-47503-$$-${dev:-host}/")
        cid=$(docker create "${live_args[@]}" "$MIGSOFTWAR_47503_LIVE_IMAGE" true)
        hc=$(docker inspect -f '{{.HostConfig.Privileged}}|{{len .HostConfig.Devices}}|{{.HostConfig.CapAdd}}' "$cid")
        docker rm "$cid" > /dev/null
        [[ "$hc" == "false|0|[]" ]] || fail "live Docker DEV='$dev': HostConfig is '$hc', expected 'false|0|[]'"
    done
    pass 'Docker reports Privileged=false, no devices and no added capabilities'
fi
