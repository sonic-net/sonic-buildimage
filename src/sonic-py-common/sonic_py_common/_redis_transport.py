"""Choose the default transport for helpers that read one local database."""

import os

from swsscommon import swsscommon


def use_local_redis_socket(db_name, namespace=None):
    key = swsscommon.SonicDBKey(namespace or '')
    if swsscommon.SonicDBConfig.getDbPort(db_name, key) == 0:
        return True

    path = swsscommon.SonicDBConfig.getDbSock(db_name, key)
    return bool(path) and os.access(path, os.W_OK)
