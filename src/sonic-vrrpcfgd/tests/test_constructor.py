import socket
import pytest
from unittest.mock import MagicMock, NonCallableMagicMock, patch

swsscommon_module_mock = MagicMock(ConfigDBConnector = NonCallableMagicMock)
# because can¡¯t use dotted names directly in a call, have to create a dictionary and unpack it using **:
mockmapping = {'swsscommon.swsscommon': swsscommon_module_mock}

with patch.dict('sys.modules', **mockmapping):
    from vrrpcfgd.vrrpcfgd import CachedDataWithOp

def test_data_with_op():
    data = CachedDataWithOp()
    assert(data.data is None)
    assert(data.op == CachedDataWithOp.OP_NONE)
    data = CachedDataWithOp(10, CachedDataWithOp.OP_ADD)
    assert(data.data == 10)
    assert(data.op == CachedDataWithOp.OP_ADD)

