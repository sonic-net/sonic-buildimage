import sys

import pytest
from click.testing import CliRunner

import utilities_common.cli as clicommon

sys.path.append('../cli/config/plugins/')
import dhcp_server


IPV4_COMMAND = dhcp_server.dhcp_server.commands["ipv4"]
MATCH_COMMAND = IPV4_COMMAND.commands["match"]
BINDING_COMMAND = IPV4_COMMAND.commands["binding"]
RANGE_COMMAND = IPV4_COMMAND.commands["range"]


@pytest.fixture
def prefix_db(mock_db):
    for table in ("DHCP_SERVER_IPV4_BINDING", "DHCP_SERVER_IPV4_PORT"):
        for key in mock_db.keys("CONFIG_DB", table + "|Vlan100|*"):
            mock_db.delete("CONFIG_DB", key)
    mock_db.hmset("CONFIG_DB", "VLAN_INTERFACE|Vlan100|100.1.2.1/24", {})
    return mock_db


@pytest.mark.parametrize("mode", ["PORT", "MATCH"])
@pytest.mark.parametrize("operation", [
    "add", "add-range", "same-entry", "binding-update", "range-update", "enable", "mode-update"
])
def test_multiple_served_prefixes_rejected_only_for_match(prefix_db, mode, operation):
    db = prefix_db
    table = "DHCP_SERVER_IPV4_BINDING" if mode == "MATCH" else "DHCP_SERVER_IPV4_PORT"
    first = "first" if mode == "MATCH" else "Ethernet2"
    second = "second" if mode == "MATCH" else "Ethernet4"
    first_entry = {"ips@": "100.1.1.30"}
    second_entry = {"ips@": "100.1.2.30"}
    if mode == "MATCH":
        first_entry["matches@"] = "vendor-a"
        second_entry["matches@"] = "vendor-b"
    db.hmset("CONFIG_DB", "{}|Vlan100|{}".format(table, first), first_entry)
    db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode", mode)
    if operation in ("add", "add-range"):
        command = BINDING_COMMAND.commands["add"] if mode == "MATCH" else IPV4_COMMAND.commands["bind"]
        args = ["Vlan100", second]
        args += ["--range=range5"] if operation == "add-range" else ["100.1.2.30"]
        if mode == "MATCH":
            args += ["--match=vendor-b"]
    elif operation == "same-entry":
        command = BINDING_COMMAND.commands["update"] if mode == "MATCH" else IPV4_COMMAND.commands["bind"]
        args = ["Vlan100", first, "100.1.1.30,100.1.2.30"]
    elif operation == "binding-update":
        second_entry["ips@"] = "100.1.1.40"
        db.hmset("CONFIG_DB", "{}|Vlan100|{}".format(table, second), second_entry)
        command = BINDING_COMMAND.commands["update"] if mode == "MATCH" else IPV4_COMMAND.commands["bind"]
        args = ["Vlan100", second, "100.1.2.30"]
    elif operation == "range-update":
        second_entry.pop("ips@")
        second_entry["ranges@"] = "range3"
        db.hmset("CONFIG_DB", "{}|Vlan100|{}".format(table, second), second_entry)
        command = RANGE_COMMAND.commands["update"]
        args = ["range3", "100.1.2.30"]
    else:
        db.hmset("CONFIG_DB", "{}|Vlan100|{}".format(table, second), second_entry)
        if operation == "enable":
            db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "state", "disabled")
            command, args = IPV4_COMMAND.commands["enable"], ["Vlan100"]
        else:
            db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode", "PORT" if mode == "MATCH" else "MATCH")
            command, args = IPV4_COMMAND.commands["update"], ["Vlan100", "--mode=" + mode]
    before = {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")}

    result = invoke(db, command, args)

    if mode == "MATCH":
        assert result.exit_code == 2, result.output
        assert "DHCP pools on Vlan100 cannot span multiple IPv4 prefixes" in result.output
        assert {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")} == before
    else:
        assert result.exit_code == 0, result.output
        assert {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")} != before


@pytest.mark.parametrize("mode", ["PORT", "MATCH"])
@pytest.mark.parametrize("wide_prefix", [False, True])
def test_single_served_prefix_allowed(prefix_db, mode, wide_prefix):
    db = prefix_db
    first_ip, second_ip = "100.1.1.30", "100.1.1.40"
    if wide_prefix:
        db.delete("CONFIG_DB", "VLAN_INTERFACE|Vlan100|100.1.1.1/24")
        db.hmset("CONFIG_DB", "VLAN_INTERFACE|Vlan100|172.17.31.254/19", {})
        first_ip, second_ip = "172.17.0.1", "172.17.26.1"
    db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode", mode)
    if mode == "MATCH":
        db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|first",
                 {"matches@": "vendor-a", "ips@": first_ip})
        command = BINDING_COMMAND.commands["add"]
        args = ["Vlan100", "second", "--match=vendor-b", second_ip]
    else:
        db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet2", {"ips@": first_ip})
        command = IPV4_COMMAND.commands["bind"]
        args = ["Vlan100", "Ethernet4", second_ip]

    result = invoke(db, command, args)

    assert result.exit_code == 0, result.output


def invoke(mock_db, command, args):
    db = clicommon.Db()
    db.db = mock_db
    return CliRunner().invoke(command, args, obj=db)


@pytest.fixture
def binding_with_deleted_range(prefix_db):
    db = prefix_db
    db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range3", {"range@": "100.1.1.10,100.1.1.12"})
    db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|surviving",
             {"matches@": "vendor-a", "ranges@": "range1,range3"})
    result = invoke(db, RANGE_COMMAND.commands["del"], ["range1", "--force"])
    assert result.exit_code == 0, result.output
    return db


@pytest.mark.parametrize("operation", [
    "add", "binding-update", "match-update", "range-update", "enable", "mode-update"
])
@pytest.mark.parametrize("conflict", ["equal-specificity", "partial-pool"])
def test_surviving_ranges_participate_in_conflict_checks(binding_with_deleted_range, operation, conflict):
    db = binding_with_deleted_range
    match = "vendor-a" if conflict == "equal-specificity" else "vendor-b"
    ips = "100.1.1.40" if conflict == "equal-specificity" else "100.1.1.11,100.1.1.13"
    entry = {"matches@": match, "ips@": ips}
    if operation == "add":
        command = BINDING_COMMAND.commands["add"]
        args = ["Vlan100", "other", "--match=" + match, ips]
    else:
        db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|other", entry)
        if operation == "binding-update":
            db.set("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|other", "ips@", "100.1.1.50")
            command, args = BINDING_COMMAND.commands["update"], ["Vlan100", "other", ips]
        elif operation == "match-update":
            db.set("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|other", "matches@", "vendor-b")
            value = "MAIA-A" if conflict == "equal-specificity" else "MAIA-C"
            command, args = MATCH_COMMAND.commands["update"], ["vendor-b", "--value=" + value]
        elif operation == "range-update":
            db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|other-range", {"range@": "100.1.1.50"})
            db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|other",
                     {"matches@": match, "ranges@": "other-range"})
            endpoints = ["100.1.1.40"] if conflict == "equal-specificity" else ["100.1.1.11", "100.1.1.13"]
            command, args = RANGE_COMMAND.commands["update"], ["other-range"] + endpoints
        elif operation == "enable":
            db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode", "MATCH")
            db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "state", "disabled")
            command, args = IPV4_COMMAND.commands["enable"], ["Vlan100"]
        else:
            command, args = IPV4_COMMAND.commands["update"], ["Vlan100", "--mode=MATCH"]
    before = {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")}

    result = invoke(db, command, args)

    assert result.exit_code == 2, result.output
    error = "overlap with equal specificity" if conflict == "equal-specificity" else "partially overlapping pools"
    assert error in result.output
    assert {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")} == before


@pytest.mark.parametrize("all_ranges_missing", [False, True])
def test_deleted_ranges_allow_nonconflicting_changes(binding_with_deleted_range, all_ranges_missing):
    db = binding_with_deleted_range
    if all_ranges_missing:
        db.delete("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range3")
    match = "vendor-a" if all_ranges_missing else "vendor-b"

    result = invoke(db, BINDING_COMMAND.commands["add"], ["Vlan100", "other", "--match=" + match, "100.1.1.40"])

    assert result.exit_code == 0, result.output


@pytest.mark.parametrize("start,end,error", [
    ("100.1.1.40", "100.1.1.41", "partially overlapping pools"),
    ("100.1.3.10", "100.1.3.11", "is not in any subnet"),
])
def test_surviving_range_update_remains_guarded(binding_with_deleted_range, start, end, error):
    db = binding_with_deleted_range
    db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|other",
             {"matches@": "vendor-b", "ips@": "100.1.1.40"})
    before = {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")}

    result = invoke(db, RANGE_COMMAND.commands["update"], ["range3", start, end])

    assert result.exit_code == 2, result.output
    assert error in result.output
    assert {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")} == before


@pytest.mark.parametrize("operation", ["add", "update"])
def test_candidate_with_missing_range_remains_strict(binding_with_deleted_range, operation):
    db = binding_with_deleted_range
    before = {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")}
    args = ["Vlan100", "other", "--match=vendor-b", "--range=range1,range3"] if operation == "add" else [
        "Vlan100", "surviving", "--match=vendor-b"]

    result = invoke(db, BINDING_COMMAND.commands[operation], args)

    assert result.exit_code == 2, result.output
    assert "Range range1 does not exist" in result.output
    assert {key: dict(db.get_all("CONFIG_DB", key)) for key in db.keys("CONFIG_DB", "*")} == before


@pytest.fixture
def dhcp_vlan200(mock_db):
    mock_db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan200",
                  dict(mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100")))


class TestConfigDHCPServerMatch:
    def test_add_interface_in_match_mode(self, mock_db):
        result = invoke(
            mock_db,
            IPV4_COMMAND.commands["add"],
            [
                "Vlan200",
                "--mode=MATCH",
                "--lease_time=1000",
                "--gateway=100.1.2.2",
                "--netmask=255.255.255.0",
            ],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan200", "mode") == "MATCH"

    def test_update_disabled_interface_to_match_mode(self, mock_db):
        result = invoke(mock_db, IPV4_COMMAND.commands["update"], ["Vlan300", "--mode=MATCH"])

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan300", "mode") == "MATCH"

    def test_update_enabled_interface_to_match_mode(self, mock_db):
        result = invoke(mock_db, IPV4_COMMAND.commands["update"], ["Vlan100", "--mode=MATCH"])

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode") == "MATCH"

    def test_update_enabled_interface_to_match_mode_without_binding(self, mock_db):
        mock_db.delete("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-a")
        mock_db.delete("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|fallback")

        result = invoke(mock_db, IPV4_COMMAND.commands["update"], ["Vlan100", "--mode=MATCH"])

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode") == "MATCH"

    def test_enable_match_interface_with_valid_bindings(self, mock_db):
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode", "MATCH")
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "state", "disabled")

        result = invoke(mock_db, IPV4_COMMAND.commands["enable"], ["Vlan100"])

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "state") == "enabled"

    def test_enable_match_interface_without_binding(self, mock_db):
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan300", "mode", "MATCH")

        result = invoke(mock_db, IPV4_COMMAND.commands["enable"], ["Vlan300"])

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan300", "state") == "enabled"

    def test_delete_interface_referenced_by_match_binding(self, mock_db):
        for key in list(mock_db.keys("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|*")):
            mock_db.delete("CONFIG_DB", key)

        result = invoke(mock_db, IPV4_COMMAND.commands["del"], ["Vlan100"])

        assert result.exit_code == 2
        assert "has match bindings" in result.output

    def test_match_add(self, mock_db):
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["vendor-c", "--type", "option60", "--value", "MAIA-C"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|vendor-c") == {
            "type": "option60",
            "value": "MAIA-C",
        }

    def test_match_add_name_with_list_separator(self, mock_db):
        """A ',' in a match name would be read back as two separate references.

        DHCP_SERVER_IPV4_BINDING/matches is stored as a single comma-joined
        'matches@' string, so such a name cannot round-trip through Config DB.
        """
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["vendor,a", "--type", "option60", "--value", "MAIA-A"],
        )

        assert result.exit_code == 2
        assert "Match name must not contain" in result.output
        assert not mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|vendor,a")

    def test_match_add_name_with_key_separator(self, mock_db):
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["rack|vendor", "--type", "option60", "--value", "MAIA-A"],
        )

        assert result.exit_code == 2
        assert "Match name must not contain" in result.output

    def test_match_add_name_with_other_punctuation(self, mock_db):
        """Non-reserved punctuation inside a valid key remains allowed."""
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["rack-1.vendor_a:x", "--type", "option60", "--value", "MAIA-A"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|rack-1.vendor_a:x")

    def test_match_add_value_with_separators(self, mock_db):
        """A match value is neither a key nor a leaf-list member, so it keeps
        accepting both separators - Option 60 strings are arbitrary bytes."""
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["odd-vendor", "--type", "option60", "--value", "MAIA,A|B"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|odd-vendor", "value") == "MAIA,A|B"

    def test_binding_add_name_with_key_separator(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia|b", "100.1.1.30", "--match", "vendor-b,port-etp2"],
        )

        assert result.exit_code == 2
        assert "Binding name must not contain" in result.output

    def test_binding_add_name_with_comma_allowed(self, mock_db):
        """Binding names are not leaf-list members, so commas remain valid."""
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia,b", "100.1.1.30", "--match", "vendor-b,port-etp2"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia,b")

    @pytest.mark.parametrize("kind", ["match", "binding"])
    @pytest.mark.parametrize("operation", ["add", "update"])
    @pytest.mark.parametrize("name", [
        pytest.param(" padded", id="leading-space"),
        pytest.param("padded ", id="trailing-space"),
        pytest.param("\tpadded", id="tab"),
        pytest.param("padded\n", id="LF"),
        pytest.param("\u00a0padded", id="NBSP"),
        pytest.param("padded\u2028", id="line-separator"),
        pytest.param("\u2029padded", id="paragraph-separator"),
        pytest.param("padded\x1b", id="non-whitespace-control"),
    ])
    def test_key_edge_whitespace_rejected(self, mock_db, dhcp_vlan200, kind, operation, name):
        if kind == "match":
            command = MATCH_COMMAND.commands[operation]
            key = "DHCP_SERVER_IPV4_MATCH|" + name
            before = {"type": "option60", "value": "OLD"}
            args = [name, "--value", "NEW"]
            if operation == "add":
                args += ["--type", "option60"]
        else:
            command = BINDING_COMMAND.commands[operation]
            key = "DHCP_SERVER_IPV4_BINDING|Vlan200|" + name
            before = {"matches@": "vendor-a", "ips@": "100.1.2.20"}
            args = ["Vlan200", name, "100.1.2.30", "--match", "vendor-a"]
        if operation == "update":
            mock_db.hmset("CONFIG_DB", key, dict(before))

        result = invoke(mock_db, command, args)

        assert result.exit_code == 2, result.output
        assert "{} name must not have leading or trailing whitespace".format(kind.capitalize()) in result.output
        if operation == "update":
            assert mock_db.get_all("CONFIG_DB", key) == before
        else:
            assert not mock_db.exists("CONFIG_DB", key)

    @pytest.mark.parametrize("operation", ["add", "update"])
    @pytest.mark.parametrize("name", [" padded", "padded "])
    def test_match_reference_edge_whitespace_rejected(self, mock_db, dhcp_vlan200, operation, name):
        mock_db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|" + name, {
            "type": "option60", "value": "MAIA",
        })
        key = "DHCP_SERVER_IPV4_BINDING|Vlan200|new-binding"
        before = {"matches@": "vendor-a", "ips@": "100.1.2.20"}
        if operation == "update":
            mock_db.hmset("CONFIG_DB", key, dict(before))

        result = invoke(
            mock_db, BINDING_COMMAND.commands[operation],
            ["Vlan200", "new-binding", "100.1.2.30", "--match", name],
        )

        assert result.exit_code == 2, result.output
        assert "Match name must not have leading or trailing whitespace" in result.output
        if operation == "update":
            assert mock_db.get_all("CONFIG_DB", key) == before
        else:
            assert not mock_db.exists("CONFIG_DB", key)

    @pytest.mark.parametrize("kind", ["match", "binding"])
    @pytest.mark.parametrize("name", ["a", "internal space", "two  spaces", "rack-1.site_a:x", "\u00e9"])
    def test_valid_key_names_add_and_update(self, mock_db, dhcp_vlan200, kind, name):
        if kind == "match":
            group = MATCH_COMMAND
            key = "DHCP_SERVER_IPV4_MATCH|" + name
            add_args = [name, "--type", "option60", "--value", "OLD"]
            update_args = [name, "--value", "NEW"]
            expected = {"type": "option60", "value": "NEW"}
        else:
            group = BINDING_COMMAND
            key = "DHCP_SERVER_IPV4_BINDING|Vlan200|" + name
            add_args = ["Vlan200", name, "100.1.2.20", "--match", "vendor-a"]
            update_args = ["Vlan200", name, "100.1.2.30"]
            expected = {"matches@": "vendor-a", "ips@": "100.1.2.30"}

        added = invoke(mock_db, group.commands["add"], add_args)
        assert added.exit_code == 0, added.output
        updated = invoke(mock_db, group.commands["update"], update_args)
        assert updated.exit_code == 0, updated.output
        assert mock_db.get_all("CONFIG_DB", key) == expected

    @pytest.mark.parametrize("value", [" MAIA A ", "\u00a0MAIA\u00a0"])
    def test_match_value_edge_whitespace_preserved(self, mock_db, value):
        key = "DHCP_SERVER_IPV4_MATCH|new-vendor"
        added = invoke(
            mock_db, MATCH_COMMAND.commands["add"],
            ["new-vendor", "--type", "option60", "--value", value],
        )
        assert added.exit_code == 0, added.output
        assert mock_db.get("CONFIG_DB", key, "value") == value

        updated_value = value + "\t"
        updated = invoke(mock_db, MATCH_COMMAND.commands["update"], ["new-vendor", "--value", updated_value])
        assert updated.exit_code == 0, updated.output
        assert mock_db.get("CONFIG_DB", key, "value") == updated_value

    @pytest.mark.parametrize("kind", ["match", "binding"])
    def test_legacy_key_edge_whitespace_can_be_deleted(self, mock_db, kind):
        if kind == "match":
            group = MATCH_COMMAND
            key = "DHCP_SERVER_IPV4_MATCH| legacy "
            entry = {"type": "option60", "value": "MAIA"}
            args = [" legacy "]
        else:
            group = BINDING_COMMAND
            key = "DHCP_SERVER_IPV4_BINDING|Vlan200| legacy "
            entry = {"matches@": "vendor-a", "ips@": "100.1.2.20"}
            args = ["Vlan200", " legacy "]
        mock_db.hmset("CONFIG_DB", key, entry)

        result = invoke(mock_db, group.commands["del"], args)

        assert result.exit_code == 0, result.output
        assert not mock_db.exists("CONFIG_DB", key)

    def test_match_add_circuit_id_unknown_port(self, mock_db):
        """The model requires a circuit_id to name a real port even with no binding."""
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["bad-port", "--type", "circuit_id", "--value", "etp99"],
        )

        assert result.exit_code == 2
        assert "does not exist" in result.output
        assert not mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|bad-port")

    def test_match_add_circuit_id_port_outside_any_vlan(self, mock_db):
        """A standalone match has no VLAN context, so only existence is checked.

        etp5 is a real port that is not a member of any DHCP VLAN. Membership is a
        binding-time rule, and enforcing it here would reject a match that is
        perfectly legal on its own.
        """
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["lone-port", "--type", "circuit_id", "--value", "etp5"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|lone-port") == {
            "type": "circuit_id",
            "value": "etp5",
        }

    def test_match_update_circuit_id_unknown_port(self, mock_db):
        """unused-match is referenced by no binding, so nothing else would catch this."""
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["update"],
            ["unused-match", "--type", "circuit_id", "--value", "etp99"],
        )

        assert result.exit_code == 2
        assert "does not exist" in result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|unused-match", "type") == "option60"

    def test_range_update_breaking_binding_rejected(self, mock_db):
        """range3 is bound by Vlan100/fallback; moving it off-subnet must be refused.

        dhcp_cfggen would silently drop the binding's pool.
        """
        result = invoke(
            mock_db,
            RANGE_COMMAND.commands["update"],
            ["range3", "100.1.2.50"],
        )

        assert result.exit_code == 2
        assert "not in any subnet of vlan Vlan100" in result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range3", "range@") == "100.1.1.10"

    def test_range_update_within_subnet_allowed(self, mock_db):
        result = invoke(
            mock_db,
            RANGE_COMMAND.commands["update"],
            ["range3", "100.1.1.50", "100.1.1.60"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range3", "range@") == (
            "100.1.1.50,100.1.1.60"
        )

    def test_range_update_with_only_port_consumers_not_guarded(self, mock_db):
        """Documents a deliberate boundary, not desired behaviour.

        range1 is referenced by DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet7 but by no
        binding. DHCP_SERVER_IPV4_PORT consumers are validated nowhere today, and
        guarding them would change behaviour that predates this series, so the
        update is still allowed. If that pre-existing gap is closed later, this
        test should flip to a rejection.
        """
        result = invoke(
            mock_db,
            RANGE_COMMAND.commands["update"],
            ["range1", "100.1.2.50"],
        )

        assert result.exit_code == 0, result.output

    def test_match_add_invalid_type(self, mock_db):
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["client-mac", "--type", "mac", "--value", "00:11:22:33:44:55"],
        )

        assert result.exit_code == 2
        assert "Only match types" in result.output

    def test_match_add_empty_value(self, mock_db):
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["empty-match", "--type", "option60", "--value", ""],
        )

        assert result.exit_code == 2
        assert "Match value must be between 1 and 255 characters" in result.output

    def test_match_add_overlength_value(self, mock_db):
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["add"],
            ["long-match", "--type", "option60", "--value", "x" * 256],
        )

        assert result.exit_code == 2
        assert "Match value must be between 1 and 255 characters" in result.output

    def test_match_update_referenced_value(self, mock_db):
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["update"],
            ["vendor-a", "--value", "MAIA-C"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|vendor-a", "value") == "MAIA-C"

    def test_match_update_ignores_unrelated_malformed_binding(self, mock_db):
        mock_db.hmset(
            "CONFIG_DB",
            "DHCP_SERVER_IPV4_BINDING|Vlan300|malformed",
            {"matches@": "vendor-b,,unused-match", "ips@": "100.1.3.20"},
        )

        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["update"],
            ["vendor-a", "--value", "MAIA-C"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|vendor-a", "value") == "MAIA-C"

    def test_match_update_rejects_duplicate_type_in_binding(self, mock_db):
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["update"],
            ["vendor-a", "--type", "circuit_id", "--value", "etp4"],
        )

        assert result.exit_code == 2
        assert "multiple matches of type circuit_id" in result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|vendor-a", "type") == "option60"

    def test_match_update_rejects_alias_outside_vlan(self, mock_db):
        result = invoke(
            mock_db,
            MATCH_COMMAND.commands["update"],
            ["port-etp2", "--value", "etp5"],
        )

        assert result.exit_code == 2
        assert "is not a member of vlan Vlan100" in result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|port-etp2", "value") == "etp2"

    def test_match_delete(self, mock_db):
        result = invoke(mock_db, MATCH_COMMAND.commands["del"], ["unused-match"])

        assert result.exit_code == 0, result.output
        assert not mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|unused-match")

    def test_match_delete_referenced(self, mock_db):
        result = invoke(mock_db, MATCH_COMMAND.commands["del"], ["port-etp2"])

        assert result.exit_code == 2
        assert "is referenced" in result.output

    def test_binding_add_with_ips(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "100.1.1.30,100.1.1.31", "--match", "vendor-b,port-etp2"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-b") == {
            "matches@": "port-etp2,vendor-b",
            "ips@": "100.1.1.30,100.1.1.31",
        }

    def test_binding_add_sorts_ips_numerically(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "100.1.1.30,100.1.1.9", "--match", "vendor-b,port-etp2"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-b", "ips@") == (
            "100.1.1.9,100.1.1.30"
        )

    def test_binding_add_with_range(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "--match", "port-etp2,vendor-b", "--range", "range3"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-b") == {
            "matches@": "port-etp2,vendor-b",
            "ranges@": "range3",
        }

    def test_binding_add_nonexistent_interface(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan200", "maia-b", "100.1.2.30", "--match", "port-etp2,vendor-b"],
        )

        assert result.exit_code == 2
        assert "not a valid dhcp interface" in result.output

    def test_binding_add_nonexistent_match(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "100.1.1.30", "--match", "missing-match"],
        )

        assert result.exit_code == 2
        assert "Match missing-match does not exist" in result.output

    def test_binding_add_duplicate_match_name(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "100.1.1.30", "--match", "vendor-b,vendor-b"],
        )

        assert result.exit_code == 2
        assert "Match list cannot contain duplicate values" in result.output

    def test_binding_add_duplicate_ip(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "100.1.1.30,100.1.1.30", "--match", "vendor-b,port-etp2"],
        )

        assert result.exit_code == 2
        assert "IP list cannot contain duplicate values" in result.output

    def test_binding_add_duplicate_match_type(self, mock_db):
        mock_db.hmset(
            "CONFIG_DB",
            "DHCP_SERVER_IPV4_MATCH|port-etp4",
            {"type": "circuit_id", "value": "etp4"},
        )

        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "two-ports", "100.1.1.30", "--match", "port-etp2,port-etp4"],
        )

        assert result.exit_code == 2
        assert "multiple matches of type circuit_id" in result.output

    def test_binding_add_with_both_pool_types(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            [
                "Vlan100",
                "maia-b",
                "100.1.1.30",
                "--match",
                "port-etp2,vendor-b",
                "--range",
                "range3",
            ],
        )

        assert result.exit_code == 2
        assert "Exactly one of IP list or range list" in result.output

    def test_binding_add_without_pool(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "--match", "port-etp2,vendor-b"],
        )

        assert result.exit_code == 2
        assert "Exactly one of IP list or range list" in result.output

    def test_binding_add_invalid_ip(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "100.1.1", "--match", "port-etp2,vendor-b"],
        )

        assert result.exit_code == 2
        assert "Illegal IP address" in result.output

    def test_binding_add_ip_out_of_subnet(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "100.1.2.30", "--match", "port-etp2,vendor-b"],
        )

        assert result.exit_code == 2
        assert "not in any subnet of vlan Vlan100" in result.output

    def test_binding_add_nonexistent_range(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "--match", "port-etp2,vendor-b", "--range", "range4"],
        )

        assert result.exit_code == 2
        assert "Range range4 does not exist" in result.output

    def test_binding_add_range_out_of_subnet(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "--match", "port-etp2,vendor-b", "--range", "range5"],
        )

        assert result.exit_code == 2
        assert "not in any subnet of vlan Vlan100" in result.output

    def test_binding_add_nonexistent_circuit_alias(self, mock_db):
        mock_db.hmset(
            "CONFIG_DB",
            "DHCP_SERVER_IPV4_MATCH|missing-port",
            {"type": "circuit_id", "value": "etp99"},
        )

        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "missing-port", "100.1.1.30", "--match", "missing-port"],
        )

        assert result.exit_code == 2
        assert "Circuit ID alias etp99 does not exist" in result.output

    @pytest.mark.parametrize("operation", ["add", "update"])
    def test_binding_validation_caches_alias_map(self, mock_db, operation):
        for index in range(124):
            mock_db.hmset("CONFIG_DB", "PORT|Ethernet{}".format(1000 + index), {
                "alias": "extra-{}".format(index),
            })
        for key in mock_db.keys("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|*"):
            mock_db.delete("CONFIG_DB", key)
        for index in range(1000):
            mock_db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_MATCH|scale-vendor-{}".format(index), {
                "type": "option60", "value": "SCALE-{}".format(index),
            })
            if operation == "update" or index:
                mock_db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|scale-{}".format(index), {
                    "matches@": "port-etp2,scale-vendor-{}".format(index), "ips@": "100.1.1.30",
                })
        mock_db.keys.reset_mock()
        mock_db.get.reset_mock()

        result = invoke(
            mock_db, BINDING_COMMAND.commands[operation],
            ["Vlan100", "scale-0", "100.1.1.30", "--match", "port-etp2,scale-vendor-0"],
        )

        assert result.exit_code == 0, result.output
        alias_reads = [call for call in mock_db.get.call_args_list
                       if call.args[1].startswith("PORT|") and call.args[2] == "alias"]
        assert len(alias_reads) == 128
        for pattern in ["PORT|*", "PORTCHANNEL|*"]:
            assert sum(call.args == ("CONFIG_DB", pattern) for call in mock_db.keys.call_args_list) == 1

    @pytest.mark.parametrize("match_name,value", [("port-etp2", "etp2"), ("vendor-a", "MAIA-C")])
    def test_match_update_shares_alias_map_across_vlans(self, mock_db, dhcp_vlan200, match_name, value):
        mock_db.hmset("CONFIG_DB", "VLAN_MEMBER|Vlan200|Ethernet2", {})
        mock_db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan200|maia-a", {
            "matches@": "port-etp2,vendor-a", "ips@": "100.1.2.20",
        })

        result = invoke(mock_db, MATCH_COMMAND.commands["update"], [match_name, "--value", value])

        assert result.exit_code == 0, result.output
        alias_reads = [call for call in mock_db.get.call_args_list
                       if call.args[1].startswith("PORT|") and call.args[2] == "alias"]
        assert len(alias_reads) == 4

    def test_range_update_reuses_alias_map(self, mock_db):
        result = invoke(mock_db, RANGE_COMMAND.commands["update"], ["range3", "100.1.1.50"])

        assert result.exit_code == 0, result.output
        alias_reads = [call for call in mock_db.get.call_args_list
                       if call.args[1].startswith("PORT|") and call.args[2] == "alias"]
        assert len(alias_reads) == 4

    def test_vendor_only_binding_does_not_read_aliases(self, mock_db, dhcp_vlan200):
        result = invoke(
            mock_db, BINDING_COMMAND.commands["add"],
            ["Vlan200", "vendor-only", "100.1.2.30", "--match", "vendor-a"],
        )

        assert result.exit_code == 0, result.output
        assert not any(call.args[1] in ["PORT|*", "PORTCHANNEL|*"] for call in mock_db.keys.call_args_list)

    @pytest.mark.parametrize("port,alias,error", [
        ("Ethernet2", "renamed", "Circuit ID alias etp2 does not exist"),
        ("Ethernet6", "etp2", "Circuit ID alias etp2 is ambiguous"),
    ])
    def test_binding_validation_refreshes_alias_map_between_commands(self, mock_db, port, alias, error):
        result = invoke(
            mock_db, BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "100.1.1.30", "--match", "port-etp2,vendor-b"],
        )
        assert result.exit_code == 0, result.output
        mock_db.hmset("CONFIG_DB", "PORT|" + port, {"alias": alias})

        result = invoke(mock_db, BINDING_COMMAND.commands["update"], ["Vlan100", "maia-b", "100.1.1.31"])

        assert result.exit_code == 2, result.output
        assert error in result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-b", "ips@") == "100.1.1.30"

    def test_binding_add_ambiguous_circuit_alias(self, mock_db):
        mock_db.hmset("CONFIG_DB", "PORT|Ethernet6", {"alias": "etp2"})

        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "ambiguous-port", "100.1.1.30", "--match", "port-etp2"],
        )

        assert result.exit_code == 2
        assert "Circuit ID alias etp2 is ambiguous" in result.output

    def test_binding_add_circuit_alias_outside_vlan(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "wrong-port", "100.1.1.30", "--match", "port-etp5"],
        )

        assert result.exit_code == 2
        assert "Circuit ID alias etp5 is not a member of vlan Vlan100" in result.output

    def test_binding_add_portchannel_circuit_id(self, mock_db):
        mock_db.hmset("CONFIG_DB", "PORTCHANNEL|PortChannel100", {"admin_status": "up"})
        mock_db.hmset("CONFIG_DB", "VLAN_MEMBER|Vlan100|PortChannel100", {"tagging_mode": "untagged"})
        mock_db.hmset(
            "CONFIG_DB",
            "DHCP_SERVER_IPV4_MATCH|port-lag",
            {"type": "circuit_id", "value": "PortChannel100"},
        )

        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "lag-binding", "100.1.1.30", "--match", "port-lag"],
        )

        assert result.exit_code == 0, result.output
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|lag-binding") == {
            "matches@": "port-lag",
            "ips@": "100.1.1.30",
        }

    def test_binding_add_aliasless_port_circuit_id(self, mock_db):
        mock_db.hmset("CONFIG_DB", "PORT|Ethernet12", {"admin_status": "up"})
        mock_db.hmset("CONFIG_DB", "VLAN_MEMBER|Vlan100|Ethernet12", {"tagging_mode": "untagged"})
        mock_db.hmset(
            "CONFIG_DB",
            "DHCP_SERVER_IPV4_MATCH|port-eth12",
            {"type": "circuit_id", "value": "Ethernet12"},
        )

        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "aliasless-binding", "100.1.1.30", "--match", "port-eth12"],
        )

        assert result.exit_code == 0, result.output

    def test_binding_add_portchannel_not_vlan_member(self, mock_db):
        mock_db.hmset("CONFIG_DB", "PORTCHANNEL|PortChannel200", {"admin_status": "up"})
        mock_db.hmset(
            "CONFIG_DB",
            "DHCP_SERVER_IPV4_MATCH|port-lag200",
            {"type": "circuit_id", "value": "PortChannel200"},
        )

        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "lag200-binding", "100.1.1.30", "--match", "port-lag200"],
        )

        assert result.exit_code == 2
        assert "Circuit ID alias PortChannel200 is not a member of vlan Vlan100" in result.output

    def test_binding_add_equal_specificity_overlap(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "vendor-only", "100.1.1.30", "--match", "vendor-a"],
        )

        assert result.exit_code == 2
        assert "overlap with equal specificity" in result.output

    def test_binding_delete(self, mock_db):
        result = invoke(mock_db, BINDING_COMMAND.commands["del"], ["Vlan100", "maia-a"])

        assert result.exit_code == 0, result.output
        assert not mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-a")

    def test_range_delete_referenced_by_binding(self, mock_db):
        mock_db.delete("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet7")

        result = invoke(mock_db, IPV4_COMMAND.commands["range"].commands["del"], ["range3"])

        assert result.exit_code == 2
        assert "DHCP_SERVER_IPV4_BINDING|Vlan100|fallback" in result.output

    def test_binding_update_match_list(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "maia-a", "--match", "port-etp2,vendor-b"],
        )

        assert result.exit_code == 0, result.output
        key = "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-a"
        assert mock_db.get("CONFIG_DB", key, "matches@") == "port-etp2,vendor-b"
        assert mock_db.get("CONFIG_DB", key, "ips@") == "100.1.1.20"

    def test_binding_update_ip_list(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "maia-a", "100.1.1.30,100.1.1.31"],
        )

        assert result.exit_code == 0, result.output
        key = "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-a"
        assert mock_db.get("CONFIG_DB", key, "ips@") == "100.1.1.30,100.1.1.31"
        assert mock_db.get("CONFIG_DB", key, "matches@") == "port-etp2,vendor-a"

    def test_binding_update_switch_ips_to_range(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "maia-a", "--range", "range1"],
        )

        assert result.exit_code == 0, result.output
        key = "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-a"
        assert mock_db.get("CONFIG_DB", key, "ranges@") == "range1"
        assert not mock_db.get("CONFIG_DB", key, "ips@")

    def test_binding_update_switch_range_to_ips(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "fallback", "100.1.1.40"],
        )

        assert result.exit_code == 0, result.output
        key = "DHCP_SERVER_IPV4_BINDING|Vlan100|fallback"
        assert mock_db.get("CONFIG_DB", key, "ips@") == "100.1.1.40"
        assert not mock_db.get("CONFIG_DB", key, "ranges@")

    def test_binding_update_not_exist(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "no-such-binding", "100.1.1.30"],
        )

        assert result.exit_code == 2
        assert "Binding no-such-binding does not exist on Vlan100" in result.output

    def test_binding_update_without_any_option(self, mock_db):
        result = invoke(mock_db, BINDING_COMMAND.commands["update"], ["Vlan100", "maia-a"])

        assert result.exit_code == 2
        assert "Nothing to update" in result.output

    def test_binding_update_both_ips_and_range(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "maia-a", "100.1.1.30", "--range", "range1"],
        )

        assert result.exit_code == 2
        assert "Exactly one of IP list or range list must be provided" in result.output

    def test_binding_update_match_not_exist(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "maia-a", "--match", "no-such-match"],
        )

        assert result.exit_code == 2
        assert "Match no-such-match does not exist" in result.output

    def test_binding_update_ip_not_in_subnet(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "maia-a", "200.1.1.30"],
        )

        assert result.exit_code == 2
        assert "is not in any subnet of vlan Vlan100" in result.output

    def test_binding_update_causing_equal_specificity_overlap(self, mock_db):
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "maia-a", "--match", "port-etp2"],
        )

        assert result.exit_code == 2
        assert "overlap with equal specificity" in result.output

    def test_binding_update_rejected_leaves_config_unchanged(self, mock_db):
        key = "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-a"
        before = dict(mock_db.get_all("CONFIG_DB", key))

        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["update"],
            ["Vlan100", "maia-a", "--match", "no-such-match"],
        )

        assert result.exit_code == 2
        assert dict(mock_db.get_all("CONFIG_DB", key)) == before

    @pytest.mark.parametrize("ip_list, exit_code", [
        ("100.1.1.20", 0),
        ("100.1.1.21", 0),
        ("100.1.1.20,100.1.1.21", 2),
    ])
    def test_binding_add_pool_overlap(self, mock_db, ip_list, exit_code):
        """maia-a owns 100.1.1.20; Kea can share an identical pool but not compile a partial overlap."""
        result = invoke(
            mock_db,
            BINDING_COMMAND.commands["add"],
            ["Vlan100", "maia-b", "--match", "port-etp2,vendor-b", ip_list],
        )

        assert result.exit_code == exit_code, result.output
        if exit_code:
            assert "Bindings maia-a and maia-b have partially overlapping pools on Vlan100" in result.output
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-b") == (exit_code == 0)

    def test_range_update_causing_partial_pool_overlap_rejected(self, mock_db):
        result = invoke(mock_db, RANGE_COMMAND.commands["update"], ["range3", "100.1.1.10", "100.1.1.20"])

        assert result.exit_code == 2
        assert "Bindings fallback and maia-a have partially overlapping pools on Vlan100" in result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range3", "range@") == "100.1.1.10"

    def test_enable_match_interface_with_partial_pool_overlap_rejected(self, mock_db):
        mock_db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-b",
                      {"matches@": "port-etp2,vendor-b", "ips@": "100.1.1.20,100.1.1.21"})
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode", "MATCH")
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "state", "disabled")

        result = invoke(mock_db, IPV4_COMMAND.commands["enable"], ["Vlan100"])

        assert result.exit_code == 2
        assert "partially overlapping pools" in result.output
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "state") == "disabled"

    @pytest.mark.parametrize("state", ["enabled", "disabled"])
    def test_binding_del_final_binding(self, mock_db, state):
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode", "MATCH")
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "state", state)
        mock_db.delete("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|fallback")

        result = invoke(mock_db, BINDING_COMMAND.commands["del"], ["Vlan100", "maia-a"])

        assert result.exit_code == 0, result.output
        assert not mock_db.keys("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|*")

    @pytest.mark.parametrize("command,args", [
        (BINDING_COMMAND.commands["add"], ["Vlan100", "vendor-b", "--match=vendor-b", "100.1.1.40"]),
        (BINDING_COMMAND.commands["update"], ["Vlan100", "maia-a", "--match=vendor-a"]),
        (IPV4_COMMAND.commands["enable"], ["Vlan100"]),
        (IPV4_COMMAND.commands["update"], ["Vlan100", "--mode=MATCH"]),
        (MATCH_COMMAND.commands["update"], ["vendor-a", "--value=MAIA-C"]),
        (RANGE_COMMAND.commands["update"], ["range3", "100.1.1.50"]),
        (BINDING_COMMAND.commands["del"], ["Vlan100", "fallback"]),
    ])
    def test_stale_binding_does_not_block_other_changes(self, mock_db, command, args):
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "mode", "MATCH")
        mock_db.set("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "state", "disabled")
        # fallback goes stale when its port leaves the VLAN, as dhcp_cfggen then skips it
        mock_db.delete("CONFIG_DB", "VLAN_MEMBER|Vlan100|Ethernet2")
        mock_db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4_BINDING|Vlan100|maia-a",
                      {"matches@": "vendor-a", "ips@": "100.1.1.20"})

        result = invoke(mock_db, command, args)

        assert result.exit_code == 0, result.output

    def test_stale_binding_cannot_be_rewritten_invalid(self, mock_db):
        mock_db.delete("CONFIG_DB", "VLAN_MEMBER|Vlan100|Ethernet2")

        result = invoke(mock_db, BINDING_COMMAND.commands["update"], ["Vlan100", "fallback", "--range=range3"])

        assert result.exit_code == 2
        assert "is not a member of vlan Vlan100" in result.output
