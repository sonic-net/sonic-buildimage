"""Choose the default transport for helpers that read one local database."""

import ipaddress
import os

from swsscommon import swsscommon


def use_local_redis_socket(db_name, namespace=None):
    key = swsscommon.SonicDBKey(namespace or '')
    host = swsscommon.SonicDBConfig.getDbHostname(db_name, key)
    if host != 'localhost':
        try:
            if not ipaddress.ip_address(host).is_loopback:
                return False
        except ValueError:
            return False
    if swsscommon.SonicDBConfig.getDbPort(db_name, key) == 0:
        return True

    path = swsscommon.SonicDBConfig.getDbSock(db_name, key)
    return bool(path) and os.access(path, os.W_OK)
