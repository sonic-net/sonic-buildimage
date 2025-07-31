#!/usr/bin/env bash
rm -f /var/run/rsyslogd.pid
rm -f /var/tmp/arpd*

supervisorctl start rsyslogd