#!/usr/bin/env bash


# Generate supervisord config file
mkdir -p /etc/supervisor/conf.d/
# Generate kea folder
mkdir -p /etc/kea/
udp_server_ip=$(ip -j -4 addr list lo scope host | jq -r -M '.[0].addr_info[0].local')
hostname=$(hostname)

# Make the script that waits for all interfaces to come up executable
# Clean up stale ready flag from previous run
rm -f /tmp/dhcpservd_ready
chmod +x /etc/kea/lease_update.sh /usr/bin/start.sh /usr/bin/wait_for_dhcpservd.sh
# The docker container should start this script as PID 1, so now that supervisord is
# properly configured, we exec /usr/local/bin/supervisord so that it runs as PID 1 for the
# duration of the container's lifetime
exec /usr/local/bin/supervisord
