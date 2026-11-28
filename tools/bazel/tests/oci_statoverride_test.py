"""Test preservation and conflict detection of inherited permission records."""
import unittest
from tools.bazel.oci.statoverride import merge_entry

ENTRY = '_lldpd adm 4750 /usr/sbin/lldpcli'


class StatoverrideTest(unittest.TestCase):
    def test_append_preserves_existing_bytes(self):
        inherited = b'root\tcrontab 2755 /usr/bin/crontab\n'
        self.assertEqual(merge_entry(inherited, ENTRY), inherited + (ENTRY + '\n').encode())
        self.assertEqual(merge_entry(inherited.rstrip(), ENTRY), inherited + (ENTRY + '\n').encode())

    def test_identical_entry_is_idempotent(self):
        inherited = (ENTRY + '\nroot root 4755 /bin/su\n').encode()
        self.assertEqual(merge_entry(inherited, ENTRY), inherited)

    def test_conflicting_permission_entry_fails(self):
        with self.assertRaisesRegex(ValueError, 'Conflicting'):
            merge_entry(b'root root 0755 /usr/sbin/lldpcli\n', ENTRY)

    def test_duplicate_and_invalid_records_fail(self):
        for content in ((ENTRY + '\n' + ENTRY + '\n').encode(), b'not a record\n'):
            with self.assertRaises(ValueError):
                merge_entry(content, ENTRY)
        for entry in ('root root 8888 /bin/thing', 'root root 4755 relative', ENTRY + '\n'):
            with self.assertRaises(ValueError):
                merge_entry(b'', entry)


if __name__ == '__main__':
    unittest.main()
