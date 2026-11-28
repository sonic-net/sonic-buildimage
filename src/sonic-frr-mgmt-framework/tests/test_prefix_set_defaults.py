"""Unit tests for prefix-set and prefix rows that omit an optional field.

Two fields on the routing-policy prefix tables are optional:

  PREFIX_SET.mode   declares a default of IPv4 in sonic-routing-policy-sets.yang
  PREFIX.action     declares a default of permit, and is the only non-key field
                    on the row, so a keys-only row is an ordinary thing to write

A defaulted leaf is not guaranteed to be materialised in CONFIG_DB, so every
consumer has to apply the default rather than require the field.  Discarding
either row loses the whole prefix-list, and FRR then treats a route-map match
against the absent list as no-match rather than as an error.

Three consumers have to agree: the runtime keyspace handler, the startup cache
build, and the boot-time Jinja templates that program FRR at container start and
on `config reload`.  All three are covered here.
"""

import os

from unittest.mock import MagicMock, NonCallableMagicMock, patch

from jinja2 import Environment, FileSystemLoader

swsscommon_module_mock = MagicMock(ConfigDBConnector=NonCallableMagicMock)

mockmapping = {
    'swsscommon.swsscommon': swsscommon_module_mock,
    'bgpcfgd': MagicMock(),
    'bgpcfgd.managers_bfd': MagicMock(),
    'bgpcfgd.directory': MagicMock(),
    'bgpcfgd.log': MagicMock(),
    'bgpcfgd.utils': MagicMock(),
}


def make_daemon(run_cmd, db_tables=None):
    """A daemon with an empty prefix-set cache and a stubbed CONFIG_DB."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.prefix_set_list = {}
    daemon.table_data_cache = {}
    tables = db_tables or {}
    daemon.config_db.get_table = MagicMock(side_effect=lambda t: tables.get(t, {}))
    daemon.config_db.get_entry = MagicMock(side_effect=lambda t, n: {})
    daemon.config_db.serialize_key = MagicMock(
            side_effect=lambda k: '|'.join(k) if isinstance(k, tuple) else k)
    return daemon


def cache_row(daemon, table, key, entry):
    """Seed table_data_cache the way __init__ seeds it from CONFIG_DB."""
    from frrcfgd.frrcfgd import ExtConfigDBConnector

    daemon.table_data_cache[ExtConfigDBConnector.get_table_key(table, key)] = dict(entry)


def handle(daemon, run_cmd, table, key, data):
    """Deliver one keyspace event; return the vtysh commands it issued."""
    run_cmd.reset_mock()
    daemon.bgp_table_handler_common(table, key, data)
    return [' '.join(c[0][1]) for c in run_cmd.call_args_list]


def tables_of(run_cmd):
    """The table name each issued command was run against (picks its daemons)."""
    return [c[0][0] for c in run_cmd.call_args_list]


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_prefix_set_without_mode_registers_as_ipv4(run_cmd):
    """An absent mode means IPv4, not a missing value.

    Previously the prefix-set was discarded with an error log, which also made
    every prefix beneath it unresolvable.
    """
    import socket

    daemon = make_daemon(run_cmd)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_NOMODE', {})

    assert 'PFS_NOMODE' in daemon.prefix_set_list, daemon.prefix_set_list
    assert daemon.prefix_set_list['PFS_NOMODE'].af == socket.AF_INET


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_prefix_set_with_explicit_mode_is_still_honoured(run_cmd):
    """Applying the default must not override an explicitly configured mode."""
    import socket

    daemon = make_daemon(run_cmd)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_V6', {'mode': 'IPv6'})

    assert daemon.prefix_set_list['PFS_V6'].af == socket.AF_INET6


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_prefix_set_already_registered_is_not_replayed(run_cmd):
    """A set the startup cache already holds is left alone.

    __init__ now caches every prefix-set, mode-less ones included, so the
    "already exists" short-circuit is the common path after a restart.  It must
    not re-register the set or replay its prefixes a second time.
    """
    import socket

    from frrcfgd.frrcfgd import MatchPrefixList

    db = {'PREFIX': {('PFS_BOOT', '10', '10.1.0.0/16', 'exact'): {}}}
    daemon = make_daemon(run_cmd, db_tables=db)
    daemon.prefix_set_list['PFS_BOOT'] = MatchPrefixList('ipv4')

    calls = handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_BOOT', {'mode': 'IPv4'})

    assert calls == [], calls
    assert daemon.prefix_set_list['PFS_BOOT'].af == socket.AF_INET
    assert len(daemon.prefix_set_list['PFS_BOOT']) == 0


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_prefix_without_action_is_programmed_as_permit(run_cmd):
    """A keys-only PREFIX row reaches FRR as permit.

    The startup path already reads such a row as permit; the runtime path used
    to skip it silently, so the prefix was never programmed.
    """
    daemon = make_daemon(run_cmd)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_NOACTION', {})
    calls = handle(daemon, run_cmd, 'PREFIX',
                   'PFS_NOACTION|10|192.168.240.0/24|exact', {})

    combined = ' '.join(calls)
    assert 'ip prefix-list PFS_NOACTION seq 10 permit 192.168.240.0/24' in combined, calls
    assert 'ipv6 prefix-list' not in combined, calls


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_prefix_with_explicit_action_is_unchanged(run_cmd):
    """Defaulting the action must not override an explicit deny."""
    daemon = make_daemon(run_cmd)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_DENY', {})
    calls = handle(daemon, run_cmd, 'PREFIX',
                   'PFS_DENY|10|192.168.240.0/24|exact', {'action': 'deny'})

    assert 'ip prefix-list PFS_DENY seq 10 deny 192.168.240.0/24' in ' '.join(calls), calls


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_unchanged_deny_is_not_reprogrammed_as_permit(run_cmd):
    """An action equal to its cached value is present, not absent.

    __add_op_to_data emits OP_NONE for a field that matches the cache, and the
    cache is seeded from CONFIG_DB at startup -- so any second write of the same
    row (a config load of the same file, a GCU re-apply) delivers OP_NONE for a
    real deny.  Defaulting there would invert the policy.
    """
    key = 'PFS_DENY|10|192.168.240.0/24|exact'

    daemon = make_daemon(run_cmd)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_DENY', {})
    handle(daemon, run_cmd, 'PREFIX', key, {'action': 'deny'})
    cache_row(daemon, 'PREFIX', key, {'action': 'deny'})

    calls = handle(daemon, run_cmd, 'PREFIX', key, {'action': 'deny'})

    assert calls == [], calls
    assert str(daemon.prefix_set_list['PFS_DENY'][0]) == 'seq 10 deny 192.168.240.0/24'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_dropping_the_action_field_reverts_to_permit(run_cmd):
    """Removing action from a surviving row means the default, not a deletion.

    The row is still configured, so the prefix must stay programmed -- as
    permit, which is what the model says an absent action is.
    """
    key = 'PFS_REVERT|10|192.168.240.0/24|exact'

    daemon = make_daemon(run_cmd)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_REVERT', {})
    handle(daemon, run_cmd, 'PREFIX', key, {'action': 'deny'})
    cache_row(daemon, 'PREFIX', key, {'action': 'deny'})

    calls = handle(daemon, run_cmd, 'PREFIX', key, {})

    combined = ' '.join(calls)
    assert 'no ip prefix-list PFS_REVERT seq 10 deny 192.168.240.0/24' in combined, calls
    assert 'ip prefix-list PFS_REVERT seq 10 permit 192.168.240.0/24' in combined, calls
    assert len(daemon.prefix_set_list['PFS_REVERT']) == 1, list(
            map(str, daemon.prefix_set_list['PFS_REVERT']))


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_rewriting_a_keys_only_row_changes_nothing(run_cmd):
    """The synthetic permit is cached, so a re-write must not churn the entry.

    The cached value is the default the absent field already means, so there is
    nothing to reprogram -- and the cache entry has to survive, because it is
    what a later row delete matches against.
    """
    from frrcfgd.frrcfgd import ExtConfigDBConnector

    key = 'PFS_SAME|10|192.168.240.0/24|exact'
    table_key = ExtConfigDBConnector.get_table_key('PREFIX', key)

    daemon = make_daemon(run_cmd)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_SAME', {})
    handle(daemon, run_cmd, 'PREFIX', key, {})
    assert daemon.table_data_cache[table_key] == {'action': 'permit'}

    calls = handle(daemon, run_cmd, 'PREFIX', key, {})

    assert calls == [], calls
    assert daemon.table_data_cache[table_key] == {'action': 'permit'}
    assert len(daemon.prefix_set_list['PFS_SAME']) == 1, list(
            map(str, daemon.prefix_set_list['PFS_SAME']))


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_deleting_a_keys_only_row_removes_the_prefix(run_cmd):
    """A keys-only row was programmed at boot, so deleting it must unprogram it.

    This is the state a reboot leaves: the boot templates rendered the entry as
    permit and __init__ cached the row exactly as CONFIG_DB holds it, with no
    action field.  Reading that absent action as "no row" leaves the entry
    behind in FRR after the row is gone.
    """
    from frrcfgd.frrcfgd import MatchPrefixList

    key = 'PFS_GONE|10|192.168.240.0/24|exact'

    daemon = make_daemon(run_cmd)
    daemon.prefix_set_list['PFS_GONE'] = MatchPrefixList('ipv4')
    daemon.prefix_set_list['PFS_GONE'].add_prefix('192.168.240.0/24', None, 'permit', '10')
    cache_row(daemon, 'PREFIX', key, {})

    calls = handle(daemon, run_cmd, 'PREFIX', key, None)

    assert 'no ip prefix-list PFS_GONE seq 10 permit 192.168.240.0/24' in ' '.join(calls), calls
    assert len(daemon.prefix_set_list['PFS_GONE']) == 0, list(
            map(str, daemon.prefix_set_list['PFS_GONE']))


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_prefix_set_registration_replays_prefixes_already_in_config_db(run_cmd):
    """Registering a prefix-set programs the prefixes already written under it.

    A PREFIX row reaches FRR only on its own keyspace event, so supplying a
    missing mode after the fact used to leave the prefix-list empty until the
    prefixes were rewritten or the daemon restarted.
    """
    db = {
        'PREFIX': {
            ('PFS_LATE', '10', '10.1.0.0/16', 'exact'): {'action': 'deny'},
            ('PFS_LATE', '20', '10.2.0.0/16', 'exact'): {},
            ('PFS_OTHER', '10', '10.9.0.0/16', 'exact'): {'action': 'permit'},
        },
    }
    daemon = make_daemon(run_cmd, db_tables=db)
    calls = handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_LATE', {'mode': 'IPv4'})

    combined = ' '.join(calls)
    assert 'ip prefix-list PFS_LATE seq 10 deny 10.1.0.0/16' in combined, calls
    # the row carrying no action is replayed as permit
    assert 'ip prefix-list PFS_LATE seq 20 permit 10.2.0.0/16' in combined, calls
    # a prefix belonging to a different set is not replayed here
    assert 'PFS_OTHER' not in combined, calls


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_replayed_prefixes_go_to_every_prefix_daemon(run_cmd):
    """The replay runs its commands as PREFIX, not as the triggering table.

    An IPv4 prefix-list resolves its daemons from TABLE_DAEMON, and
    TABLE_DAEMON['PREFIX_SET'] is bgpd alone -- a list programmed under it would
    never reach zebra, ospfd or pimd, which is the failure this change fixes.
    """
    db = {'PREFIX': {('PFS_DAEMONS', '10', '10.1.0.0/16', 'exact'): {}}}
    daemon = make_daemon(run_cmd, db_tables=db)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_DAEMONS', {'mode': 'IPv4'})

    assert tables_of(run_cmd) == ['PREFIX'], tables_of(run_cmd)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_replay_does_not_double_program_the_rows_own_event(run_cmd):
    """A replayed row is recorded, so its own keyspace event is a no-op.

    add_prefix appends without dedup, so programming a row twice leaves two
    copies in the cache; a later delete then removes one and strands the other,
    and get_prefix goes on returning the stale entry.
    """
    db = {
        'PREFIX': {
            ('PFS_ORDER', '10', '10.1.0.0/16', 'exact'): {'action': 'deny'},
            ('PFS_ORDER', '20', '10.2.0.0/16', 'exact'): {},
        },
    }
    daemon = make_daemon(run_cmd, db_tables=db)
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_ORDER', {'mode': 'IPv4'})

    # each PREFIX row's own event follows the PREFIX_SET one in the ordinary
    # config-load ordering
    follow = handle(daemon, run_cmd, 'PREFIX',
                    'PFS_ORDER|10|10.1.0.0/16|exact', {'action': 'deny'})
    follow += handle(daemon, run_cmd, 'PREFIX',
                     'PFS_ORDER|20|10.2.0.0/16|exact', {})

    assert follow == [], follow
    assert len(daemon.prefix_set_list['PFS_ORDER']) == 2, list(
            map(str, daemon.prefix_set_list['PFS_ORDER']))


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_failed_replay_leaves_nothing_for_the_retry_to_double(run_cmd):
    """A replay whose command fails must leave no entry behind.

    add_prefix appends before the command is attempted.  Keeping that entry on
    failure means the row's own event -- which finds nothing cached, so programs
    the row itself -- ends up with two copies for the one command that landed,
    and a later delete removes one and strands the other.  The ordinary add path
    already reverts its append on the same failure.
    """
    db = {
        'PREFIX': {
            ('PFS_FAIL', '10', '10.1.0.0/16', 'exact'): {'action': 'deny'},
        },
    }
    daemon = make_daemon(run_cmd, db_tables=db)

    # the replay's command fails
    run_cmd.return_value = False
    handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_FAIL', {'mode': 'IPv4'})
    assert len(daemon.prefix_set_list['PFS_FAIL']) == 0, list(
            map(str, daemon.prefix_set_list['PFS_FAIL']))

    # the row's own event then programs it, once
    run_cmd.return_value = True
    retry = handle(daemon, run_cmd, 'PREFIX',
                   'PFS_FAIL|10|10.1.0.0/16|exact', {'action': 'deny'})
    assert len(retry) == 1, retry
    assert len(daemon.prefix_set_list['PFS_FAIL']) == 1, list(
            map(str, daemon.prefix_set_list['PFS_FAIL']))


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_replay_reads_the_prefix_table_once_per_batch(run_cmd):
    """P registrations must not cost P full PREFIX table reads.

    get_table is a keys() scan plus one hgetall per row on the single listener
    thread, so re-reading it per prefix-set is P x (N+1) round trips.
    """
    db = {
        'PREFIX': {
            ('PFS_A', '10', '10.1.0.0/16', 'exact'): {},
            ('PFS_B', '10', '10.2.0.0/16', 'exact'): {},
        },
    }
    daemon = make_daemon(run_cmd, db_tables=db)
    daemon.bgp_message.put(('PFS_A', False, 'PREFIX_SET', {}))
    daemon.bgp_message.put(('PFS_B', False, 'PREFIX_SET', {}))
    daemon._BGPConfigDaemon__update_bgp([])

    prefix_reads = [c for c in daemon.config_db.get_table.call_args_list
                    if c[0][0] == 'PREFIX']
    assert len(prefix_reads) == 1, prefix_reads
    assert 'PFS_A' in daemon.prefix_set_list and 'PFS_B' in daemon.prefix_set_list


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_replay_skips_prefix_of_the_wrong_address_family(run_cmd):
    """A v6 prefix filed under a v4 set is skipped, not fatal to the replay."""
    db = {
        'PREFIX': {
            ('PFS_MIXED', '10', '10.1.0.0/16', 'exact'): {},
            ('PFS_MIXED', '20', 'fc00::/64', 'exact'): {},
        },
    }
    daemon = make_daemon(run_cmd, db_tables=db)
    calls = handle(daemon, run_cmd, 'PREFIX_SET', 'PFS_MIXED', {'mode': 'IPv4'})

    combined = ' '.join(calls)
    assert 'ip prefix-list PFS_MIXED seq 10 permit 10.1.0.0/16' in combined, calls
    assert 'fc00::/64' not in combined, calls


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_startup_cache_applies_both_defaults(run_cmd):
    """The startup cache build agrees with the runtime path.

    A prefix-set with no mode is cached as IPv4, and a prefix with no action is
    cached as permit.  The startup path already defaulted the action; the mode
    half used to skip the prefix-set entirely.  An empty-string mode reads the
    same way here as it does at runtime.
    """
    import socket

    from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector

    run_cmd.return_value = True
    tables = {
        'PREFIX_SET': {'PFS_BOOT': {}, 'PFS_EMPTY': {'mode': ''}},
        'PREFIX': {('PFS_BOOT', '10', '10.3.0.0/16', 'exact'): {}},
    }
    # ExtConfigDBConnector inherits get_table from the mocked base class, so
    # there is no real attribute to replace -- create the patch point.
    with patch.object(ExtConfigDBConnector, 'get_table', create=True,
                      side_effect=lambda table: tables.get(table, {})):
        daemon = BGPConfigDaemon()

    assert 'PFS_BOOT' in daemon.prefix_set_list, daemon.prefix_set_list
    pfx_list = daemon.prefix_set_list['PFS_BOOT']
    assert pfx_list.af == socket.AF_INET
    assert len(pfx_list) == 1, list(map(str, pfx_list))
    assert str(pfx_list[0]) == 'seq 10 permit 10.3.0.0/16', str(pfx_list[0])
    assert daemon.prefix_set_list['PFS_EMPTY'].af == socket.AF_INET


def restored_daemon(run_cmd, tables):
    """A daemon built the way unified restore builds one.

    docker_routing_config_mode=unified with use_template_render_for_restore
    false makes __init__ replay every CONFIG_DB row through the ordinary
    handlers, so FRR is programmed from those events rather than from a
    rendered config.  That is the path this constructs.
    """
    from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector

    run_cmd.return_value = True
    metadata = {
        'bgp_asn': '65100',
        'docker_routing_config_mode': 'unified',
        'use_template_render_for_restore': 'false',
    }
    entries = {('DEVICE_METADATA', 'localhost'): metadata}
    # ExtConfigDBConnector inherits these from the mocked base class, so there
    # is no real attribute to replace -- create the patch points.
    with patch.object(ExtConfigDBConnector, 'get_table', create=True,
                      side_effect=lambda table: {k: dict(v) for k, v
                                                 in tables.get(table, {}).items()}), \
         patch.object(ExtConfigDBConnector, 'get_entry', create=True,
                      side_effect=lambda table, key: dict(entries.get((table, key), {}))), \
         patch.object(ExtConfigDBConnector, 'serialize_key', create=True,
                      side_effect=lambda key, sep='|': (sep.join(key)
                                                        if isinstance(key, tuple) else key)):
        return BGPConfigDaemon()


def prefix_list_cmds(run_cmd):
    """Just the issued prefix-list commands, dropping the constructor's others."""
    return [' '.join(c[0][1]) for c in run_cmd.call_args_list
            if any('prefix-list' in str(tok) for tok in c[0][1])]


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_unified_restore_holds_one_entry_per_keys_only_row(run_cmd):
    """Restore must leave the daemon holding exactly what it programmed.

    The constructor used to seed prefix_set_list from the PREFIX table and then
    replay the same rows through the handlers, and add_prefix appends without
    dedup, so a row ended up cached twice for the one command restore issued.  A
    later delete then removed one copy and stranded the other, and get_prefix --
    which compares neither action nor sequence -- resolves the stale copy first,
    so the daemon stopped holding a convergent model of the list.
    """
    tables = {
        'PREFIX_SET': {'PFS_RESTORE': {'mode': 'IPv4'}},
        'PREFIX': {('PFS_RESTORE', '10.4.0.0/16', 'exact'): {}},
    }
    daemon = restored_daemon(run_cmd, tables)

    pfx_list = daemon.prefix_set_list['PFS_RESTORE']
    assert len(pfx_list) == 1, list(map(str, pfx_list))
    assert str(pfx_list[0]) == 'permit 10.4.0.0/16', str(pfx_list[0])
    assert prefix_list_cmds(run_cmd) == [
        "vtysh -c configure terminal -c ip prefix-list PFS_RESTORE permit 10.4.0.0/16"], \
        prefix_list_cmds(run_cmd)

    # and the row leaves cleanly, with nothing stranded behind it
    handle(daemon, run_cmd, 'PREFIX', 'PFS_RESTORE|10.4.0.0/16|exact', None)
    assert len(pfx_list) == 0, list(map(str, pfx_list))


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_unified_restore_holds_one_entry_per_explicit_action_row(run_cmd):
    """The same for a row that names its action.

    This shape predates the defaults work -- an explicit action always reached
    the restore handler -- so the duplicate it left behind was never specific to
    a keys-only row.
    """
    tables = {
        'PREFIX_SET': {'PFS_RESTORE': {'mode': 'IPv4'}},
        'PREFIX': {('PFS_RESTORE', '20', '10.5.0.0/16', 'exact'): {'action': 'deny'}},
    }
    daemon = restored_daemon(run_cmd, tables)

    pfx_list = daemon.prefix_set_list['PFS_RESTORE']
    assert len(pfx_list) == 1, list(map(str, pfx_list))
    assert str(pfx_list[0]) == 'seq 20 deny 10.5.0.0/16', str(pfx_list[0])

    handle(daemon, run_cmd, 'PREFIX', 'PFS_RESTORE|20|10.5.0.0/16|exact', None)
    assert len(pfx_list) == 0, list(map(str, pfx_list))


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_startup_cache_still_seeds_prefixes_off_the_restore_path(run_cmd):
    """Only the replay path skips the seeding.

    In separated mode, or with template rendering on, nothing replays the rows
    afterwards, so the constructor's cache is the only record the daemon has.
    """
    tables = {
        'PREFIX_SET': {'PFS_BOOT': {'mode': 'IPv4'}},
        'PREFIX': {('PFS_BOOT', '10', '10.6.0.0/16', 'exact'): {}},
    }
    from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector

    run_cmd.return_value = True
    with patch.object(ExtConfigDBConnector, 'get_table', create=True,
                      side_effect=lambda table: {k: dict(v) for k, v
                                                 in tables.get(table, {}).items()}):
        daemon = BGPConfigDaemon()

    assert daemon.config_mode == 'separated'
    pfx_list = daemon.prefix_set_list['PFS_BOOT']
    assert len(pfx_list) == 1, list(map(str, pfx_list))
    assert str(pfx_list[0]) == 'seq 10 permit 10.6.0.0/16', str(pfx_list[0])


def _render(template_name, **context):
    """Render a bgpd template the way sonic-cfggen renders it at cold boot."""
    template_dir = os.path.join(os.path.dirname(__file__), '..', 'templates', 'bgpd')
    env = Environment(loader=FileSystemLoader(template_dir), trim_blocks=True)
    return env.get_template(template_name).render(**context)


def test_boot_template_programs_a_prefix_set_without_mode():
    """The boot path applies the mode default too.

    docker_init.sh renders these templates through sonic-cfggen at container
    start and on `config reload`, so a mode-less prefix-set skipped here is lost
    on every boot however the daemon behaves afterwards.
    """
    out = _render('bgpd.conf.db.pref_list.j2',
                  PREFIX_SET={'PFS_NOMODE': {}, 'PFS_V6': {'mode': 'IPv6'}},
                  PREFIX={
                      ('PFS_NOMODE', '10', '10.1.0.0/16', 'exact'): {},
                      ('PFS_V6', '10', 'fc00::/64', 'exact'): {'action': 'deny'},
                  })

    assert 'ip prefix-list PFS_NOMODE seq 10 permit 10.1.0.0/16' in out, out
    assert 'ipv6 prefix-list PFS_V6 seq 10 deny fc00::/64' in out, out


def test_boot_template_renders_an_empty_mode_as_ipv4():
    """An empty-string mode is the default too, as it is in the daemon."""
    out = _render('bgpd.conf.db.pref_list.j2',
                  PREFIX_SET={'PFS_EMPTY': {'mode': ''}},
                  PREFIX={('PFS_EMPTY', '10', '10.1.0.0/16', 'exact'): {}})

    assert 'ip prefix-list PFS_EMPTY seq 10 permit 10.1.0.0/16' in out, out


def test_boot_route_map_template_resolves_a_mode_less_prefix_set():
    """The match clause needs the AF token, which mode supplies.

    Without the default the address family renders empty (or the render fails
    outright), so the route-map loses its match clause at boot.
    """
    out = _render('bgpd.conf.db.route_map.j2',
                  ROUTE_MAP={('POLICY', '10'): {
                      'route_operation': 'permit',
                      'match_prefix_set': 'PFS_NOMODE',
                      'match_next_hop_set': 'PFS_NOMODE',
                  }},
                  PREFIX_SET={'PFS_NOMODE': {}})

    assert ' match ip address prefix-list PFS_NOMODE' in out, out
    assert ' match ip next-hop prefix-list PFS_NOMODE' in out, out
