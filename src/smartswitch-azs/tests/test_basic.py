"""
Basic tests for smartswitch-azs package
"""
import unittest
import sys
import os

# Add the parent directory to import modules
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

# Import the modules.
try:
    import smartswitch-azs
    SMARTSWITCH_IMPORTED = True
except ImportError:
    SMARTSWITCH_IMPORTED = False

class TestSmartSwitchAZS(unittest.TestCase):
    """Basic test class for smartswitch-azs"""

    def test_imports(self):
        """Test modules can be imported"""
        self.assertTrue(SMARTSWITCH_IMPORTED, "smartswitch-azs module imported successsfully.")
    
    def test_dummy(self):
        self.assertTrue(True, "Dummy test passed.")

if __name__== "__main__":
    unittest.main()