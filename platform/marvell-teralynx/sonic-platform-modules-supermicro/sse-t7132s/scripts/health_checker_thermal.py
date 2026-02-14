#!/usr/bin/python3

'''
This script is for healthd user_defined_checkers.
'''

from swsscommon.swsscommon import SonicV2Connector
from natsort import natsorted

TEMPER_TABLE_NAME = 'TEMPERATURE_INFO'
TEMPER_FIELD_NAME = 'temperature'
TIMESTAMP_FIELD_NAME = 'timestamp'
HIGH_THRESH_FIELD_NAME = 'high_threshold'
LOW_THRESH_FIELD_NAME = 'low_threshold'
CRIT_HIGH_THRESH_FIELD_NAME = 'critical_high_threshold'
CRIT_LOW_THRESH_FIELD_NAME = 'critical_low_threshold'
WARNING_STATUS_FIELD_NAME = 'warning_status'

class TemperCheck(object):
    def __init__(self):
        self.db = SonicV2Connector(host="127.0.0.1")
        self.db.connect(self.db.STATE_DB)

    def show(self):
        keys = self.db.keys(self.db.STATE_DB, TEMPER_TABLE_NAME + '*')
        if not keys:
            #print('Thermal Not detected\n')
            return

        print("Thermal")
        for key in natsorted(keys):
            key_list = key.split('|')
            if len(key_list) != 2: # error data in DB, log it and ignore
                #print('Warn: Invalid key in table {}: {}'.format(TEMPER_TABLE_NAME, key))
                continue

            name = key_list[1]
            data_dict = self.db.get_all(self.db.STATE_DB, key)
            if data_dict[WARNING_STATUS_FIELD_NAME] == 'False':
                print("{}:OK".format(name))
            else:
                temperature = data_dict[TEMPER_FIELD_NAME]
                high_threshold = data_dict[HIGH_THRESH_FIELD_NAME]
                low_threshold = data_dict[LOW_THRESH_FIELD_NAME]
                print("{}:{} status is warning about temperature {} (threshold high {} low {})".
                      format(name, name, temperature, high_threshold, low_threshold))

if __name__ == "__main__":
    temperCheck = TemperCheck()
    temperCheck.show()
