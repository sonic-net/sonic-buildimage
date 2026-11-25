#!/bin/bash

sonic-cfggen -d --redis-unix-socket -t /usr/share/sonic/templates/copp_cfg.j2 > /etc/sonic/copp_cfg.json
