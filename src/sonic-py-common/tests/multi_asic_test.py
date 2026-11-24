from unittest import mock

import pytest

from sonic_py_common import multi_asic


@pytest.mark.parametrize('connect,constructor', [
    (multi_asic.connect_config_db_for_ns, 'ConfigDBConnector'),
    (multi_asic.connect_to_all_dbs_for_ns, 'SonicV2Connector'),
])
@pytest.mark.parametrize('options,native_options', [
    ({}, {}),
    ({'use_tcp': False}, {'use_unix_socket_path': True}),
    ({'use_tcp': False, 'authenticate': True}, {'use_unix_socket_path': True, 'authenticate': True}),
])
def test_namespace_connections_use_the_requested_transport(connect, constructor, options, native_options):
    with mock.patch.object(multi_asic.swsscommon, constructor) as factory, \
            mock.patch.object(multi_asic, 'is_supervisor', return_value=False):
        factory.return_value.get_db_list.return_value = ['CONFIG_DB', 'CHASSIS_APP_DB', 'CHASSIS_STATE_DB']
        result = connect('asic0', **options)
    factory.assert_called_once_with(namespace='asic0', **native_options)
    assert result is factory.return_value
    if constructor == 'ConfigDBConnector':
        result.connect.assert_called_once_with()
    else:
        result.connect.assert_called_once_with('CONFIG_DB')


@pytest.mark.parametrize('use_tcp,authenticate', [(True, False), (False, True)])
@pytest.mark.parametrize('entry_exists', [False, True])
def test_mod_entry_uses_selected_transport_for_existence_check_and_write(use_tcp, authenticate, entry_exists):
    with mock.patch.object(multi_asic, 'get_namespace_list', return_value=['asic0']), \
            mock.patch.object(multi_asic, 'connect_config_db_for_ns') as connect:
        db = connect.return_value
        db.get_entry.return_value = {'admin_status': 'down'} if entry_exists else {}
        multi_asic.mod_entry('PORT', 'Ethernet0', {'admin_status': 'up'}, modIfExists=True,
                             use_tcp=use_tcp, authenticate=authenticate)
    db.get_entry.assert_called_once_with('PORT', 'Ethernet0')
    calls = [mock.call('asic0', use_tcp=use_tcp)]
    if entry_exists:
        calls.append(mock.call('asic0', use_tcp=use_tcp, authenticate=authenticate))
        db.mod_entry.assert_called_once_with('PORT', 'Ethernet0', {'admin_status': 'up'})
    else:
        db.mod_entry.assert_not_called()
    assert connect.call_args_list == calls


class TestMultiAsic:
    def test_get_container_name_from_asic_id(self):
        assert multi_asic.get_container_name_from_asic_id('database', 0) == 'database0'

    def test_get_asic_sub_role(self):
        # Mock asic.conf file
        import textwrap
        mock_asic_conf_content = textwrap.dedent("""
            NUM_ASIC=3
            DEV_ID_ASIC_0=01:00.0
            DEV_ID_ASIC_1=02:00.0
            DEV_ID_ASIC_2=03:00.0
            SUB_ROLE_ASIC_0=FrontEnd
            SUB_ROLE_ASIC_1=BackEnd
        """)

        with mock.patch('sonic_py_common.multi_asic.get_asic_conf_file_path', return_value='/mock/path/asic.conf'):
            with mock.patch('builtins.open', mock.mock_open(read_data=mock_asic_conf_content)):
                assert multi_asic.get_asic_sub_role(0) == 'FrontEnd'
                assert multi_asic.get_asic_sub_role(1) == 'BackEnd'
                assert multi_asic.get_asic_sub_role(2) == None

    def test_get_namespaces_from_linux_current_ns(self):
        # When invoked inside an ASIC namespace, return just that namespace
        # via the early-return path (the sort helper is not needed / not imported here).
        with mock.patch('sonic_py_common.multi_asic.get_current_namespace', return_value='asic0'):
            assert multi_asic.get_namespaces_from_linux() == ['asic0']

    def test_get_namespaces_from_linux_global_ns_sorted(self):
        # In the global namespace, enumerate and naturally-sort all namespaces
        # (exercises the lazily-imported natural-sort path).
        with mock.patch('sonic_py_common.multi_asic.get_current_namespace', return_value=''), \
             mock.patch('glob.glob', return_value=['/run/netns/asic10',
                                                   '/run/netns/asic2',
                                                   '/run/netns/asic1']):
            assert multi_asic.get_namespaces_from_linux() == ['asic1', 'asic2', 'asic10']
