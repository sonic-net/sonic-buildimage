#!/usr/bin/env bash
HWSKU_DIR=/usr/share/sonic/hwsku

rm -f /var/run/rsyslogd.pid
supervisorctl start rsyslogd
supervisorctl start saiserver
