"""sonic_yang_cfg_generator
version_added: "1.0"
author: Arvindsrinivasan Lakshmi naraismhan (arlakshm@microsoft.com)
short_description: Parse sonic_yang data and generate the config_db entries
"""

from __future__ import print_function

import json
import sys


import sonic_yang
# TODO: Remove this once we no longer support Python 2
if sys.version_info.major == 3:
    UNICODE_TYPE = str
else:
    UNICODE_TYPE = unicode

YANG_MODELS_DIR = "/usr/local/yang-models"
DEFAULT_YANG_DATA_FILE = "/etc/sonic/config_yang.json"


class SonicYangCfgDbGenerator:

    def __init__(self, yang_models_dir=YANG_MODELS_DIR):
        self.yang_models_dir = yang_models_dir
        self.yang_parser = sonic_yang.SonicYang(self.yang_models_dir)
        self.yang_parser.loadYangModel()

    def get_config_db_from_yang_data(self,
                                     yang_data_file=DEFAULT_YANG_DATA_FILE):
        self.yang_data_file = yang_data_file
        config_db_json = dict()
        with open(self.yang_data_file, "r") as yang_file:
            try:
                self.yang_data = json.load(yang_file)
                config_db_json = self.yang_parser.XlateYangToConfigDB(
                    yang_data=self.yang_data)
            except json.JSONDecodeError as e:
                print("Unable to parse Yang data file {} Error: {}".format(
                    yang_data_file, e))
        return config_db_json

    def validate_config_db_json(self, config_db_json):
        self.yang_parser.loadData(configdbJson=config_db_json)
        try:
            self.yang_parser.validate_data_tree()
            return True
        except sonic_yang.SonicYangException as e:
            print("yang data in {} is not valid".format(self.yang_data_file))
            return False

    def generate_config(self, yang_data_file=DEFAULT_YANG_DATA_FILE):
        config_db_json = self.get_config_db_from_yang_data(
            yang_data_file=yang_data_file)
        if self.validate_config_db_json(config_db_json):
            return config_db_json
        else:
            return {}
