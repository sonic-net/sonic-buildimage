#!/bin/bash

#remove syslog

sudo find /var/log/ -mtime +7 -name "syslog.*" | sudo xargs rm -rf

#echo "remove_syslog.sh crontab is running" >> /home/admin/shell.txt

exit 0
