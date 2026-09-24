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
        return MockConnector.data[key]

    def exists(self, db_id, key):
        return key in MockConnector.data

    def set(self, db_id, key, field, value):
        self.data[key] = {}
        self.data[key][field] = value

    def hmset(self, db_id, key, fieldsvalues):
        self.data[key] = {}
        for field,value in fieldsvalues.items():
            self.data[key][field] = value
