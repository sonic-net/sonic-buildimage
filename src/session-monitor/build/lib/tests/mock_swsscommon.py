class DBConnector:
    def __init__(self, db_name, timeout, use_unix_socket_path=False):
        self.db_name = db_name
        self.timeout = timeout
        self.use_unix_socket_path = use_unix_socket_path

class Table:
    def __init__(self, db, table_name):
        self.db = db
        self.table_name = table_name
    
    def get(self, key):
        return False, []
    
    def getKeys(self):
        return []

class SubscriberStateTable(Table):
    def pop(self):
        return "", "", []

class Select:
    TIMEOUT = 0
    ERROR = 1
    OBJECT = 2
    
    def __init__(self):
        self.selectables = []
    
    def addSelectable(self, selectable):
        self.selectables.append(selectable)
    
    def select(self, timeout):
        return Select.TIMEOUT, None