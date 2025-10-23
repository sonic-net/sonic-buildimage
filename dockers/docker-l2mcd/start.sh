#!/usr/bin/env bash

rm -f /var/run/rsyslogd.pid
rm -f /var/run/l2mcd/*
rm -f /var/run/l2mcmgrd/*

# supervisorctl start rsyslogd

# supervisorctl start l2mcd

# supervisorctl start l2mcmgrd
