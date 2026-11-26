import libyang
import pytest


MODULE = "sonic-gnmi-client"


def config(certs):
    return {"sonic-gnmi-client:sonic-gnmi-client": {"GNMI_CLIENT": {"certs": certs}}}


@pytest.mark.parametrize("certs", [
    {"ca_crt": "/etc/sonic/credentials/gnmi_root.cer",
     "client_crt": "/etc/sonic/credentials/gnmi_client.cer",
     "client_key": "/etc/sonic/credentials/gnmi_client.key"},
    {"ca_crt": "/etc/sonic/site-pki/root.cer",
     "client_crt": "/etc/sonic/bmc-client/identity.cer",
     "client_key": "/etc/sonic/bmc-client/private.key"},
    {"ca_crt": "/etc/sonic/custom.cer"},
    {"client_crt": "/etc/sonic/client.cer"},
    {"client_key": "/etc/sonic/private.key"},
])
def test_optional_certificate_paths(yang_model, certs):
    yang_model.load_data(config(certs))


# Same file-name rules as GNMI|certs: an absolute path, certificates end in .cer, keys in .key.
@pytest.mark.parametrize("field, value", [
    (field, value)
    for field in ("ca_crt", "client_crt")
    for value in ("", "/", "relative.cer", "${CERT_DIR}/file.cer", "/etc/sonic/with space.cer",
                  "/etc/sonic/ca.pem", "/etc/sonic/ca.crt", "/etc/sonic/ca.key")
] + [
    ("client_key", value)
    for value in ("", "/", "relative.key", "/etc/sonic/with space.key",
                  "/etc/sonic/client.cer", "/etc/sonic/client.pem")
])
def test_invalid_certificate_path_rejected(yang_model, field, value):
    yang_model.load_data(config({field: value}), "Unsatisfied pattern")


@pytest.mark.parametrize("table, node", [
    ({"wrong-row": {"ca_crt": "/etc/sonic/ca.cer"}}, "wrong-row"),
    ({"certs": {"unknown": "/etc/sonic/ca.cer"}}, "unknown"),
])
def test_unknown_row_or_field_rejected(yang_model, table, node):
    yang_model.load_data({"sonic-gnmi-client:sonic-gnmi-client": {"GNMI_CLIENT": table}},
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
