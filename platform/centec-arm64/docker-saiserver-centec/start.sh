#!/usr/bin/env bash

start_centec()
{
}


rm -f /var/run/rsyslogd.pid

supervisorctl start rsyslogd

start_centec

supervisorctl start saiserver
