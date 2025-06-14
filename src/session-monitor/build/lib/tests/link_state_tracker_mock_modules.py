class MockDBConnector:
    """
    Mock for DBConnector for database connections
    """

    CONFIG_DB = 0
    STATE_DB = 1

    def __init__(self, db_name):
        self.db_name = db_name
        self.tables = {}

class MockTable:
    """
    Base class for table mocks
    """
    def __init__(self, db_connector, table_name):
        self.db_connector = db_connector
        self.table_name = table_name
        self.data = {}
    
    def get(self, key):
        """
        Get a key's value from the table
        """
        if str(key) in self.data:
            return True, list(self.data[str(key)].items())
        else:
            return False, []
    
    def set(self, key, value):
        """
        Set a key-value pair in the table
        """
        if str(key) not in self.data:
            self.data[str(key)] = {}
        
        # Value can be a list and dict
        if isinstance(value, list):
            for name, val in value:
                self.data[str(key)][name] = val
        elif isinstance(value, dict):
            for name, val in value.items():
                self.data[str(key)][name] = val
    
    def delete(self, *args):
        """
        Delete a key from the table
        """
        key = args[0]
        if str(key) in self.data:
            self.data.pop(str(key))
    
    def hget(self, key, field):
        """
        Get a field from a key in the table
        """
        if str(key) in self.data and field in self.data[str(key)]:
            return True, self.data[str(key)][field]
        else:
            return False, None

class MockStateTable(MockTable):
    """
    Mock for state table
    """
    pass

class MockConfigTable(MockTable):
    """
    Mock for config table
    """
    pass

class MockLagTable:
    """
    Mock a SubscriberStateTable for testing
    See sonic-swss-common/common/subscriberstatetable.cpp
    """
    def __init__(self, db, table_name="mock_table"):
        self.db = db
        self.table_name = table_name
        self.updates = [] # List of tuples (keys, operations, dict of field_value pairs)
        self.data = {}
    
    def pop(self):
        """
        Pop the next update
        """
        if self.updates:
            return self.updates.pop(0)
        else:
            return None, None, {} # key, operation, field_value_sets
    
    def add_update(self, key, operation, fvp):
        """
        Add an update to the queue for testing
        """
        self.updates.append((key, operation, fvp))

    def add(self, key, value):
        """
        Add a key with an up/down value to the table
        """
        self.data[key] = {"oper_status": value}

    def add_portchannel(self, portchannel, status):
        """
        Add a portchannel with up/down status
        """
        self.data[portchannel] = {"oper_status": status}
        
    def add_event(self, key, operation, field_values):
        """
        Add an event to the mock lag table
        """
        self.updates.append((key, operation, field_values))
        
    def get(self, key):
        """
        Get data for a key
        """
        if key in self.data:
            return True, [(k, v) for k, v in self.data[key].items()]
        return False, []
    
class MockSelect:
    """
    Mock the swsscommon.Select for testing
    See sonic-swss-common/common/select.cpp
    """
    TIMEOUT = 0
    ERROR = 1
    OBJECT = 2 # FD_ISSET

    def __init__(self):
        self.selectable_objects = []
        self.return_value = []  # List of tuples (state, selectableObj)
    
    def select(self, timeout_ms):
        """
        Simulate a select() call
        """
        if self.return_values:
            return self.return_value.pop(0)
        return self.TIMEOUT, None

    def add_return_value(self, state, selectableObj=None):
        """
        Add a return value for the next select() call
        """
        self.return_value.append((state, selectableObj))
    
    def addSelectable(self, selectable):
        """
        Add a selectable object to the select
        """
        self.selectable_objects.append(selectable)

class MockSubprocess:
    """
    Mock subprocess for testing
    """
    def __init__(self):
        self.commands = []
        self.return_values = {}

    class MockCompletedProcess:
        def __init__(self, args, returncode, stdout, stderr):
            self.args = args
            self.returncode = returncode
            self.stdout = stdout
            self.stderr = stderr

    def run(self, command, **kwargs):
        """
        Mock the subprocess.run() method for testing
        """
        command_tuple = tuple(command)
        self.commands.append((command_tuple, kwargs))

        if command_tuple in self.return_values:
            if isinstance(self.return_values[command_tuple], Exception):
                raise self.return_values[command_tuple]
            return self.return_values[command_tuple]
        return self.MockCompletedProcess(command_tuple)
    
    def set_return_value(self, command, result):
        """
        Set the return value for a specific command
        """
        self.return_values[tuple(command)] = result

    def set_exception(self, command, exception):
        """
        Set an exception to be raised for a specific command
        """
        self.return_values[tuple(command)] = exception