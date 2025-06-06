#!/bin/bash

if [ $# -ne 0  ]; then
    if [ "$1" == "-h" ]; then
        echo "Script patches the sysObjectID by modifying .docker/docker-snmp/start.sh in workspace before building the image"
        echo ""
        echo "Syntax: NokiaSnmpBuildTimePatch.sh  [-h]"
        echo "   Optional Parameter:"
        echo "        -h      -- Display this syntax help message"
        echo ""
        exit 0
    else
        echo "Synatx error"
        exit 0
    fi
fi

if [ ! -e ./dockers/docker-snmp/start.sh ]; then
    echo "Error: Patching failed"
    echo "File ./dockers/docker-snmp/start.sh not found. Script must be executed at the sonic-buildimage directory"
    echo ""
    exit 0
fi

grep -q "# Signature: Start patching sysObjectID" ./dockers/docker-snmp/start.sh
if [ $? -eq 0 ]; then
    echo "Skip patching dockers/docker-snmp/start.sh file. Patching is found in dockers/docker-snmp/start.sh"
else
    echo "Patching dockers/docker-snmp/start.sh"
    echo '
# Signature: Start patching sysObjectID
grep "^sysObjectID" /etc/snmp/snmpd.conf
if [ $? -ne 0 ]; then
    ObjectID_TXT="/usr/share/sonic/platform/sysObjectID.txt" 
    if [ -e $ObjectID_TXT ]; then
        objectID=$(head -1 $ObjectID_TXT)
        if [ "$objectID" != "" ]; then
            echo "sysObjectID $objectID" >> /etc/snmp/snmpd.conf
            grep "^sysObjectID" /usr/share/sonic/templates/snmpd.conf.j2
            if [ $? -ne 0 ]; then
                echo "sysObjectID $objectID" >> /usr/share/sonic/templates/snmpd.conf.j2
            fi
        fi
    fi
fi' >> ./dockers/docker-snmp/start.sh
fi

# platform and its OID must be defined in the same position in followign two list 
platform_list=("arm64-nokia_ixs7215_52xb-r0" "x86_64-nokia_ixr7250_x1b-r0" "x86_64-nokia_ixr7250_x3b-r0" "x86_64-nokia_ixr7220_h4-r0" "x86_64-nokia_ixr7220_d4-r0")
oid_list=(".1.3.6.1.4.1.6527.1.20.91" ".1.3.6.1.4.1.6527.1.20.92" ".1.3.6.1.4.1.6527.1.20.93" ".1.3.6.1.4.1.6527.1.20.94" ".1.3.6.1.4.1.6527.1.20.95")

x=0
for platform in "${platform_list[@]}"; do
    if [ -d ./device/nokia/$platform ]; then
       if [ -f ./device/nokia/$platform/sysObjectID.txt ]; then
           echo "Skip patching for $platform platform. File device/nokia/$platform/sysObjectID.txt is found"
       else
           echo "Patching sysObjectID for $platform by adding file device/nokia/$platform/sysObjectID.txt"
           echo "${oid_list[$x]}" > ./device/nokia/$platform/sysObjectID.txt
       fi
    fi
    x=$(( $x + 1 ))
done
echo "Build time patching done" 
