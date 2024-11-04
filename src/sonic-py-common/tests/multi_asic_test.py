from sonic_py_common import multi_asic


class TestMultiAsic:
    def test_get_container_name_from_asic_id(self):
        assert multi_asic.get_container_name_from_asic_id('database', 0) == 'database0'
