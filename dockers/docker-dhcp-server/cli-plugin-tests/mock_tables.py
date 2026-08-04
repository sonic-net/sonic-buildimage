# MONKEY PATCH!!!
import json
import os
from unittest import mock

import mockredis
import redis
import swsssdk
from sonic_py_common import multi_asic
from swsssdk import SonicDBConfig, SonicV2Connector, ConfigDBConnector, ConfigDBPipeConnector
from swsscommon import swsscommon


topo = None
dedicated_dbs = {}

def clean_up_config():
    # Set SonicDBConfig variables to initial state
    # so that it can be loaded with single or multiple
    # namespaces before the test begins.
    SonicDBConfig._sonic_db_config = {}
    SonicDBConfig._sonic_db_global_config_init = False
    SonicDBConfig._sonic_db_config_init = False

def load_namespace_config():
    # To support multi asic testing
    # SonicDBConfig load_sonic_global_db_config
    # is invoked to load multiple namespaces
    clean_up_config()
    SonicDBConfig.load_sonic_global_db_config(
        global_db_file_path=os.path.join(
            os.path.dirname(os.path.abspath(__file__)), 'database_global.json'))

def load_database_config():
    # Load local database_config.json for single namespace test scenario
    clean_up_config()
    SonicDBConfig.load_sonic_db_config(
        sonic_db_file_path=os.path.join(
            os.path.dirname(os.path.abspath(__file__)), 'database_config.json'))


_old_connect_SonicV2Connector = SonicV2Connector.connect

def connect_SonicV2Connector(self, db_name, retry_on=True):
    # add topo to kwargs for testing different topology
    self.dbintf.redis_kwargs['topo'] = topo
    # add the namespace to kwargs for testing multi asic
    self.dbintf.redis_kwargs['namespace'] = self.namespace
    # Mock DB filename for unit-test
    global dedicated_dbs
    if dedicated_dbs and dedicated_dbs.get(db_name):
        self.dbintf.redis_kwargs['db_name'] = dedicated_dbs[db_name]
    else:
        self.dbintf.redis_kwargs['db_name'] = db_name
    self.dbintf.redis_kwargs['decode_responses'] = True
    _old_connect_SonicV2Connector(self, db_name, retry_on)

def _subscribe_keyspace_notification(self, db_name, client):
    pass


def config_set(self, *args):
    pass


class MockPubSub:
    def get_message(self):
        return None

    def psubscribe(self, *args, **kwargs):
        pass

    def __call__(self, *args, **kwargs):
        return self

    def listen(self):
        return []

    def punsubscribe(self, *args, **kwargs):
        pass

    def clear(self):
        pass

INPUT_DIR = os.path.dirname(os.path.abspath(__file__))


class SwssSyncClient(mockredis.MockRedis):
    def __init__(self, *args, **kwargs):
        super(SwssSyncClient, self).__init__(strict=True, *args, **kwargs)
        # Namespace is added in kwargs specifically for unit-test
        # to identify the file path to load the db json files.
        topo = kwargs.pop('topo')
        namespace = kwargs.pop('namespace')
        db_name = kwargs.pop('db_name')
        self.decode_responses = kwargs.pop('decode_responses', False) == True
        fname = db_name.lower() + ".json"
        self.pubsub = MockPubSub()

        if namespace is not None and namespace is not multi_asic.DEFAULT_NAMESPACE:
            fname = os.path.join(INPUT_DIR, namespace, fname)
        elif topo is not None:
            fname = os.path.join(INPUT_DIR, topo, fname)
        else:
            fname = os.path.join(INPUT_DIR, fname)

        if os.path.exists(fname):
            with open(fname) as f:
                js = json.load(f)
                for k, v in js.items():
                    if 'expireat' in v and 'ttl' in v and 'type' in v and 'value' in v:
                        # database is in redis-dump format
                        if v['type'] == 'hash':
                            # ignore other types for now since sonic has hset keys only in the db
                            for attr, value in v['value'].items():
                                self.hset(k, attr, value)
                    else:
                        for attr, value in v.items():
                            self.hset(k, attr, value)

    # Patch mockredis/mockredis/client.py
    # The offical implementation assume decode_responses=False
    # Here we detect the option and decode after doing encode
    def _encode(self, value):
        "Return a bytestring representation of the value. Taken from redis-py connection.py"

        value = super(SwssSyncClient, self)._encode(value)

        if self.decode_responses:
           return value.decode('utf-8')

    # Patch mockredis/mockredis/client.py
    # The official implementation will filter out keys with a slash '/'
    # ref: https://github.com/locationlabs/mockredis/blob/master/mockredis/client.py
    def keys(self, pattern='*'):
        """Emulate keys."""
        import fnmatch
        import re

        # Make regex out of glob styled pattern.
        regex = fnmatch.translate(pattern)
        regex = re.compile(regex)

        # Find every key that matches the pattern
        return [key for key in self.redis if regex.match(key)]


swsssdk.interface.DBInterface._subscribe_keyspace_notification = _subscribe_keyspace_notification
mockredis.MockRedis.config_set = config_set
redis.StrictRedis = SwssSyncClient
SonicV2Connector.connect = connect_SonicV2Connector
swsscommon.SonicV2Connector = SonicV2Connector
swsscommon.ConfigDBConnector = ConfigDBConnector
swsscommon.ConfigDBPipeConnector = ConfigDBPipeConnector
