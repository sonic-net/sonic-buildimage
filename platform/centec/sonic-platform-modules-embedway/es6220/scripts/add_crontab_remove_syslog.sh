#!/bin/bash

#sed -n '7p' /etc/rsyslog.d/99-default.conf
sudo sed -i '7c !user.debug;*.*;cron,auth,authpriv.none     -/var/log/syslog' /etc/rsyslog.d/99-default.conf
sudo sed -i '10c user.debug      /var/log/sdkdebug' /etc/rsyslog.d/99-default.conf
#sed -n '7p' /etc/rsyslog.d/99-default.conf
sudo service rsyslog restart

sudo sed -i '9c */5 *   * * *   root    /usr/bin/remove_syslog.sh' /etc/crontab
#echo "*/5 * * * * root /usr/bin/remove_syslog.sh" >> /etc/crontab

exit 0
