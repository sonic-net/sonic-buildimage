#!/bin/bash

sonic-cfggen -d -t /usr/share/sonic/templates/copp_cfg.j2 > /etc/sonic/copp_cfg.json
