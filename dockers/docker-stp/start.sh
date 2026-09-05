#!/usr/bin/env bash

rm -f /var/run/rsyslogd.pid
rm -f /var/run/stpd/*
rm -f /var/run/stpmgrd/*

supervisorctl start rsyslogd

supervisorctl start stpd

supervisorctl start stpmgrd
