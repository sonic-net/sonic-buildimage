#!/usr/bin/env bash

if [ "${RUNTIME_OWNER}" == "" ]; then
    RUNTIME_OWNER="kube"
fi

TZ=$(cat /etc/timezone)
rm -rf /etc/localtime
ln -sf /usr/share/zoneinfo/$TZ /etc/localtime

cp /etc/rsyslog_plugin/00-rsyslog_plugin.conf /etc/rsyslog.d/
cp /etc/rsyslog_plugin/*_regex.json /etc/rsyslog.d/
