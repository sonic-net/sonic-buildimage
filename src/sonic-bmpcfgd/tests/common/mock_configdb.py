class MockConfigDb(object):
    """
        Mock Config DB which responds to data tables requests and store updates to the data table
    """
    STATE_DB = None
    CONFIG_DB = None
    event_queue = []

    def __init__(self, **kwargs):
        self.handlers = {}

    @staticmethod
    def set_config_db(test_config_db):
        MockConfigDb.CONFIG_DB = test_config_db

    @staticmethod
    def mod_config_db(test_config_db):
        MockConfigDb.CONFIG_DB.update(test_config_db)

    @staticmethod
    def deserialize_key(key, separator="|"):
        tokens = key.split(separator)
        if len(tokens) > 1:
            return tuple(tokens)
        else:
            return key

    @staticmethod
    def get_config_db():
        return MockConfigDb.CONFIG_DB

    def connect(self, wait_for_init=True, retry_on=True):
        pass

    def close(self, db_name):
        pass

    def get(self, db_id, key, field):
        return MockConfigDb.CONFIG_DB[key][field]

    def get_entry(self, key, field):
        return MockConfigDb.CONFIG_DB[key][field]

    def mod_entry(self, key, field, data):
        existing_data = self.get_entry(key, field)
        existing_data.update(data)
        self.set_entry(key, field, existing_data)

    def set_entry(self, key, field, data):
        MockConfigDb.CONFIG_DB[key][field] = data

    def get_table(self, table_name):
        data = {}
        if table_name in MockConfigDb.CONFIG_DB:
            for k, v in MockConfigDb.CONFIG_DB[table_name].items():
                data[self.deserialize_key(k)] = v
        return data

    def subscribe(self, table_name, callback):
        self.handlers[table_name] = callback

    def publish(self, table_name, key, op, data):
        self.handlers[table_name](key, op, data)

    def listen(self, init_data_handler=None):
        for e in MockConfigDb.event_queue:
            self.handlers[e[0]](e[0], e[1], self.get_entry(e[0], e[1]))


class MockSelect():

    event_queue = []
    OBJECT = "OBJECT"
    TIMEOUT = "TIMEOUT"
    ERROR = ""
    NUM_TIMEOUT_TRIES = 0

    @staticmethod
    def set_event_queue(Q):
        MockSelect.event_queue = Q

    @staticmethod
    def get_event_queue():
        return MockSelect.event_queue

    @staticmethod
    def reset_event_queue():
        MockSelect.event_queue = []

    def __init__(self):
        self.sub_map = {}
        self.TIMEOUT = "TIMEOUT"
        self.ERROR = "ERROR"

    def addSelectable(self, subscriber):
        self.sub_map[subscriber.table] = subscriber

    def select(self, TIMEOUT):
        if not MockSelect.get_event_queue() and MockSelect.NUM_TIMEOUT_TRIES == 0:
            raise TimeoutError
        elif MockSelect.NUM_TIMEOUT_TRIES != 0:
            MockSelect.NUM_TIMEOUT_TRIES = MockSelect.NUM_TIMEOUT_TRIES - 1
            return MockSelect.TIMEOUT, 0
        
        table, key = MockSelect.get_event_queue().pop(0)
        self.sub_map[table].nextKey(key)
        return "OBJECT", self.sub_map[table]


class MockSubscriberStateTable():

    FD_INIT = 0

    @staticmethod
    def generate_fd():
        curr = MockSubscriberStateTable.FD_INIT
        MockSubscriberStateTable.FD_INIT = curr + 1
        return curr

    @staticmethod
    def reset_fd():
        MockSubscriberStateTable.FD_INIT = 0

    def __init__(self, conn, table, pop=None, pri=None):
        self.fd = MockSubscriberStateTable.generate_fd()
        self.next_key = ''
        self.table = table

    def getFd(self):
        return self.fd

    def nextKey(self, key):
        print("next key")
        self.next_key = key

    def pop(self):
        table = MockConfigDb.CONFIG_DB.get(self.table, {})
        print(self.next_key)
        if self.next_key not in table:
            op = "DEL"
            fvs = {}
        else:
            op = "SET"
            fvs = table.get(self.next_key, {})
        return self.next_key, op, fvs


class MockDBConnector():
    def __init__(self, db, val, tcpFlag=False, name=None):
        self.data = {}

    def hget(self, key, field):
        if key not in self.data:
            return None
        if field not in self.data[key]:
            return None
        return self.data[key][field]

    def hset(self, key, field, value):
        if key not in self.data:
            self.data[key] = {}
        self.data[key][field] = value
