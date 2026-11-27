#!/usr/bin/env bash

DB_READY_TIMEOUT_SEC=${DB_READY_TIMEOUT_SEC:-120}
DB_READY_POLL_INTERVAL_SEC=${DB_READY_POLL_INTERVAL_SEC:-1}

[[ "$DB_READY_TIMEOUT_SEC" =~ ^[0-9]+$ ]] || DB_READY_TIMEOUT_SEC=120
[[ "$DB_READY_POLL_INTERVAL_SEC" =~ ^[0-9]+$ ]] || DB_READY_POLL_INTERVAL_SEC=1
if (( DB_READY_POLL_INTERVAL_SEC <= 0 )); then
    DB_READY_POLL_INTERVAL_SEC=1
fi

elapsed=0

# Wait until redis starts
while [[ $(sonic-db-cli PING | grep -c PONG) -le 0 ]]; do
    if (( elapsed >= DB_READY_TIMEOUT_SEC )); then
        echo "Timed out waiting for redis PING after ${elapsed}s (timeout=${DB_READY_TIMEOUT_SEC}s)" >&2
        exit 1
    fi
    sleep "$DB_READY_POLL_INTERVAL_SEC"
    elapsed=$((elapsed + DB_READY_POLL_INTERVAL_SEC))
done

# If there is a config_db.json dump file, load it.
if [ -r /etc/sonic/config_db.json ]; then
    if [ -r /etc/sonic/init_cfg.json ]; then
        sonic-cfggen -j /etc/sonic/init_cfg.json -j /etc/sonic/config_db.json --write-to-db
    else
        sonic-cfggen -j /etc/sonic/config_db.json --write-to-db
    fi
fi

sonic-db-cli CONFIG_DB SET "CONFIG_DB_INITIALIZED" "1"
