class MockConnector(object):
    STATE_DB = None
    data = {}

    def __init__(self, host):
        pass

    def connect(self, db_id):
        pass

    def get(self, db_id, key, field):
        return MockConnector.data[key][field]

    def set(self, db_id, key, field, value):
        if key not in MockConnector.data:
            MockConnector.data[key] = {}
        MockConnector.data[key][field] = value

    def keys(self, db_id, pattern):
        match = pattern.split('*')[0]
        ret = []
        for key in MockConnector.data.keys():
            if match in key:
                ret.append(key)

        return ret

    def get_all(self, db_id, key):
        return MockConnector.data[key]

    def delete(self, db_id, key):
        return MockConnector.data.delete(key)

    def delete_all_by_pattern(self, db_id, pattern):
        keys = self.keys(db_id, pattern)
        for key in keys:
            self.delete(db_id, key)


