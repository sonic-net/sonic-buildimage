"""Choose the default transport for helpers that read one local database."""

import ipaddress
import os

from swsscommon import swsscommon


def use_local_redis_socket(db_name, namespace=None):
    key = swsscommon.SonicDBKey(namespace or '')
    host = swsscommon.SonicDBConfig.getDbHostname(db_name, key)
    path = swsscommon.SonicDBConfig.getDbSock(db_name, key)
    # ASIC metadata advertises the database container's bridge address.
    asic = namespace[4:] if namespace and namespace.startswith('asic') else ''
    asic_socket = (asic.isdigit() and os.path.dirname(path) in
                   ('/var/run/redis' + asic, '/run/redis' + asic))
    if host != 'localhost':
        try:
            if not ipaddress.ip_address(host).is_loopback and not asic_socket:
                return False
        except ValueError:
            return False
    if swsscommon.SonicDBConfig.getDbPort(db_name, key) == 0:
        return True

    return bool(path) and os.access(path, os.W_OK)
