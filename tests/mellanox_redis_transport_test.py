"""Check the three local Mellanox database transport expressions."""
import ast
from pathlib import Path
from types import SimpleNamespace

import pytest

ROOT = Path(__file__).resolve().parents[1]
FILES = (
    "platform/mellanox/mlnx-platform-api/sonic_platform/db_table_helper.py",
    "device/mellanox/x86_64-mlnx_msn2700-r0/plugins/sfputil.py",
)


def calls():
    for name in FILES:
        for node in ast.walk(ast.parse((ROOT / name).read_text())):
            if isinstance(node, ast.Call) and ast.unparse(node.func) in (
                    "DBConnector", "swsscommon.DBConnector"):
                yield pytest.param(node, id=f"{name}:{node.lineno}")


@pytest.mark.parametrize("call", list(calls()))
@pytest.mark.parametrize("namespace", ["", "asic0"])
@pytest.mark.parametrize("port", [0, 6379, 6400, None])
def test_local_transport_is_explicit_despite_tcp_metadata(call, namespace, port):
    captured = []
    names = {"namespace": namespace, "STATE_DB": "STATE_DB",
             "APPL_DB": "APPL_DB", "REDIS_TIMEOUT_USECS": 0}

    def evaluate(node):
        return eval(compile(ast.Expression(node), "caller", "eval"), names)

    database = evaluate(call.args[0])
    target_namespace = namespace if len(call.args) > 3 else ""

    def get_port(actual_database, actual_namespace):
        assert (actual_database, actual_namespace) == (database, target_namespace)
        if port is None:
            raise RuntimeError("database metadata unavailable")
        return port

    names["DBConnector"] = lambda *args: captured.append(args)
    names["SonicDBConfig"] = SimpleNamespace(getDbPort=get_port)
    names["swsscommon"] = SimpleNamespace(
        DBConnector=names["DBConnector"], SonicDBConfig=names["SonicDBConfig"])
    evaluate(call)
    assert len(captured) == 1
    assert captured[0][:3] == (database, 0, False)
    if len(call.args) > 3:
        assert captured[0][3] == target_namespace
