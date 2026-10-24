#!/bin/bash

dump_folder="/tmp/hw-mgmt-dump"
tmp_script="tmp_script.sh"

if [ -d $dump_folder ]; then
    rm -rf $dump_folder
fi

mkdir $dump_folder

dump_cmd() {
    echo "===============================   $@ (start)"
    "$@"
    echo -e "===============================   $@ (end)\n\n\n"
}

{
    dump_cmd show platform firmware status
    dump_cmd show platform firmware updates
    dump_cmd show platform ssdhealth
    dump_cmd show boot
    dump_cmd w
    dump_cmd dmidecode -t1 -t2 -t 11

} > $dump_folder/general.txt

{
    for file in `ls /sys/bus/i2c/devices/ | sort -g`;
    do
        device_name=$(cat /sys/bus/i2c/devices/$file/name)
        echo -e "${file} name:  $device_name \n"
    done
} > $dump_folder/i2cdevices.txt

cat > /tmp/$tmp_script <<EOF
{
echo "stats_dump all" | socat - UNIX-CONNECT:/tmp/sai_cli_server
}
EOF

docker cp /tmp/$tmp_script syncd:/
docker exec syncd bash /$tmp_script

docker exec syncd bash -c "cp /tmp/stats_dump.txt /stats_dump.txt"

docker cp syncd:/stats_dump.txt $dump_folder/
docker exec syncd bash -c "rm /$tmp_script"
docker exec syncd bash -c "rm /stats_dump.txt"

rm /tmp/$tmp_script

tar czf /tmp/hw-mgmt-dump.tar.gz -C $dump_folder .
rm -rf $dump_folder
