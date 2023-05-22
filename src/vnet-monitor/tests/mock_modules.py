class MockStateDbTable(object):
    def __init__(self):
        self.data = {}
    
    def set(self, table_key, kvs):
        if table_key not in self.data:
            self.data[table_key] = {}
        for kv in kvs:
            self.data[table_key][kv[0]] = kv[1]
    
    def hget(self, table_key, key):
        if table_key in self.data and key in self.data[table_key]:
            return True, self.data[table_key][key]
        else:
            return False, None
    
    def delete(self, *args):
        table_key = args[0]
        if table_key in self.data:
            self.data.pop(table_key)
