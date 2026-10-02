#!/bin/bash
# Create the front-panel networks a cSONiC container needs, then start it on them.
#
# create_vnet.sh next door wires veth pairs into netns "servers". This does the
# docker-network equivalent: one bridge per front-panel link, which is what you
# want when the far end is another container rather than a bare namespace. Name
# two containers and they share the bridges, giving back-to-back links.
#
# start.sh inside the image filters lanemap.ini/port_config.ini down to the
# eth<N> interfaces that exist, and sai_vs binds Ethernet<N> to eth<N> by lane,
# so the interfaces have to be attached, under the right names, before the
# container first starts. Hence create + connect + start rather than `docker run`.
#
# Two bridge settings are not defaults and have no useful error when missing:
#
#   MTU 9154 - a 9100-byte frame grows by the 32-byte MACsec header. vslib
#   raises the container-side eth<N> itself but not the host veth or the bridge,
#   so a lower MTU silently drops every full-size frame once MACsec is on.
#
#   group_fwd_mask 0x4008 - bit 14 forwards LLDP (01:80:c2:00:00:0e) and bit 3
#   forwards EAPOL (01:80:c2:00:00:03), which carries MKPDUs. Without bit 3 each
#   side creates its egress SC and then waits forever for a peer it never hears.
#
# Needs root: group_fwd_mask is written through sysfs.
set -euo pipefail

usage() {
    cat >&2 <<USAGE
Usage: $0 [-n <ports>] [-i <image>] [-p <prefix>] [-b <subnet-base>] [-d] <name> [<name2>]

  -n <ports>        front-panel ports per node (default 4)
  -i <image>        image to start (default docker-sonic-vs:latest)
  -p <prefix>       network name prefix (default csonic-l; the
                    subnet base and link index are appended)
  -b <subnet-base>  third octet of 172.30.<base+N>.0/24 (default 40)
  -c <dir>          root for the per-node config volume (default /var/lib/csonic)
  -C                start with no config at all, leaving every port admin down
  -d                tear down the named containers and the networks instead

  Naming two nodes puts both on the same bridges, so Ethernet<N> on one faces
  Ethernet<N> on the other.
USAGE
    exit 1
}

PORTS=4
IMAGE=docker-sonic-vs:latest
PREFIX=csonic-l
SUBNET_BASE=40
CONFIG_ROOT=/var/lib/csonic
CONFIG_ROOT_IS_DEFAULT=true
WITH_CONFIG=true
TEARDOWN=false

while getopts ":n:i:p:b:c:Cd" opt; do
    case $opt in
        n) PORTS=$((OPTARG)) ;;
        i) IMAGE=$OPTARG ;;
        p) PREFIX=$OPTARG ;;
        b) SUBNET_BASE=$((OPTARG)) ;;
        c) CONFIG_ROOT=$OPTARG; CONFIG_ROOT_IS_DEFAULT=false ;;
        C) WITH_CONFIG=false ;;
        d) TEARDOWN=true ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

[ $# -ge 1 ] && [ $# -le 2 ] || usage
NODES=("$@")

# An empty name would make the teardown's rm -rf "$CONFIG_ROOT/$node" expand to
# the config root itself, taking every other node's saved config with it. The
# :? on CONFIG_ROOT does not cover that.
for node in "${NODES[@]}"; do
    [ -n "$node" ] || usage
done

# Each lab takes 172.30.<base+1>..<base+PORTS>, so an out-of-range pair yields a
# subnet like 172.30.256.0/24. Nothing holds that, base_is_free says yes, and
# docker network create then fails on a later link having already made the
# earlier ones. Check the supplied values against the same bound the suggestion
# loop below uses, and fail before anything is created.
[ "$PORTS" -ge 1 ] || { echo "$0: -n must be at least 1" >&2; exit 1; }
[ "$SUBNET_BASE" -ge 0 ] && [ $((SUBNET_BASE + PORTS)) -le 255 ] || {
    echo "$0: -b $SUBNET_BASE with -n $PORTS runs past 172.30.255.0/24" >&2
    echo "  the last subnet would be 172.30.$((SUBNET_BASE + PORTS)).0/24" >&2
    exit 1
}

if [ "$(id -u)" != 0 ]; then
    echo "$0: must run as root (group_fwd_mask is written through sysfs)" >&2
    exit 1
fi

# The subnet is what makes a lab unique, so the network name carries the base.
# Without it a second lab started with -b <other> finds $PREFIX$i already there,
# reuses a network on the first lab's subnet, and docker then rejects the
# container address with "no configured subnet contains IP address ...".
net_name() { printf '%s%s-%s' "$PREFIX" "$SUBNET_BASE" "$1"; }

if $TEARDOWN; then
    for node in "${NODES[@]}"; do
        docker rm -f "$node" >/dev/null 2>&1 && echo "  removed $node"
    done
    for i in $(seq 1 "$PORTS"); do
        for n in "$(net_name "$i")" "$PREFIX$i"; do
            docker network rm "$n" >/dev/null 2>&1 && echo "  removed network $n"
        done
    done
    if $CONFIG_ROOT_IS_DEFAULT; then
        for node in "${NODES[@]}"; do
            rm -rf "${CONFIG_ROOT:?}/$node"
        done
    fi
    exit 0
fi

# -p moves the network names but not the addresses, so a second lab started
# alongside a first collides on the subnet, and docker reports only "Pool
# overlaps with other one on this address space" without naming either side.
# Say which subnet, which network holds it, and which -b would clear it.
subnet_of() {
    docker network inspect "$1" --format '{{range .IPAM.Config}}{{.Subnet}} {{end}}' 2>/dev/null
}

used=""
for net in $(docker network ls --format '{{.Name}}'); do
    used="$used $(subnet_of "$net" | sed "s|\\(\\S\\+\\)|\\1=$net|g")"
done

base_is_free() {
    for i in $(seq 1 "$PORTS"); do
        case " $used " in
            *" 172.30.$(($1 + i)).0/24="*) return 1 ;;
        esac
    done
    return 0
}

if ! base_is_free "$SUBNET_BASE"; then
    echo "$0: subnet collision, nothing created:" >&2
    for i in $(seq 1 "$PORTS"); do
        want="172.30.$((SUBNET_BASE + i)).0/24"
        for entry in $used; do
            case "$entry" in
                "$want="*) echo "  $want is already held by network ${entry#*=}" >&2 ;;
            esac
        done
    done
    echo "Each lab uses 172.30.<base+1>..<base+$PORTS>. Free bases:" >&2
    shown=0
    for base in $(seq 10 10 240); do
        if [ $((base + PORTS)) -le 255 ] && base_is_free "$base"; then
            echo "  -b $base" >&2
            shown=$((shown + 1))
            if [ "$shown" -ge 3 ]; then
                break
            fi
        fi
    done
    exit 1
fi

for i in $(seq 1 "$PORTS"); do
    net="$(net_name "$i")"
    want="172.30.$((SUBNET_BASE + i)).0/24"
    if docker network inspect "$net" >/dev/null 2>&1; then
        have=$(docker network inspect "$net" \
                 --format '{{range .IPAM.Config}}{{.Subnet}}{{end}}' 2>/dev/null)
        if [ "$have" != "$want" ]; then
            echo "$0: network $net holds $have, expected $want" >&2
            echo "  remove it, or choose another name with -p" >&2
            exit 1
        fi
        echo "  network $net already exists, reusing it"
        continue
    fi
    docker network create \
        --subnet "$want" \
        --opt com.docker.network.driver.mtu=9154 \
        "$net" >/dev/null
    echo "  network $net  172.30.$((SUBNET_BASE + i)).0/24  mtu 9154"
done

# start.sh reads /var/sonic/config_db.json if one is mounted there, and that
# branch REPLACES its own port generation -- init_cfg.json carries only
# DEVICE_METADATA and FEATURE -- so a supplied config has to bring the whole PORT
# table with it. Read the rows out of the image's own lanemap.ini and
# port_config.ini rather than hardcoding them, so this tracks whatever HWSKU the
# image was built with. Without a config every port comes up admin down:
# start.sh forces that ("Test cases dependent") on the config it generates.
write_config() {
    local node=$1 dir=$2 lanemap=$3 portcfg=$4

    mkdir -p "$dir"
    {
        printf '{\n'
        printf '    "DEVICE_METADATA": {\n        "localhost": {\n            "hostname": "%s"\n        }\n    },\n' "$node"
        printf '    "PORT": {'
        local sep="" i lanes row
        for i in $(seq 1 "$PORTS"); do
            lanes=$(awk -F: -v d="eth$i" '$1 == d { print $2 }' "$lanemap")
            [ -n "$lanes" ] || continue
            # Column order is declared by the file's own "#" header and is not
            # the same for every HWSKU -- DPU-2P, for one, has no speed column --
            # so resolve the positions by name the way sonic-cfggen does. Reading
            # them positionally put an empty speed in the config for any image
            # whose layout differs, and PortsOrch then rejects the port.
            row=$(awk -v l="$lanes" '
                /^[[:space:]]*#/ {
                    if (!hdr) {
                        head = $0
                        sub(/^[[:space:]]*#/, "", head)
                        n = split(head, f, /[[:space:]]+/)
                        k = 0
                        for (i = 1; i <= n; i++) if (f[i] != "") col[f[i]] = ++k
                        hdr = 1
                    }
                    next
                }
                hdr && col["lanes"] && $(col["lanes"]) == l {
                    # "|" and not a tab: tab is an IFS whitespace character, so
                    # the read below would collapse a run of them into one
                    # separator and shift every field after an absent column
                    # left. None of these four values can contain a "|".
                    printf "%s|%s|%s|%s\n", $(col["name"]),
                        col["alias"] ? $(col["alias"]) : "",
                        col["index"] ? $(col["index"]) : "",
                        col["speed"] ? $(col["speed"]) : ""
                    exit
                }' "$portcfg")
            [ -n "$row" ] || continue
            IFS='|' read -r p_name p_alias p_index p_speed <<<"$row"
            printf '%s\n        "%s": {\n' "$sep" "$p_name"
            printf '            "lanes": "%s",\n' "$lanes"
            # Any of these the HWSKU does not declare is omitted rather than
            # written empty, which is what sonic-cfggen's own port_config reader
            # does with a column that is not there.
            [ -n "$p_alias" ] && printf '            "alias": "%s",\n' "$p_alias"
            [ -n "$p_index" ] && printf '            "index": "%s",\n' "$p_index"
            [ -n "$p_speed" ] && printf '            "speed": "%s",\n' "$p_speed"
            printf '            "mtu": "9100",\n'
            printf '            "admin_status": "up"\n'
            printf '        }'
            sep=","
        done
        printf '\n    }\n}\n'
    } > "$dir/config_db.json"
}

config_mount=()
if $WITH_CONFIG; then
    platform=$(docker image inspect "$IMAGE" --format '{{range .Config.Env}}{{println .}}{{end}}' | sed -n 's/^PLATFORM=//p')
    hwsku=$(docker image inspect "$IMAGE" --format '{{range .Config.Env}}{{println .}}{{end}}' | sed -n 's/^HWSKU=//p')
    skudir=/usr/share/sonic/device/$platform/$hwsku
    tmp=$(mktemp -d)
    # set -e can leave by way of the docker run/create calls below; without this
    # the extracted port rows stay behind on every failed run.
    trap 'rm -rf "${tmp:-}"' EXIT
    docker run --rm --entrypoint cat "$IMAGE" "$skudir/lanemap.ini" > "$tmp/lanemap.ini"
    docker run --rm --entrypoint cat "$IMAGE" "$skudir/port_config.ini" > "$tmp/port_config.ini"
    echo "  port rows from $hwsku"
fi

for node in "${NODES[@]}"; do
    if $WITH_CONFIG; then
        write_config "$node" "$CONFIG_ROOT/$node" "$tmp/lanemap.ini" "$tmp/port_config.ini"
        config_mount=(-v "$CONFIG_ROOT/$node:/var/sonic")
        echo "  config for $node in $CONFIG_ROOT/$node/config_db.json"
    fi
    docker create --name "$node" --hostname "$node" --privileged \
        "${config_mount[@]}" "$IMAGE" >/dev/null
    echo "  created $node from $IMAGE"
done

# --driver-opt endpoint.ifname is only honoured by `network connect`, which is
# why the containers are created stopped and connected one link at a time.
host_octet=1
for node in "${NODES[@]}"; do
    host_octet=$((host_octet + 1))
    for i in $(seq 1 "$PORTS"); do
        docker network connect \
            --ip "172.30.$((SUBNET_BASE + i)).$host_octet" \
            --driver-opt com.docker.network.endpoint.ifname="eth$i" \
            "$(net_name "$i")" "$node"
    done
    echo "  attached eth1..eth$PORTS on $node"
done

# docker only materialises the bridge netdev once the network has an endpoint,
# so the mask has to be set after the connects.
for i in $(seq 1 "$PORTS"); do
    br=br-$(docker network inspect "$(net_name "$i")" --format '{{.Id}}' | cut -c1-12)
    echo 0x4008 > "/sys/class/net/$br/bridge/group_fwd_mask"
    echo "  $br group_fwd_mask -> $(cat "/sys/class/net/$br/bridge/group_fwd_mask") (LLDP + EAPOL)"
done

for node in "${NODES[@]}"; do
    docker start "$node" >/dev/null
    echo "  started $node"
done
