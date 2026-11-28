import sys
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from generate_vs_hwsku import hwsku_directories, process_hwsku


class VsHwskuOrderTest(unittest.TestCase):
    def test_duplicate_providers_and_existing_virtual_sku(self):
        physical = "x86_64-nvidia_sn5640-r0"
        simulator = "x86_64-nvidia_sn5640_simx-r0"
        expected = {"shared": physical, "Mellanox-SN5640-C512S2": simulator, "virtual": "preseeded"}
        iterdir = Path.iterdir
        with TemporaryDirectory() as tmp:
            device = Path(tmp, "source")
            for provider in (simulator, physical):
                for name in expected:
                    source = device / "mellanox" / provider / name
                    source.mkdir(parents=True)
                    (source / "provider").write_text(provider)
            for reverse in (False, True):
                with self.subTest(reverse=reverse):
                    stage = Path(tmp, str(reverse))
                    platform = stage / "device/x86_64-kvm_x86_64-r0"
                    (platform / "virtual").mkdir(parents=True)
                    (platform / "virtual/provider").write_text("preseeded")
                    with patch.object(Path, "iterdir", lambda path: iter(sorted(iterdir(path), reverse=reverse))):
                        for sku in hwsku_directories(device):
                            process_hwsku(sku, stage, device)
                    self.assertEqual({name: (platform / name / "provider").read_text() for name in expected}, expected)


if __name__ == "__main__":
    unittest.main()
