#!/usr/bin/env bash

# Wait until redis starts
until [[ $(sonic-db-cli PING | grep -c PONG) -gt 0 ]]; do
  sleep 1;
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
