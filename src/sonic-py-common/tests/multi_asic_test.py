from unittest import mock
from sonic_py_common import multi_asic
import pytest


@pytest.mark.parametrize('namespace', ['', 'asic0'])
@pytest.mark.parametrize('authenticate,use_socket', [(False, False), (False, True), (True, False)])
def test_config_connection_preserves_reader_or_explicit_writer(namespace, authenticate, use_socket):
    with mock.patch.object(multi_asic, 'use_local_redis_socket', return_value=use_socket) as select, \
            mock.patch.object(multi_asic.swsscommon, 'ConfigDBConnector') as connector:
        result = multi_asic.connect_config_db_for_ns(namespace, authenticate=authenticate)
    options = {'namespace': namespace, 'use_unix_socket_path': authenticate or use_socket}
    if authenticate:
        options['authenticate'] = True
        select.assert_not_called()
    else:
        select.assert_called_once_with('CONFIG_DB', namespace)
    connector.assert_called_once_with(**options)
    result.connect.assert_called_once_with()


@pytest.mark.parametrize('all_local', [False, True])
@pytest.mark.parametrize('supervisor', [False, True])
def test_all_database_reader_keeps_remote_tcp_and_chassis_filter(all_local, supervisor):
    names = ['CONFIG_DB', 'STATE_DB', 'CHASSIS_APP_DB', 'CHASSIS_STATE_DB']
    with mock.patch.object(multi_asic.swsscommon.SonicDBConfig, 'getDbList', return_value=names), \
            mock.patch.object(multi_asic.swsscommon, 'SonicV2Connector') as factory, \
            mock.patch.object(multi_asic, 'use_local_redis_socket', return_value=all_local), \
            mock.patch.object(multi_asic, 'is_supervisor', return_value=supervisor):
        result = multi_asic.connect_to_all_dbs_for_ns('asic0')
    assert result is factory.return_value
    expected = names if supervisor else names[:2]
    assert result.connect.call_args_list == [mock.call(name) for name in expected]
    factory.assert_called_once_with(namespace='asic0', use_unix_socket_path=all_local)


def test_all_database_reader_does_not_apply_config_socket_choice_to_remote_db():
    names = ['CONFIG_DB', 'CHASSIS_STATE_DB']
    with mock.patch.object(multi_asic.swsscommon.SonicDBConfig, 'getDbList', return_value=names), \
            mock.patch.object(multi_asic.swsscommon, 'SonicV2Connector') as factory, \
            mock.patch.object(multi_asic, 'use_local_redis_socket',
                              side_effect=lambda name, namespace: name == 'CONFIG_DB'), \
            mock.patch.object(multi_asic, 'is_supervisor', return_value=True):
        multi_asic.connect_to_all_dbs_for_ns()
    factory.assert_called_once_with(namespace='', use_unix_socket_path=False)


def test_mod_entry_requests_writer_authentication():
    with mock.patch.object(multi_asic, 'get_namespace_list', return_value=['asic0']), \
            mock.patch.object(multi_asic, 'connect_config_db_for_ns') as connect:
        multi_asic.mod_entry('PORT', 'Ethernet0', {'admin_status': 'up'})
    connect.assert_called_once_with('asic0', authenticate=True)
    connect.return_value.mod_entry.assert_called_once_with('PORT', 'Ethernet0', {'admin_status': 'up'})


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
