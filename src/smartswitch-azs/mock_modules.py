class MockConfigDb(object):
    def __init__(self):
        self.data = {}
    
    def get_table(self, table_name):
        if table_name in self.data:
            return self.data[table_name]
        
    def set_entry(self, table_key, key, value):
        if table_key not in self.data:
            self.data[table_key] = {}
        self.data[table_key][key] = value
    
    def delete(self, table_key, key):
        if table_key in self.data and key in self.data[table_key]:
            self.data[table_key].pop(key)

class MockStateLagTable(object):
    def __init__(self):
        self.portchannels = {}

    def set(self, key, value):
        if key not in self.portchannels:
            self.portchannels[key] = {}
        self.portchannels[key] = value

    def get(self, key):
        if key in self.portchannels:
            return True, self.portchannels[key]
        else:
            return False, None

    def hget(self, table_key, key):
        if table_key in self.portchannels and key in self.portchannels[table_key]:
            return True, self.portchannels[table_key][key]
        else:
            return False, None

class MockStateDbTable(object):
    def __init__(self):
        self.data = {}

    def set(self, key, value):
        if key not in self.data:
            self.data[key] = {}
        self.data[key] = value

    def get(self, key):
        if key in self.data:
            return True, self.data[key]
        else:
            return False, None

    def hget(self, table_key, key):
        if table_key in self.data and key in self.data[table_key]:
            return True, self.data[table_key][key]
        else:
            return False, None

    def delete(self, table_key):
        if table_key in self.data:
            self.data.pop(table_key)

class MockRouteTable(object):
    def __init__(self):
        self.routedata = {}

    def set(self, key, value):
        if key not in self.routedata:
            self.routedata[key] = {}
        self.routedata[key] = value

    def get(self, key):
        if key in self.routedata:
            return True, self.routedata[key]
        else:
            return False, None

    def hget(self, table_key, key):
        if table_key in self.routedata and key in self.routedata[table_key]:
            return True, self.routedata[table_key][key]
        else:
            return False, None

    def delete(self, table_key):
        if table_key in self.routedata:
            self.routedata.pop(table_key)

