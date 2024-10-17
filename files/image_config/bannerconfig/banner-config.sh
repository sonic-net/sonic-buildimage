#!/bin/bash -e

STATE=$(sonic-db-cli CONFIG_DB HGET 'BANNER_MESSAGE|global' state)
LOGIN=
MOTD=
LOGOUT=

if [[ $STATE == "enabled" ]]; then
    LOGIN=$(sonic-db-cli CONFIG_DB HGET 'BANNER_MESSAGE|global' login)
    MOTD=$(sonic-db-cli CONFIG_DB HGET 'BANNER_MESSAGE|global' motd)
    LOGOUT=$(sonic-db-cli CONFIG_DB HGET 'BANNER_MESSAGE|global' logout)

    echo -e "$LOGIN" > /etc/issue.net
    echo -e "$LOGIN" > /etc/issue
    echo -e "$MOTD" > /etc/motd
    echo -e "$LOGOUT" > /etc/logout_message
fi
