#!/bin/bash
# Copyright (C) 2017 Ingrasys, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

INTERVAL=3
I2C_UTILS="/usr/sbin/i2c_utils.sh"
QSFP_SI_SCRIPT="/usr/sbin/qsfp_si_cfg.sh"
QSFP_ARRAY=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 \
            0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

#QSFP SI monitor
function _qsfp_si_monitor {
    local i
    local status
    for i in {0..63};
    do
        status=`${I2C_UTILS} i2c_qsfp_status_get $(expr $i + 1) | egrep '^status=.*$' | sed -e 's/status=//g'`
        if [ "${status}" == "1" ]; then
            _qsfp_type_check $i
        fi
    done
}

#QSFP type
function _qsfp_type_check {
    local port=$1
    local qsfp_type=`${I2C_UTILS} i2c_qsfp_type_get $(expr $port + 1)`
    local identifier=`echo "$qsfp_type" | grep '^identifier=.*$' | sed -e 's/identifier=//g'`
    # identifier 11 for qsfp, 3 for sfp
    if [ "${identifier}" == "11" ] || [ "${identifier}" == "3" ]; then
        connector=`echo "$qsfp_type" | grep '^connector=.*$' | sed -e 's/connector=//g'`
        case ${connector} in
            21|23)
                #DAC
                if [ "${QSFP_ARRAY[$port]}" != "${connector}" ]; then
                    echo "Change Port $(expr $port + 1) to DAC"
                    QSFP_ARRAY[$port]=${connector}
                    ${QSFP_SI_SCRIPT} dac $port >/dev/null
                fi
            ;;
            *) 
                #AOC
                if [ "${QSFP_ARRAY[$port]}" != "${connector}" ]; then
                    echo "Change Port $(expr $port + 1) to AOC"
                    QSFP_ARRAY[$port]=${connector}
                    ${QSFP_SI_SCRIPT} aoc $port >/dev/null
                fi
            ;;
        esac
    fi
}

#Docker exist check
function _docker_swss_check {
    while true
    do
        # Check if syncd starts
        result=`sonic-db-cli ASIC_DB HLEN HIDDEN`
        if [ "$result" == "3" ]; then
            return
        fi
        sleep $INTERVAL
    done
}

#Docker exist check
function _qsfp_si_cfg_script_check {

    if [ -f ${QSFP_SI_SCRIPT} ] && [ -x ${QSFP_SI_SCRIPT} ]; then
        echo "SI Script exists. Start monitor."
        return
    else
        echo "SI Script not exist. Exit monitor."
        exit
    fi
}

# main function
function _main {
    #Check SI Script
    _qsfp_si_cfg_script_check
    #Check docker swss is running
    _docker_swss_check
    while true
    do
        _qsfp_si_monitor
        # Sleep while still handling signals
        sleep $INTERVAL &
        wait $!
    done
}

_main

