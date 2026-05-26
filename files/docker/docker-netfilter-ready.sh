#!/bin/sh

# Docker may be socket-activated very early during first boot. On some systems,
# the IPv6 iptables/nf_tables path may not be ready yet, which can cause dockerd
# initialization to fail and cascade dependency failures to database/config
# services. Wait until the ip6tables paths used by dockerd are usable.

for i in $(seq 1 30); do
    if ip6tables -w 5 -t filter -L >/dev/null 2>&1 && \
       ip6tables -w 5 -t nat -L >/dev/null 2>&1; then
        logger "docker-netfilter-ready: ip6tables ready after ${i} attempt(s)"
        exit 0
    fi
    sleep 1
done

logger "docker-netfilter-ready: ip6tables not ready after 30 attempts"
echo "Docker netfilter/ip6tables path is not ready" >&2
exit 1
