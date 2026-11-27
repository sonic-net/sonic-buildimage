"""CI scheduling reserves JVM memory while using the runner's effective memory."""

import tempfile
from pathlib import Path
import unittest
from unittest import mock

from tools.bazel.ci import resources


class ResourceTests(unittest.TestCase):
    def test_memory_is_bounded_by_host_and_container_after_heap_reserve(self):
        gib = 1024**3
        for host, limits, expected in ((32, [6], 3276), (32, [16], 11468),
                                      (6, [16], 3276), (16, [], 11468),
                                      (32, [16, 6], 3276)):
            with self.subTest(host=host, limits=limits):
                self.assertEqual(resources.memory_budget_mb(host * gib, [v * gib for v in limits]), expected)
        with self.assertRaisesRegex(ValueError, 'JVM heap reserve'):
            resources.memory_budget_mb(2 * gib, [])
        with self.assertRaisesRegex(ValueError, 'positive'):
            resources.memory_budget_mb(32 * gib, [], heap_mb=0)

    def test_enclosing_and_namespaced_cgroup_limits_are_observed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            proc = root / 'proc'
            (proc / 'self').mkdir(parents=True)
            (proc / 'self/cgroup').write_text('0::/pod/runner\n')
            groups = root / 'groups'
            (groups / 'pod/runner').mkdir(parents=True)
            (groups / 'memory.max').write_text('max\n')
            (groups / 'pod/memory.max').write_text(str(6 * 1024**3))
            (groups / 'pod/runner/memory.max').write_text(str(16 * 1024**3))
            with mock.patch.object(resources.os, 'sysconf', side_effect=lambda key:
                                   {'SC_PHYS_PAGES': 32 * 1024**3 // 4096, 'SC_PAGE_SIZE': 4096}[key]):
                self.assertEqual(resources.action_memory_mb(groups, proc), 3276)
                self.assertEqual(resources.action_memory_mb(groups, proc, heap_mb=4096), 1638)
                (proc / 'self/cgroup').write_text('0::/\n')
                (groups / 'memory.max').write_text(str(16 * 1024**3))
                self.assertEqual(resources.action_memory_mb(groups, proc), 11468)

    def test_configuration_preserves_existing_cache_and_mirror_policy(self):
        with tempfile.TemporaryDirectory() as temp:
            workspace, home = Path(temp) / 'workspace', Path(temp) / 'home'
            workspace.mkdir(); home.mkdir()
            (workspace / '.bazelrc.user').write_text('common --remote_cache=grpc://cache:9092\n')
            (home / '.bazelrc').write_text('common --repository_cache=/cache/repository\n')
            with mock.patch.object(resources, 'action_memory_mb', return_value=3276):
                resources.configure(workspace, home)
            self.assertEqual((workspace / '.bazelrc.user').read_text(),
                'common --remote_cache=grpc://cache:9092\nstartup --host_jvm_args=-Xmx2048m\nbuild --local_ram_resources=3276\n')
            self.assertEqual((home / '.bazelrc').read_text(),
                'common --repository_cache=/cache/repository\nstartup --host_jvm_args=-Xmx2048m\nbuild --local_ram_resources=3276\n')

    def test_custom_heap_is_reserved_and_applied_to_both_rc_files(self):
        with tempfile.TemporaryDirectory() as temp:
            workspace, home = Path(temp) / 'workspace', Path(temp) / 'home'
            workspace.mkdir()
            home.mkdir()
            with mock.patch.object(resources, 'action_memory_mb', return_value=1638) as budget:
                resources.configure(workspace, home, heap_mb=4096)
                budget.assert_called_once_with(heap_mb=4096)
            for rc in (workspace / '.bazelrc.user', home / '.bazelrc'):
                self.assertEqual(rc.read_text(), 'startup --host_jvm_args=-Xmx4096m\n'
                                 'build --local_ram_resources=1638\n')


if __name__ == '__main__':
    unittest.main()
