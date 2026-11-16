class MockRedisCommand(object):
    def __init__(self):
        self._op = None
        self._table = None
        self._field = None
        self._value = None

    def formatDEL(self, table):
        self._op = 'del'
        self._table = table

    def formatHSET(self, table, field, value):
        self._op = 'hset'
        self._table = table
        self._field = field
        self._value = value

    def formatHDEL(self, table, field):
        self._op = 'hdel'
        self._table = table
        self._field = field


class MockRedisTransactioner(object):
    def __init__(self, connector):
        self._data = connector._data if hasattr(connector, '_data') else MockConnector.data
        self._cmds = []

    def multi(self):
        self._cmds = []

    def enqueue(self, cmd, expected_type):
        self._cmds.append(cmd)

    def _exec(self):
        for cmd in self._cmds:
            if cmd._op == 'del':
                if cmd._table in self._data:
                    del self._data[cmd._table]
            elif cmd._op == 'hdel':
                if cmd._table in self._data and cmd._field in self._data[cmd._table]:
                    del self._data[cmd._table][cmd._field]
            elif cmd._op == 'hset':
                if cmd._table not in self._data:
                    self._data[cmd._table] = {}
                self._data[cmd._table][cmd._field] = cmd._value
        self._cmds = []
        return True


class MockRedisClient(object):
    def __init__(self, data):
        self._data = data

    def hdel(self, key, field):
        if key in self._data and field in self._data[key]:
            del self._data[key][field]


class MockConnector(object):
    STATE_DB = None
    data = {}

    def __init__(self, use_unix_socket_path):
        pass

    def connect(self, db_id):
        pass

    def get(self, db_id, key, field):
        return MockConnector.data[key][field]

    def keys(self, db_id, pattern):
        match = pattern.split('*')[0]
        ret = []
        for key in MockConnector.data.keys():
            if match in key:
                ret.append(key)

        return ret

    def get_all(self, db_id, key):
        return MockConnector.data.get(key, {})

    def exists(self, db_id, key):
        return key in MockConnector.data

    def set(self, db_id, key, field, value):
        if key not in self.data:
            self.data[key] = {}
        self.data[key][field] = value

    def hmset(self, db_id, key, fieldsvalues):
        if key not in self.data:
            self.data[key] = {}
        for field, value in fieldsvalues.items():
            self.data[key][field] = value

    def delete_all_by_pattern(self, db_id, pattern):
        match = pattern.replace('*', '')
        keys_to_delete = [k for k in list(self.data.keys()) if k.startswith(match)]
        for k in keys_to_delete:
            del self.data[k]

    def get_redis_client(self, db_id):
        return MockRedisClient(self.data)
