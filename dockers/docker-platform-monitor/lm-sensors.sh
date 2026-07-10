#!/usr/bin/env bash
#
# Based off /etc/init.d/lm-sensors
#


# NOTE: lm-sensors v3.3.5 appears to have a bug. If `sensors -s` is called, it
# will first load /etc/sensors.conf, then load all files in /etc/sensors.d/,
# overriding any values that may have already been specified in
# /etc/sensors.conf. However, it appears this overriding is not taking place.
# As a workaround, as long as a platform-specific sensors.conf has been copied
# to /etc/sensors.d/, we will ONLY load that file, otherwise we load the default.
if [ -e /etc/sensors.d/sensors.conf ]; then
    /usr/bin/sensors -s -c /etc/sensors.d/sensors.conf > /dev/null 2>&1
else
    /usr/bin/sensors -s > /dev/null 2>&1
fi

/usr/bin/sensors > /dev/null 2>&1

# Currently, there is no way to run sensord in the foreground, so we
# can't use supervisord. Instead, we just start the service for now.
service sensord start
