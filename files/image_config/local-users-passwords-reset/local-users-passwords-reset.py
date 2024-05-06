#!/usr/bin/env python

import syslog
from swsscommon.swsscommon import ConfigDBConnector, DBConnector
from swsscommon import swsscommon


STATE_DB = "STATE_DB"


def get_platform_local_users_passwords_reset():
    try:
        from sonic_platform.local_users_passwords_reset import LocalUsersConfigurationReset
        local_users_password_reset_class = LocalUsersConfigurationReset()
    except ImportError:
        syslog.syslog(syslog.LOG_WARNING, "LocalUsersConfigurationReset: sonic_platform package not installed. Unable to find platform local users passwords reset implementation")
        raise Exception('Local users passwords reset implementation is not defined')

    return local_users_password_reset_class


class LocalUsersConfigurationResetService:
    def __init__(self):
        state_db_conn = DBConnector(STATE_DB, 0)
        # Wait if the Warm/Fast boot is in progress
        if swsscommon.RestartWaiter.isAdvancedBootInProgress(state_db_conn):
            swsscommon.RestartWaiter.waitAdvancedBootDone()

        self.config_db = ConfigDBConnector()
        self.config_db.connect(wait_for_init=True, retry_on=True)
        syslog.syslog(syslog.LOG_INFO, 'ConfigDB connect success')

    def get_feature_state(self):
        '''
        Check if the feature is enabled by reading the redis table
        '''
        table = self.config_db.get_table(swsscommon.CFG_LOCAL_USERS_PASSWORDS_RESET)
        if table:
            state = table.get('global', {}).get('state')
            return True if state == 'enabled' else False

        return False

    def start(self):
        '''
        If the feature is enabled then reset the password's using the platform
        specific implementation
        '''
        local_users_password_reset = get_platform_local_users_passwords_reset()
        feature_enabled = self.get_feature_state()
        syslog.syslog(syslog.LOG_INFO, 'Feature is {}'.format('enabled' if feature_enabled else 'disabled'))
        should_trigger = local_users_password_reset.should_trigger()
        if should_trigger and feature_enabled:
            local_users_password_reset.start()


def main():
    LocalUsersConfigurationResetService().start()


if __name__ == "__main__":
    main()
