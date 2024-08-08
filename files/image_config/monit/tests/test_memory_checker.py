import unittest
from unittest.mock import patch, MagicMock
import memory_checker

class TestMemoryChecker(unittest.TestCase):

    def test_check_memory_usage(self):
        container_name = "test_container"
        threshold_value = 1000000

        memory_checker.get_container_id = MagicMock(return_value="container_id")
        memory_checker.get_memory_usage = MagicMock(return_value="2000000")
        memory_checker.get_inactive_cache_usage = MagicMock(return_value="500000")

        with self.assertRaises(SystemExit) as cm:
            memory_checker.check_memory_usage(container_name, threshold_value)

        self.assertEqual(cm.exception.code, 3)  
 
    def test_get_container_id(self):
        container_name = "test_container"
        expected_container_id = "container_id"

        memory_checker.get_command_result = MagicMock(return_value="container_id name")
        container_id = memory_checker.get_container_id(container_name)
        self.assertEqual(container_id, expected_container_id)

    def test_get_memory_usage(self):
        container_id = "container_id"
        expected_memory_usage = "1000000"

        memory_checker.os.path.exists = MagicMock(return_value=True)
        memory_checker.open = MagicMock()
        memory_checker.open().__enter__().read.return_value = "1000000"
        memory_usage = memory_checker.get_memory_usage(container_id)
        self.assertEqual(memory_usage, expected_memory_usage)

    def test_get_inactive_cache_usage(self):
        container_id = "container_id"
        expected_cache_usage = "500000"

        memory_checker.os.path.exists = MagicMock(return_value=True)
        memory_checker.open = MagicMock()
        memory_checker.open().__enter__().__iter__.return_value = ["total_inactive_file 500000"]
        cache_usage = memory_checker.get_inactive_cache_usage(container_id)
        self.assertEqual(cache_usage, expected_cache_usage)
  
if __name__ == '__main__':
    unittest.main()
