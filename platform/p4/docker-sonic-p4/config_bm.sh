simple_switch_CLI --pre SimplePreLAG < /usr/share/p4-sai-bm/bridge_default_config.txt
simple_switch_CLI < /usr/share/p4-sai-bm/bridge_default_config_mirror.txt
simple_switch_CLI --pre SimplePreLAG --thrift-port 9091 < /usr/share/p4-sai-bm/router_default_config.txt
simple_switch_CLI --thrift-port 9091 < /usr/share/p4-sai-bm/router_default_config_mirror.txt