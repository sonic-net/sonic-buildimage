import libyang
import pytest


MODULE = "sonic-bmc-gnoi"


def config(certs):
    return {"sonic-bmc-gnoi:sonic-bmc-gnoi": {"BMC_GNOI": {"certs": certs}}}


@pytest.mark.parametrize("certs", [
    {"ca_crt": "/etc/sonic/bmc-link/ca.crt",
     "client_crt": "/etc/sonic/bmc-link/client.crt",
     "client_key": "/etc/sonic/bmc-link/client.key"},
    {"ca_crt": "/etc/sonic/site-pki/root.pem",
     "client_crt": "/etc/sonic/bmc-client/identity.pem",
     "client_key": "/etc/sonic/bmc-client/private.key"},
    {"ca_crt": "/etc/sonic/custom.pem"},
    {"client_crt": "/etc/sonic/client.pem"},
    {"client_key": "/etc/sonic/private.pem"},
])
def test_optional_certificate_paths(yang_model, certs):
    yang_model.load_data(config(certs))


@pytest.mark.parametrize("field", ["ca_crt", "client_crt", "client_key"])
@pytest.mark.parametrize("value", [
    "", "/", "relative.pem", "${CERT_DIR}/file.pem", "/etc/sonic/with space.pem",
])
def test_invalid_certificate_path_rejected(yang_model, field, value):
    yang_model.load_data(config({field: value}), "Unsatisfied pattern")


@pytest.mark.parametrize("table, node", [
    ({"wrong-row": {"ca_crt": "/etc/sonic/ca.pem"}}, "wrong-row"),
    ({"certs": {"unknown": "/etc/sonic/ca.pem"}}, "unknown"),
])
def test_unknown_row_or_field_rejected(yang_model, table, node):
    yang_model.load_data({"sonic-bmc-gnoi:sonic-bmc-gnoi": {"BMC_GNOI": table}},
                         'Node "{}" not found'.format(node))


def test_no_leaf_declares_a_default(yang_model):
    module = yang_model.ctx.get_module(MODULE)
    containers = (libyang.SNode.CONTAINER,)
    top = next(iter(module.children(types=containers)))
    table = next(iter(top.children(types=containers)))
    row = next(iter(table.children(types=containers)))
    leaves = list(row.children(types=(libyang.SNode.LEAF,)))
    assert sorted(leaf.name() for leaf in leaves) == ["ca_crt", "client_crt", "client_key"]
    assert [leaf.default() for leaf in leaves] == [None, None, None]
