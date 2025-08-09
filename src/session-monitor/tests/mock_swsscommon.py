class DBConnector:
    def __init__(self, db_name, timeout, use_unix_socket_path=False):
        self.db_name = db_name
        self.timeout = timeout
        self.use_unix_socket_path = use_unix_socket_path

class Table:
    def __init__(self, db, table_name):
        self.db = db
        self.table_name = table_name
        self.data = {}
    
    def get(self, key):
        """Mock get method - returns (exists, list_of_tuples)"""
        if str(key) in self.data:
            return True, list(self.data[str(key)].items())
        else:
            return False, []
    
    def getKeys(self):
        """Get all keys in the table"""
        return list(self.data.keys())
    
    def hset(self, key, field, value):
        """Mock hset method for setting individual fields"""
        if str(key) not in self.data:
            self.data[str(key)] = {}
        self.data[str(key)][field] = value
        return True
    
    def hget(self, key, field):
        """Mock hget method - returns (exists, value)"""
        if str(key) in self.data and field in self.data[str(key)]:
            return True, self.data[str(key)][field]
        else:
            return False, None
    
    def set_data(self, key, data_dict):
        """Helper method for tests to set up data"""
        self.data[str(key)] = data_dict
    
    def delete(self, key):
        """Delete an entry"""
        key_str = str(key)
        if key_str in self.data:
            del self.data[key_str]
            return True
        else:
            return False

class SubscriberStateTable(Table):
    def __init__(self, db, table_name):
        super().__init__(db, table_name)
        self.updates = []   # Queue for updates
        self.fd = id(self)  # Unique identifier for this table
    
    def pop(self):
        """Pop the next update from the queue"""
        if self.updates:
            return self.updates.pop(0)
        else:
            return "", "", []
    
    def add_update(self, key, operation, fvp):
        """Add an update to the queue for testing"""
        self.updates.append((key, operation, fvp))
    
    def getFd(self):
        """Return file descriptor for select operations"""
        return self.fd

class Select:
    TIMEOUT = 0
    ERROR = 1
    OBJECT = 2
    
    def __init__(self):
        self.selectables = []
        self.return_values = []
    
    def addSelectable(self, selectable):
        self.selectables.append(selectable)
    
    def select(self, timeout):
        """Mock select that returns pre-configured values"""
        if self.return_values:
            state, selectable = self.return_values.pop(0)
            return state, selectable
        return Select.TIMEOUT, None