import os
import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

modules_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(modules_path, 'src'))

class TestPortUtil:
    def test_get_vlan_interface_oid_map(self):
        db = mock.MagicMock()
        db.get_all = mock.MagicMock()
        db.get_all.return_value = {}

        from swsssdk.port_util import get_vlan_interface_oid_map
        assert not get_vlan_interface_oid_map(db, True)
