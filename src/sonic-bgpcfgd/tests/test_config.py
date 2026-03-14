from unittest.mock import MagicMock

from bgpcfgd.config import ConfigMgr


def test_constructor():
    frr = MagicMock()
    c = ConfigMgr(frr)
    assert c.frr == frr
    assert c.current_config is None
    assert c.current_config_raw is None
    assert c.changes == ""
    assert c.peer_groups_to_restart == []

def test_reset():
    frr = MagicMock()
    c = ConfigMgr(frr)
    c.reset()
    assert c.frr == frr
    assert c.current_config is None
    assert c.current_config_raw is None
    assert c.changes == ""
    assert c.peer_groups_to_restart == []

def test_update():
    frr = MagicMock()
    frr.get_config = MagicMock(return_value = """!
 text1
 ! comment
 text2
 text3
 ! comment
 text4
    """)
    c = ConfigMgr(frr)
    c.update()
    assert c.current_config_raw == [' text1', ' text2', ' text3', ' text4', '    ', '     ']
    assert c.current_config == [['text1'], ['text2'], ['text3'], ['text4']]

def test_push_list():
    frr = MagicMock()
    c = ConfigMgr(frr)
    c.push_list(["change1", "change2"])
    assert c.changes == "change1\nchange2\n"
    c.push_list(["change3", "change4"])
    assert c.changes == "change1\nchange2\nchange3\nchange4\n"

def test_push():
    frr = MagicMock()
    c = ConfigMgr(frr)
    c.push("update1\nupdate2\n")
    assert c.changes == "update1\nupdate2\n\n"
    c.push("update3\nupdate4\n")
    assert c.changes == "update1\nupdate2\n\nupdate3\nupdate4\n\n"

def test_push_and_push_list():
    frr = MagicMock()
    c = ConfigMgr(frr)
    c.push("update1\nupdate2\n")
    c.push_list(["change1", "change2"])
    assert c.changes == "update1\nupdate2\n\nchange1\nchange2\n"

def test_restart_peer_groups():
    frr = MagicMock()
    c = ConfigMgr(frr)
    c.restart_peer_groups(["pg_1", "pg_2"])
    assert c.peer_groups_to_restart == ["pg_1", "pg_2"]
    c.restart_peer_groups(["pg_3", "pg_4"])
    assert c.peer_groups_to_restart == ["pg_1", "pg_2", "pg_3", "pg_4"]

def test_commit_empty_changes():
    frr = MagicMock()
    c = ConfigMgr(frr)
    res = c.commit()
    assert res
    assert not frr.write.called

def commit_changes_common(write_error, restart_error, result):
    frr = MagicMock()
    frr.write = MagicMock(return_value = write_error)
    frr.restart_peer_groups = MagicMock(return_value = restart_error)
    c = ConfigMgr(frr)
    c.reset = MagicMock()
    c.push_list(["change1", "change2"])
    c.restart_peer_groups(["pg1", "pg2"])
    res = c.commit()
    assert res == result
    assert c.reset.called
    frr.write.assert_called_with('change1\nchange2\n')
    frr.restart_peer_groups.assert_called_with(["pg1", "pg2"])

def test_commit_changes_no_errors():
    commit_changes_common(True, True, True)

def test_commit_changes_write_error():
    commit_changes_common(False, True, False)

def test_commit_changes_restart_error():
    commit_changes_common(True, False, False)

def test_commit_changes_both_errors():
    commit_changes_common(False, False, False)

def test_restart_get_text():
    frr = MagicMock()
    frr.get_config = MagicMock(return_value = """!
 text1
 ! comment
 text2
 text3
 ! comment
 text4
    """)
    c = ConfigMgr(frr)
    c.update()
    assert c.get_text() == [' text1', ' text2', ' text3', ' text4', '    ', '     ']

def to_canonical_common(raw_text, expected_canonical):
    frr = MagicMock()
    c = ConfigMgr(frr)
    assert c.to_canonical(raw_text) == expected_canonical

def test_to_canonical_empty():
    raw_config = """
!
    !
    !
    !

    !

"""
    to_canonical_common(raw_config, [])

def test_to_canonical_():
    raw_config = """
!
router bgp 12345
  bgp router-id 1020
  address-family ipv4
    neighbor PEER_V4 peer-group
    neighbor PEER_V4 route-map A10 in
  exit-address-family
  address-family ipv6
    neighbor PEER_V6 peer-group
    neighbor PEER_V6 route-map A20 in
  exit-address-family
route-map A10 permit 10
!
route-map A20 permit 10
!

"""
    expected = [
        ['router bgp 12345'],
        ['router bgp 12345', 'bgp router-id 1020'],
        ['router bgp 12345', 'address-family ipv4'],
        ['router bgp 12345', 'address-family ipv4', 'neighbor PEER_V4 peer-group'],
        ['router bgp 12345', 'address-family ipv4', 'neighbor PEER_V4 route-map A10 in'],
        ['router bgp 12345', 'exit-address-family'],
        ['router bgp 12345', 'address-family ipv6'],
        ['router bgp 12345', 'address-family ipv6', 'neighbor PEER_V6 peer-group'],
        ['router bgp 12345', 'address-family ipv6', 'neighbor PEER_V6 route-map A20 in'],
        ['router bgp 12345', 'exit-address-family'],
        ['route-map A10 permit 10'],
        ['route-map A20 permit 10']
    ]
    to_canonical_common(raw_config, expected)

def test_count_spaces():
    frr = MagicMock()
    c = ConfigMgr(frr)
    assert c.count_spaces("  !") == 2
    assert c.count_spaces("!") == 0
    assert c.count_spaces("") == 0

def test_from_canonical():
    canonical = [
        ['router bgp 12345'],
        ['router bgp 12345', 'bgp router-id 1020'],
        ['router bgp 12345', 'address-family ipv4'],
        ['router bgp 12345', 'address-family ipv4', 'neighbor PEER_V4 peer-group'],
        ['router bgp 12345', 'address-family ipv4', 'neighbor PEER_V4 route-map A10 in'],
        ['router bgp 12345', 'exit-address-family'],
        ['router bgp 12345', 'address-family ipv6'],
        ['router bgp 12345', 'address-family ipv6', 'neighbor PEER_V6 peer-group'],
        ['router bgp 12345', 'address-family ipv6', 'neighbor PEER_V6 route-map A20 in'],
        ['router bgp 12345', 'exit-address-family'],
        ['route-map A10 permit 10'],
        ['route-map A20 permit 10']
    ]
    expected = 'router bgp 12345\n' \
               ' bgp router-id 1020\n' \
               ' address-family ipv4\n' \
               '  neighbor PEER_V4 peer-group\n' \
               '  neighbor PEER_V4 route-map A10 in\n' \
               ' exit-address-family\n' \
               ' address-family ipv6\n' \
               '  neighbor PEER_V6 peer-group\n' \
               '  neighbor PEER_V6 route-map A20 in\n' \
               ' exit-address-family\n' \
               'route-map A10 permit 10\n' \
               'route-map A20 permit 10\n'
    frr = MagicMock()
    c = ConfigMgr(frr)
    raw = c.from_canonical(canonical)
    assert raw == expected
