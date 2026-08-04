#/bin/bash

# process name/id
DAEMON_NAME=`basename $0`
DAEMON_PID="$$"

DEF_SEVERITY="INFO"

#/*
#* FEATURE:
#*   log_msg
#* PURPOSE:
#*   log message
#* PARAMETERS:
#*   msg                  (IN) message
#* RETURNS:
#*
#*/
function log_msg() {
  local msg=$1

  `logger -t $DAEMON_NAME -p $DEF_SEVERITY $msg`
}
