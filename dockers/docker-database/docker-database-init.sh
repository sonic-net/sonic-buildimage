#!/usr/bin/env bash

# For linux host namespace, in both single and multi ASIC platform use the loopback interface
# For other namespaces, use eth0 interface which is connected to the docker0 bridge in the host.
if [[ $NAMESPACE_ID == "" ]]
then
    INTFC=lo
else
    INTFC=eth0
fi

# Get the ip address of the interface
# if the ip address was not retrieved correctly, put localhost(127.0.0.1) as the default.
host_ip=$(ip -4 -o addr show $INTFC | awk '{print $4}' | cut -d'/' -f1 | head -1)
if [[ $host_ip == "" ]]
then
    host_ip=127.0.0.1
fi

REDIS_DIR=/var/run/redis$NAMESPACE_ID
mkdir -p $REDIS_DIR/sonic-db
mkdir -p /etc/supervisor/conf.d/

if [ -f /etc/sonic/database_config$NAMESPACE_ID.json ]; then
    cp /etc/sonic/database_config$NAMESPACE_ID.json $REDIS_DIR/sonic-db/database_config.json
else
    HOST_IP=$host_ip j2 /usr/share/sonic/templates/database_config.json.j2 > $REDIS_DIR/sonic-db/database_config.json
fi

# on VoQ system, we only publish redis_chassis instance and CHASSIS_APP_DB when
# either chassisdb.conf indicates starts chassis_db or connect to chassis_db,
# and redis_chassis instance is started in different container.
# in order to do that, first we save original database config file, then
# call update_chasissdb_config to remove chassis_db config from
# the original database config file and use the modified config file to generate
# supervisord config, so that we won't start redis_chassis service.
# then we will decide to publish modified or original database config file based
# on the setting in chassisdb.conf
start_chassis_db=0
chassis_db_address=""
chassis_db_port=""
chassisdb_config="/usr/share/sonic/platform/chassisdb.conf"
[ -f $chassisdb_config ] && source $chassisdb_config

db_cfg_file="/var/run/redis/sonic-db/database_config.json"
db_cfg_file_tmp="/var/run/redis/sonic-db/database_config.json.tmp"
cp $db_cfg_file $db_cfg_file_tmp

if [[ $DATABASE_TYPE == "chassisdb" ]]; then
    # Docker init for database-chassis
    echo "Init docker-database-chassis..."
    update_chassisdb_config -j $db_cfg_file_tmp -k -p $chassis_db_port
    # generate all redis server supervisord configuration file
    sonic-cfggen -j $db_cfg_file_tmp -t /usr/share/sonic/templates/supervisord.conf.j2 > /etc/supervisor/conf.d/supervisord.conf
    sonic-cfggen -j $db_cfg_file_tmp -t /usr/share/sonic/templates/critical_processes.j2 > /etc/supervisor/critical_processes
    rm $db_cfg_file_tmp
    exec /usr/local/bin/supervisord
    exit 0
fi

# copy/generate the database_global.json file if this is global database service in multi asic platform.
if [[ $NAMESPACE_ID == "" ]] && [[ $NAMESPACE_COUNT -gt 1 ]]
then
    if [ -f /etc/sonic/database_global.json ]; then
        cp /etc/sonic/database_global.json $REDIS_DIR/sonic-db/database_global.json
    else
        j2 /usr/share/sonic/templates/database_global.json.j2 > $REDIS_DIR/sonic-db/database_global.json
    fi
fi
# delete chassisdb config to generate supervisord config
update_chassisdb_config -j $db_cfg_file_tmp -d
sonic-cfggen -j $db_cfg_file_tmp -t /usr/share/sonic/templates/supervisord.conf.j2 > /etc/supervisor/conf.d/supervisord.conf
sonic-cfggen -j $db_cfg_file_tmp -t /usr/share/sonic/templates/critical_processes.j2 > /etc/supervisor/critical_processes

if [[ "$start_chassis_db" != "1" ]] && [[ -z "$chassis_db_address" ]]; then
     cp $db_cfg_file_tmp $db_cfg_file
else
     update_chassisdb_config -j $db_cfg_file -p $chassis_db_port
fi
rm $db_cfg_file_tmp

# copy dump.rdb file to each instance for restoration
DUMPFILE=/var/lib/redis/dump.rdb
redis_inst_list=`/usr/bin/python3 -c "from swsscommon import swsscommon; print(' '.join(swsscommon.SonicDBConfig.getInstanceList().keys()))"`
for inst in $redis_inst_list
do
    mkdir -p /var/lib/$inst
    if [[ -f $DUMPFILE ]]; then
        # copy warmboot rdb file into each new instance location
        if [[ "$DUMPFILE" != "/var/lib/$inst/dump.rdb" ]]; then
            cp $DUMPFILE /var/lib/$inst/dump.rdb
        fi
    else
        echo -n > /var/lib/$inst/dump.rdb
    fi
done

TZ=$(cat /etc/timezone)
rm -rf /etc/localtime
ln -sf /usr/share/zoneinfo/$TZ /etc/localtime

chown -R redis:redis $REDIS_DIR

exec /usr/local/bin/supervisord
