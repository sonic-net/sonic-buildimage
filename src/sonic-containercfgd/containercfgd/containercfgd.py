from swsscommon.swsscommon import RestartWaiter
RestartWaiter.waitAdvancedBootDone()

import os
import re
import signal
import subprocess
import sys

from sonic_py_common import daemon_base, logger
from swsscommon.swsscommon import ConfigDBConnector

SYSLOG_IDENTIFIER = "containercfgd"
logger = logger.Logger(SYSLOG_IDENTIFIER)

# Table names
FEATURE_TABLE = 'FEATURE'
SYSLOG_CONFIG_FEATURE_TABLE = 'SYSLOG_CONFIG_FEATURE'

# Table field names
SYSLOG_RATE_LIMIT_INTERVAL = 'rate_limit_interval'
SYSLOG_RATE_LIMIT_BURST = 'rate_limit_burst'

# Container name
container_name = None
service_name = None


def run_command(command):
    """
    Utility function to run an shell command and return the output.
    :param command: Shell command string.
    :return: Output of the shell command.
    """
    return subprocess.check_output(command, text=True, stderr=subprocess.PIPE)


class ContainerConfigDaemon(daemon_base.DaemonBase):
    handlers = {}

    def __init__(self):
        super(ContainerConfigDaemon, self).__init__(SYSLOG_IDENTIFIER)

    def run(self):
        """Register config handlers and listen to CONFIG DB changes
        """
        config_db = ConfigDBConnector()
        config_db.connect(wait_for_init=True, retry_on=True)
        self.log_notice(f'Connected to CONFIG DB')
        for table_name, handler in self.handlers.items():
            config_db.subscribe(table_name, handler.handle_config)
        config_db.listen(init_data_handler=self.init_data_handler)

    def init_data_handler(self, init_data):
        """Handle initial data in CONFIG DB

        Args:
            init_data (dict): Initial data when first time connecting to CONFIG DB. {<table_name>: {<field_name>: <field_value>}}
        """
        for handler in self.handlers.values():
            handler.handle_init_data(init_data)

    @classmethod
    def register_handler(cls, table_name, object_type):
        """Register CONFIG DB handler

        Args:
            table_name (str): CONFIG DB table name
            object_type (class): Class of CONFIG DB handler
        """
        cls.handlers[table_name] = object_type()

    def signal_handler(self, sig, frame):
        if sig == signal.SIGHUP:
            self.log_info("ContainerCfgd: Caught SIGHUP - ignoring...")
        elif sig == signal.SIGINT:
            self.log_info("ContainerCfgd: Caught SIGINT - exiting...")
            sys.exit(128 + sig)
        elif sig == signal.SIGTERM:
            self.log_info("ContainerCfgd: Caught SIGTERM - exiting...")
            sys.exit(128 + sig)
        else:
            self.log_warning("ContainerCfgd: Caught unhandled signal '{}'".format(sig))


def config_handler(table_name):
    """Decorator to register CONFIG DB handler

    Args:
        table_name (str): CONFIG DB table name
    """
    def wrapper(object_type):
        ContainerConfigDaemon.register_handler(table_name, object_type)
        return object_type
    return wrapper


@config_handler(SYSLOG_CONFIG_FEATURE_TABLE)
class SyslogHandler:
    # syslog conf file path in docker
    SYSLOG_CONF_PATH = '/etc/rsyslog.conf'
    # temporary syslog conf file path in docker
    TMP_SYSLOG_CONF_PATH = '/tmp/rsyslog.conf'

    # Regular expressions to extract value from rsyslog.conf
    INTERVAL_PATTERN = '.*SystemLogRateLimitInterval\s+(\d+).*'
    BURST_PATTERN = '.*SystemLogRateLimitBurst\s+(\d+).*'

    def __init__(self):
        self.current_interval, self.current_burst = self.parse_syslog_conf()

    def handle_config(self, table, key, data):
        """Handle CONFIG DB change. Callback by ConfigDBConnector.

        Args:
            table (str): CONFIG DB table name
            key (str): Key of the changed entry
            data (dict): Data of the entry: {<field_name>: <field_value>}
        """
        try:
            if key != service_name:
                return
            self.update_syslog_config(data)
        except Exception as e:
            logger.log_error('Failed to config syslog for container {} with data {} - {}'.format(key, data, e))

    def handle_init_data(self, init_data):
        """Handle initial data in CONFIG DB. Callback by ConfigDBConnector.

        Args:
            init_data (dict): Initial data when first time connecting to CONFIG DB. {<table_name>: {<field_name>: <field_value>}}
        """
        if SYSLOG_CONFIG_FEATURE_TABLE in init_data:
            if service_name in init_data[SYSLOG_CONFIG_FEATURE_TABLE]:
                self.update_syslog_config(init_data[SYSLOG_CONFIG_FEATURE_TABLE][service_name])

    def update_syslog_config(self, data):
        """Parse existing syslog conf and apply new syslog conf.

        Args:
            data (dict): Data of the entry: {<field_name>: <field_value>}
        """
        new_interval = '0' if not data else data.get(SYSLOG_RATE_LIMIT_INTERVAL, '0')
        new_burst = '0' if not data else data.get(SYSLOG_RATE_LIMIT_BURST, '0')

        if new_interval == self.current_interval and new_burst == self.current_burst:
            logger.log_notice('Syslog rate limit configuration does not change, ignore it')
            return

        logger.log_notice(f'Configure syslog rate limit interval={new_interval}, burst={new_burst}')

        if os.path.exists(self.TMP_SYSLOG_CONF_PATH):
            os.remove(self.TMP_SYSLOG_CONF_PATH)
        with open(self.TMP_SYSLOG_CONF_PATH, 'w+') as f:
            json_args = f'{{"container_name": "{service_name}" }}'
            output = run_command(['sonic-cfggen', '-d', '-t', '/usr/share/sonic/templates/rsyslog-container.conf.j2', '-a', json_args])
            f.write(output)
        run_command(['cp', self.TMP_SYSLOG_CONF_PATH, self.SYSLOG_CONF_PATH])
        run_command(['supervisorctl', 'restart', 'rsyslogd'])
        self.current_interval = new_interval
        self.current_burst = new_burst

    def parse_syslog_conf(self):
        """Passe existing syslog conf and extract config values

        Returns:
            tuple: interval,burst
        """
        interval = '0'
        burst = '0'

        with open(self.SYSLOG_CONF_PATH, 'r') as f:
            content = f.read()
            pattern = re.compile(self.INTERVAL_PATTERN)
            for match in pattern.finditer(content):
                interval = match.group(1)
                break

            pattern = re.compile(self.BURST_PATTERN)
            for match in pattern.finditer(content):
                burst = match.group(1)
                break

        return interval, burst


def main():
    global container_name
    global service_name
    namespace_id = os.environ['NAMESPACE_ID']
    container_name = os.environ['CONTAINER_NAME']
    if not namespace_id:
        service_name = container_name
    else:
        service_name = container_name.rstrip(namespace_id)
    daemon = ContainerConfigDaemon()
    daemon.run()


if __name__ == '__main__':
    main()
