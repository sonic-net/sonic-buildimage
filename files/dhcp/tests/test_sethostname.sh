#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)

test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' 0 1 2 15

# Fake docker: every <id>.hosts file under $FAKE_DOCKER_DIR is a running
# host-network container, and "exec" runs the hook's in-container script
# against that file instead of the container's /etc/hosts.
FAKE_DOCKER_DIR=$test_dir/containers
LOGGER_OUT=$test_dir/logger.out
export FAKE_DOCKER_DIR LOGGER_OUT
mkdir -p "$test_dir/bin" "$FAKE_DOCKER_DIR"

cat > "$test_dir/bin/docker" <<'EOF'
#!/bin/sh
case "$1" in
    ps)
        # Only answer the query the hook is expected to make.
        [ "$*" = "ps -q --filter network=host" ] || exit 0
        for f in "$FAKE_DOCKER_DIR"/*.hosts; do
            [ -e "$f" ] && basename "$f" .hosts
        done
        exit 0
        ;;
    exec)
        [ "$2" = -i ] && [ "$4" = sh ] && [ "$5" = -c ] && [ "$7" = sh ] || exit 2
        [ "${10}" = /etc/hosts ] || exit 2
        exec sh -c "$6" sh "$8" "$9" "$FAKE_DOCKER_DIR/$3.hosts"
        ;;
esac
exit 2
EOF
cat > "$test_dir/bin/logger" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >> "$LOGGER_OUT"
EOF
chmod +x "$test_dir/bin/docker" "$test_dir/bin/logger"
PATH=$test_dir/bin:$PATH

fail()
{
    printf 'FAIL: %s\n' "$1" >&2
    exit 1
}

expect_file()
{
    printf '%s\n' "$2" > "$test_dir/expected"
    cmp -s "$test_dir/expected" "$1" || {
        printf 'unexpected contents of %s:\n' "$1" >&2
        diff "$test_dir/expected" "$1" >&2 || true
        exit 1
    }
}

reset()
{
    rm -f "$FAKE_DOCKER_DIR"/*.hosts "$LOGGER_OUT"
}

# Sourcing the hook for a reason it does not handle must only define the helper.
reason=TEST
. "$SCRIPT_DIR/../sethostname"

# IPv4 hook: the host has already been renamed from "sonic" to "switch1".
host_hosts=$test_dir/host.hosts
printf '%s\n' '127.0.0.1       localhost' '127.0.0.1 switch1' > "$host_hosts"

# A container started before the rename: stale entry plus a container-only
# --add-host entry that must survive.
reset
printf '%s\n' '127.0.0.1       localhost' '127.0.0.1 sonic' \
    '10.0.0.1 redis_chassis.server' > "$FAKE_DOCKER_DIR/lldp.hosts"
# A container started after the rename already has the new entry.
printf '%s\n' '127.0.0.1       localhost' '127.0.0.1 switch1' \
    > "$FAKE_DOCKER_DIR/snmp.hosts"

sonic_update_container_hosts sonic switch1 "$host_hosts"

expect_file "$FAKE_DOCKER_DIR/lldp.hosts" '127.0.0.1       localhost
10.0.0.1 redis_chassis.server
127.0.0.1 switch1'
expect_file "$FAKE_DOCKER_DIR/snmp.hosts" '127.0.0.1       localhost
127.0.0.1 switch1'
[ ! -e "$LOGGER_OUT" ] || fail "unexpected log: $(cat "$LOGGER_OUT")"

# Running the update a second time changes nothing.
sonic_update_container_hosts sonic switch1 "$host_hosts"
expect_file "$FAKE_DOCKER_DIR/lldp.hosts" '127.0.0.1       localhost
10.0.0.1 redis_chassis.server
127.0.0.1 switch1'

# A name that is a suffix of another entry must not remove that entry.
reset
printf '%s\n' '127.0.0.1 sonic' '10.0.0.2 peer-sonic.example.com' \
    > "$FAKE_DOCKER_DIR/lldp.hosts"
sonic_update_container_hosts sonic switch1 "$host_hosts"
expect_file "$FAKE_DOCKER_DIR/lldp.hosts" '10.0.0.2 peer-sonic.example.com
127.0.0.1 switch1'

# A container whose hosts file cannot be read is logged and does not stop the
# others from being updated.
reset
printf '%s\n' '127.0.0.1 sonic' > "$FAKE_DOCKER_DIR/lldp.hosts"
mkdir "$FAKE_DOCKER_DIR/broken.hosts"
sonic_update_container_hosts sonic switch1 "$host_hosts" 2>/dev/null
rmdir "$FAKE_DOCKER_DIR/broken.hosts"
expect_file "$FAKE_DOCKER_DIR/lldp.hosts" '127.0.0.1 switch1'
grep -q 'container broken$' "$LOGGER_OUT" || fail "broken container not logged"

# No running containers: nothing to do, nothing logged.
reset
sonic_update_container_hosts sonic switch1 "$host_hosts"
[ ! -e "$LOGGER_OUT" ] || fail "unexpected log with no containers"

# Host names come from the DHCP server; only host name characters are accepted.
assert_valid()
{
    sonic_is_valid_hostname "$1" || fail "expected valid host name: $1"
}
assert_invalid()
{
    if sonic_is_valid_hostname "$1"; then
        fail "expected invalid host name: $1"
    fi
}
# 3 x 63-character labels + 3 dots + 61 = 253 characters, the maximum.
long_label=$(printf '%063d' 0)
name_253="$long_label.$long_label.$long_label.$(printf '%061d' 0)"
[ "${#name_253}" = 253 ] || fail "test name is ${#name_253} characters, not 253"
assert_valid 'switch1'
assert_valid 'switch1.example.com'
assert_valid 'sw_1-a.EXAMPLE.com'
assert_valid "$name_253"
assert_invalid ''
assert_invalid "${name_253}0"
assert_invalid 'a/b'
assert_invalid 'sonic/e touch /tmp/x;#'
assert_invalid '.*'
assert_invalid 'a*'
assert_invalid 'a b'
assert_invalid "a'b"
assert_invalid 'a\b'
assert_invalid 'a$b'
assert_invalid "a
b"

# Metacharacters in either name: no container is touched, no embedded sed
# command runs, and the refusal is logged.
for bad_name in "sonic/e touch $test_dir/injected;#" "sonic/w $test_dir/injected" '.*' 'son*'; do
    reset
    printf '%s\n' '127.0.0.1 localhost' '127.0.0.1 sonic' \
        '10.0.0.1 redis_chassis.server' > "$FAKE_DOCKER_DIR/lldp.hosts"
    cp "$FAKE_DOCKER_DIR/lldp.hosts" "$test_dir/before"
    # The assertions below decide the result, so a regression reports which
    # name broke instead of stopping the test under set -e.
    sonic_update_container_hosts "$bad_name" switch1 "$host_hosts" 2>/dev/null || :
    sonic_update_container_hosts sonic "$bad_name" "$host_hosts" 2>/dev/null || :
    cmp -s "$test_dir/before" "$FAKE_DOCKER_DIR/lldp.hosts" \
        || fail "container hosts changed for name: $bad_name"
    [ ! -e "$test_dir/injected" ] || fail "sed command ran for name: $bad_name"
    [ "$(grep -c 'invalid host name' "$LOGGER_OUT")" = 2 ] \
        || fail "invalid name not logged: $bad_name"
done

# A '.' in a name matches only a literal '.'.
reset
printf '%s\n' '127.0.0.1 a.b' '10.0.0.3 axb' > "$FAKE_DOCKER_DIR/lldp.hosts"
sonic_update_container_hosts a.b switch1 "$host_hosts"
expect_file "$FAKE_DOCKER_DIR/lldp.hosts" '10.0.0.3 axb
127.0.0.1 switch1'

# IPv6 hook: the host carries both an IPv4 and an IPv6 entry for the new FQDN.
. "$SCRIPT_DIR/../sethostname6"
printf '%s\n' '127.0.0.1       localhost' '127.0.0.1 switch1.example.com' \
    ':: switch1.example.com' > "$host_hosts"
reset
printf '%s\n' '127.0.0.1       localhost' '127.0.0.1 sonic' ':: sonic' \
    > "$FAKE_DOCKER_DIR/lldp.hosts"
sonic_update_container_hosts sonic switch1.example.com "$host_hosts"
expect_file "$FAKE_DOCKER_DIR/lldp.hosts" '127.0.0.1       localhost
127.0.0.1 switch1.example.com
:: switch1.example.com'

printf 'PASS\n'
