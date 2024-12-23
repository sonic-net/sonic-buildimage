#!/bin/bash

VAR_LOG_SIZE=$(df -k /var/log | sed -n 2p | awk '{ print $2 }')

# Adjust NUM_LOGS_TO_ROTATE to reflect number of common log files (auth, cron,
# teamd, telemetry, etc.) to rotate.
NUM_LOGS_TO_ROTATE=8

# Detect common log files (auth, cron, teamd, telemetry, etc.) size. 720M is
# required to hold extra space to rotate all files (16M each) two times.
if [[ "$VAR_LOG_SIZE" < "204800" ]]; then
    LOG_FILE_ROTATE_SIZE_MB=1
elif [[ "$VAR_LOG_SIZE" < "409600" ]]; then
    LOG_FILE_ROTATE_SIZE_MB=2
else
    LOG_FILE_ROTATE_SIZE_MB=16
fi

# Reserve space for btmp, wtmp, dpkg.log, monit.log, etc., as well as logs that
# should be disabled, just in case they get created and rotated
RESERVED_SPACE_KB=4096

# Limit usable space to 90 to define percentage of the partition minus the
# reserved space for other logs
USABLE_SPACE_KB=$(((VAR_LOG_SIZE * 90 / 100) - RESERVED_SPACE_KB))

# Set our threshold so as to maintain enough space to write all logs from empty
# to full. Most likely, some logs will have non-zero size when this is called,
# so this errs on the side of caution, giving us a bit of a cushion if a log
# grows quickly and passes its rotation size. USABLE_SPACE_KB should be at least
# twice as big as THRESHOLD_KB value.
THRESHOLD_KB=$((USABLE_SPACE_KB - \
                (LOG_FILE_ROTATE_SIZE_MB * 1024 * 2 * NUM_LOGS_TO_ROTATE)))

# Append data to template
APPEND_DATA="{\"max_logs_size_kb\":$THRESHOLD_KB,
              \"common_file_size_mb\":\"$LOG_FILE_ROTATE_SIZE_MB\"}"

# Generic log files
sonic-cfggen -d -t /usr/share/sonic/templates/logrotate-common.j2 \
             -a "$APPEND_DATA" > /etc/logrotate.d/rsyslog

# Specific log files
sonic-cfggen -d -t /usr/share/sonic/templates/logrotate-debug.j2 \
             -a "$APPEND_DATA" > /etc/logrotate.d/debug

sonic-cfggen -d -t /usr/share/sonic/templates/logrotate-syslog.j2 \
             -a "$APPEND_DATA" > /etc/logrotate.d/syslog
