#!/bin/bash
REV=$(lspci -xxx -s 0:0.0 | grep rev | awk -F 'rev ' '{print $2}' | sed 's/)//')
if [ $REV -gt 2 ]
then
    exit 0
fi

test_val=(55 aa)
num_val=${#test_val[@]}
index=0
poll_interval=300
cpld_scratch_reg=0x102
smf_scratch_reg=0x202

function log_crit() {
   local msg=$1

  `logger -p user.crit -t DELL_LPC_BUS_MON $msg`
}

function validate_lpc() {
    local reg=$1
    local val=$2
    local reg_str="CPLD scratch register"

    if [ $reg == $smf_scratch_reg ]
    then
        reg_str="SMF scratch register"
    fi
    io_rd_wr.py --set --val $val --offset $reg
    get_val=$(io_rd_wr.py  --get --offset $reg | cut -d " " -f3)
    if [ $val != $get_val ]
    then
        log_crit "LPC bus has deteriorated on this unit. \
                  $reg_str has value $get_val while expected is $val \
                  Please contact technical support"
    fi
}
while true
do
    val=${test_val[$index]}
    validate_lpc $cpld_scratch_reg $val
    validate_lpc $smf_scratch_reg $val
    index=$(((index+1)%num_val))
    sleep $poll_interval
done
