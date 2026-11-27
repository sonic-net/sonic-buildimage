#!/usr/bin/env python3
"""Read-only native connector probe, streamed into the protected container.

Only reports the authenticated username. Never prints credentials or Redis
values. This verifies a cooperating connector, not mandatory client identity.
"""
import json
import stat
import sys
from pathlib import Path
from swsscommon import swsscommon as sw

user = sys.argv[1]
config = json.loads(Path('/var/run/redis/sonic-db/database_config.json').read_text())
instance = config['INSTANCES']['redis']
assert instance['username'] == user, 'native configuration user differs'
secret = Path(instance['password_file'])
assert stat.S_IMODE(secret.stat().st_mode) == 0o600, 'credential permissions differ'
client = sw.DBConnector('CONFIG_DB', 1000, False)
reply = sw.RedisReply(client, 'CLIENT INFO')
info = sw.RedisReply.to_string(reply.getContext(), '')
fields = dict(part.split('=', 1) for part in info.split() if '=' in part)
assert fields.get('user') == user, 'native connection identity differs'
# Old connectors unconditionally issue CONFIG SET here. They must fail this
# readiness check rather than receive Redis administration permissions.
config_client = sw.ConfigDBConnector(use_unix_socket_path=True)
config_client.connect(wait_for_init=False, retry_on=False)
print(json.dumps({'user': user}))
