import json

MOCK_CONFIG_DB_PATH = "tests/test_data/mock_config_db.json"


class MockConfigDb(object):
    def __init__(self, config_db_path=MOCK_CONFIG_DB_PATH):
        with open(config_db_path, "r", encoding="utf8") as file:
            self.config_db = json.load(file)

    def get_config_db_table(self, table_name):
        return self.config_db.get(table_name, {})
