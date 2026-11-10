"""Native swsscommon regression tests against a dedicated disposable Redis.

Set ISIS_TEST_REDIS_SOCKET to an isolated server's Unix socket. Never point this
at a SONiC production Redis: the fixture clears the dedicated server's DB 4.
"""
import os
import unittest

from sonic_isis import capture, commit


@unittest.skipUnless(os.environ.get('ISIS_TEST_REDIS_SOCKET'), 'requires isolated native SDK Redis')
class TestNativeAtomicSDK(unittest.TestCase):
    def setUp(self):
        from swsscommon.swsscommon import DBConnector
        self.native = DBConnector(4, os.environ['ISIS_TEST_REDIS_SOCKET'], 0)
        self.native.flushdb()
        native = self.native

        class Connector:
            CONFIG_DB = 'CONFIG_DB'

            def get_redis_client(self, database):
                assert database == self.CONFIG_DB
                return native

        self.db = Connector()

    def tearDown(self):
        self.native.flushdb()

    def test_roundtrip_lists_empty_rows_and_atomic_level_updates(self):
        before = capture(self.db)
        row = {'net_titles': ['49.0001.0000.0000.0001.00'], 'enabled': 'false'}
        commit(self.db, before, [('ISIS_INSTANCE', ('default', 'CORE'), row),
                                 ('ISIS_INSTANCE', ('default', 'EMPTY'), {})])
        observed = capture(self.db)
        self.assertEqual(observed['ISIS_INSTANCE'][('default', 'CORE')], row)
        self.assertEqual(observed['ISIS_INSTANCE'][('default', 'EMPTY')], {})
        commit(self.db, observed, [
            ('ISIS_INSTANCE_LEVEL', ('default', 'CORE', level), {'spf_minimum_interval': '7'})
            for level in ('level-1', 'level-2')])
        observed = capture(self.db)
        self.assertEqual(len(observed['ISIS_INSTANCE_LEVEL']), 2)
        commit(self.db, observed, [('ISIS_INSTANCE', ('default', 'EMPTY'), None)])
        self.assertNotIn(('default', 'EMPTY'), capture(self.db)['ISIS_INSTANCE'])

    def test_concurrent_insert_refuses_entire_commit(self):
        before = capture(self.db)
        self.native.hset('ISIS_INSTANCE|default|OTHER', 'enabled', 'false')
        with self.assertRaisesRegex(ValueError, '^IS-IS configuration changed; retry$'):
            commit(self.db, before, [('ISIS_INSTANCE', ('default', 'CORE'), {'enabled': 'false'})])
        observed = capture(self.db)['ISIS_INSTANCE']
        self.assertNotIn(('default', 'CORE'), observed)
        self.assertEqual(observed[('default', 'OTHER')], {'enabled': 'false'})

    def test_concurrent_field_change_refuses_delete(self):
        self.native.hset('ISIS_INSTANCE|default|CORE', 'enabled', 'false')
        before = capture(self.db)
        self.native.hset('ISIS_INSTANCE|default|CORE', 'enabled', 'true')
        with self.assertRaisesRegex(ValueError, 'configuration changed; retry'):
            commit(self.db, before, [('ISIS_INSTANCE', ('default', 'CORE'), None)])
        self.assertEqual(capture(self.db)['ISIS_INSTANCE'][('default', 'CORE')]['enabled'], 'true')

    def test_multi_area_retirement_rejects_entire_atomic_batch(self):
        for name in ('CORE', 'EDGE'):
            self.native.hset('ISIS_INSTANCE|default|' + name, 'enabled', 'true')
        before = capture(self.db)
        for row in (None, {'enabled': 'false'}):
            with self.assertRaisesRegex(ValueError, 'operation refused until FRR is fixed'):
                commit(self.db, before, [
                    ('ISIS_INSTANCE_LEVEL', ('default', 'CORE', 'level-2'), {'spf_minimum_interval': '7'}),
                    ('ISIS_INSTANCE', ('default', 'EDGE'), row)])
            self.assertEqual(capture(self.db), before)
        commit(self.db, before, [
            ('ISIS_INSTANCE', ('default', name), {'enabled': 'false'}) for name in ('CORE', 'EDGE')])
        self.assertTrue(all(row['enabled'] == 'false' for row in capture(self.db)['ISIS_INSTANCE'].values()))


if __name__ == '__main__':
    unittest.main()
