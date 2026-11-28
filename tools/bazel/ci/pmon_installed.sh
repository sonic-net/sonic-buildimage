#!/bin/sh
# Exercise the installed services without switch hardware or a live Redis instance.
set -eu

sensors -v
test -x /usr/bin/docker_init.sh
test -x /usr/bin/SmartCmd
test -x /usr/local/bin/ledd
test -x /usr/local/bin/xcvrd
python3 -B - "$2" <<'PY'
import sys
from importlib.metadata import PackageNotFoundError, version

# The default VS build selects the PDDF API and its pinned JSON validator.
if sys.argv[1] == "y":
    assert version("sonic-platform-pddf-common") == "1.0"
    assert version("jsonschema") == "2.6.0"
elif sys.argv[1] == "n":
    try:
        version("sonic-platform-pddf-common")
    except PackageNotFoundError:
        pass
    else:
        raise AssertionError("PDDF was disabled but its wheel is installed")
else:
    raise ValueError("Expected Make's PDDF setting to be y or n")

for daemon in ("ledd", "pcied", "psud", "syseepromd", "thermalctld", "sensormond",
               "xcvrd", "ycabled", "chassisd", "stormond", "bmcctld"):
    if not version("sonic-" + daemon):
        raise ValueError("Missing installed version for sonic-" + daemon)
PY

sonic-cfggen -a '{"API_VERSION":3,"python2_daemons":[],"HAVE_SENSORS_CONF":1}' \
    -t /usr/share/sonic/templates/docker-pmon.supervisord.conf.j2 > /tmp/pmon-supervisord.conf
grep -Fx '[program:lm-sensors]' /tmp/pmon-supervisord.conf
grep -Fx 'command=python3 /usr/local/bin/xcvrd' /tmp/pmon-supervisord.conf

if [ "$1" = debug ]; then
    ssh -V
fi
