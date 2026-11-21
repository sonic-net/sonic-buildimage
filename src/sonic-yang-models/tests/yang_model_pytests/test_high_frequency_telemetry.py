"""GCU and the other removers find a port's or buffer pool's dependents through
libyang backlinks, so HIGH_FREQUENCY_TELEMETRY_GROUP object_names must be a
backlink of both PORT and BUFFER_POOL names."""

import pytest

OBJECT_NAMES = ('/sonic-high-frequency-telemetry:sonic-high-frequency-telemetry'
                '/HIGH_FREQUENCY_TELEMETRY_GROUP/HIGH_FREQUENCY_TELEMETRY_GROUP_LIST/object_names')


@pytest.mark.parametrize('target', [
    '/sonic-port:sonic-port/PORT/PORT_LIST/name',
    '/sonic-buffer-pool:sonic-buffer-pool/BUFFER_POOL/BUFFER_POOL_LIST/name',
], ids=['PORT', 'BUFFER_POOL'])
def test_object_names_is_a_backlink(yang_model, target):
    assert OBJECT_NAMES in yang_model.ctx.find_backlinks_paths(target)
