# Adapted from gs1571 f7b6ef84dfb0ff8ffcdeb032cd17867c31298baa.
"""Read one scoped ConfigDB snapshot in an independently bounded process.

The native SDK does not expose response timeouts through ConfigDBConnector.
The parent owns this process's lifetime; it never shares the BGP client's Redis
connection and never treats a timeout/partial JSON as an empty configuration.
"""

import json

from sonic_isis import SNAPSHOT_TABLES as TABLES


def capture(connector):
    from sonic_isis import capture as atomic_capture
    return {name: {'|'.join(key) if isinstance(key, tuple) else key: value for key, value in rows.items()}
            for name, rows in atomic_capture(connector).items()}


def main():
    from swsscommon.swsscommon import ConfigDBConnector
    connector = ConfigDBConnector()
    connector.connect(wait_for_init=False, retry_on=False)
    print(json.dumps(capture(connector), allow_nan=False))


if __name__ == '__main__':
    main()
