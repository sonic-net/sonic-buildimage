class SonicV2Connector:
    TEST_SERIAL = "MT1822K07815"
    TEST_MODEL = "MSN2700-CS2FO"
    TEST_REV = "A1"

    def __init__(self):
        self.STATE_DB = 'STATE_DB'
        self.data = {"serial": self.TEST_SERIAL,
                     "model": self.TEST_MODEL,
                     "revision": self.TEST_REV}

    def connect(self, db):
        pass

    def get(self, db, table, field):
        return self.data.get(field, "N/A")
