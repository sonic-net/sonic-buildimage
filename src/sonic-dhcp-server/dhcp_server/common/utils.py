from swsscommon import swsscommon

DEFAULT_REDIS_HOST = "127.0.0.1"
DEFAULT_REDIS_PORT = 6379


class DhcpDbConnector(object):
    def __init__(self, redis_host=DEFAULT_REDIS_HOST, redis_port=DEFAULT_REDIS_PORT, redis_sock=None):
        if redis_sock is not None:
            self.redis_sock = redis_sock
            self.config_db = swsscommon.DBConnector(swsscommon.CONFIG_DB, redis_sock, 0)
            self.state_db = swsscommon.DBConnector(swsscommon.STATE_DB, redis_sock, 0)
        else:
            self.config_db = swsscommon.DBConnector(swsscommon.CONFIG_DB, redis_host, redis_port, 0)
            self.state_db = swsscommon.DBConnector(swsscommon.STATE_DB, redis_host, redis_port, 0)

    def get_config_db_table(self, table_name):
        """
        Get table from config_db.
        Args:
            table_name: Name of table want to get.
        Return:
            Table objects.
        """
        return _parse_table_to_dict(swsscommon.Table(self.config_db, table_name))

    def get_state_db_table(self, table_name):
        """
        Get table from state_db.
        Args:
            table_name: Name of table want to get.
        Return:
            Table objects.
        """
        return _parse_table_to_dict(swsscommon.Table(self.state_db, table_name))


def get_entry(table, entry_name):
    """
    Get dict entry from Table object.
    Args:
        table: Table object.
        entry_name: Name of entry.
    Returns:
        Dict of entry, sample:
            {
                "customized_options": "option60,option223",
                "gateway": "192.168.0.1",
                "lease_time": "900",
                "mode": "PORT",
                "netmask": "255.255.255.0",
                "state": "enabled"
            }
    """
    (_, entry) = table.get(entry_name)
    return dict(entry)


def terminate_proc(proc):
    """
    Terminate process, to make sure it exit successfully
    Args:
        proc: Process object in psutil
    """
    proc.terminate()
    proc.wait()


def merge_intervals(intervals):
    """
    Merge ip range intervals.
    Args:
        intervals: Ip ranges, may have overlaps, sample:
            [
                [IPv4Address('192.168.0.2'), IPv4Address('192.168.0.5')],
                [IPv4Address('192.168.0.3'), IPv4Address('192.168.0.6')],
                [IPv4Address('192.168.0.10'), IPv4Address('192.168.0.10')]
            ]
    Returns:
        Merged ip ranges, sample:
            [
                [IPv4Address('192.168.0.2'), IPv4Address('192.168.0.6')],
                [IPv4Address('192.168.0.10'), IPv4Address('192.168.0.10')]
            ]
    """
    intervals.sort(key=lambda x: x[0])
    ret = []
    for interval in intervals:
        if len(ret) == 0 or interval[0] > ret[-1][-1]:
            ret.append(interval)
        else:
            ret[-1][-1] = max(ret[-1][-1], interval[-1])
    return ret


def _parse_table_to_dict(table):
    ret = {}
    for key in table.getKeys():
        entry = get_entry(table, key)
        new_entry = {}

        for field, value in entry.items():
            # if value of this field is list, field end with @, so cannot found by hget
            if table.hget(key, field)[0]:
                new_entry[field] = value
            else:
                new_entry[field] = value.split(",")
        ret[key] = new_entry
    return ret
