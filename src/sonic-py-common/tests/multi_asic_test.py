from unittest import mock
from sonic_py_common import multi_asic


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
