"""Read one scoped ConfigDB snapshot in an independently bounded process.

The native SDK does not expose response timeouts through ConfigDBConnector.
The parent owns this process's lifetime; it never shares the BGP client's Redis
connection and never treats a timeout/partial JSON as an empty configuration.
"""

import json

from .isis_config import MANAGED_TABLES, ROUTED_TABLES, key_parts


TABLES = MANAGED_TABLES + ROUTED_TABLES + ('PORT', 'PORTCHANNEL', 'VLAN', 'VRF')


def capture(connector):
    result = {}
    for name in TABLES:
        rows = connector.get_table(name)
        if not isinstance(rows, dict):
            raise ValueError('Invalid scoped ConfigDB table')
        normalized = {}
        for key, value in rows.items():
            serialized = '|'.join(key_parts(key))
            if serialized in normalized or not isinstance(value, dict):
                raise ValueError('Invalid scoped ConfigDB row')
            normalized[serialized] = value
        result[name] = normalized
    return result


def main():
    from swsscommon.swsscommon import ConfigDBConnector
    connector = ConfigDBConnector()
    connector.connect(wait_for_init=False, retry_on=False)
    print(json.dumps(capture(connector), allow_nan=False))


if __name__ == '__main__':
    main()
