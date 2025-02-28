#!/bin/bash

sonic-cfggen -d -t /usr/share/sonic/templates/chrony.conf.j2 >/etc/chrony/chrony.conf
sonic-cfggen -d -t /usr/share/sonic/templates/chrony.keys.j2 >/etc/chrony/chrony.keys
chmod o-r /etc/chrony/chrony.keys
