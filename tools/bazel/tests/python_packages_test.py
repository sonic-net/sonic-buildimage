"""Require complete native test evidence for the Python wheels retained by CI."""

import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import python_packages


class PythonPackageEvidenceTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.workspace = Path(self.temporary.name)
        self.directory = self.workspace / "artifacts"
        self.revision = {"tested_head": "tested-revision", "architecture": "amd64"}
        self.wheel_receipts = {
            target: {
                "status": "passed", "architecture": "x86_64", "python": "3.13.9",
                "wheels": [{"filename": name, "sha256": hashlib.sha256(name.encode()).hexdigest()}
                           for name in expected],
            }
            for target, _, _, expected in python_packages.WHEEL_CHECKS
        }
        # The config-engine installation test also installs its YANG dependency.
        # Model the producer's actual receipt independently of collector inputs.
        dependency = "sonic_yang_mgmt-1.0-py3-none-any.whl"
        self.wheel_receipts["sonic_python_wheels_test"]["wheels"].append({
            "filename": dependency, "sha256": hashlib.sha256(dependency.encode()).hexdigest(),
        })

    def fixture(self, *, zipped=False):
        for target in python_packages.TESTS:
            package, name = target.removeprefix("//").split(":")
            source = self.workspace / "bazel-testlogs" / package / name
            source.mkdir(parents=True, exist_ok=True)
            (source / "test.log").write_text("Test passed\n")
            (source / "test.xml").write_text(
                '<testsuite tests="1" failures="0" errors="0" skipped="0">'
                '<testcase name="test_example"/></testsuite>\n')
            contents = {}
            if name.endswith("_full_test"):
                contents["pytest-inventory.json"] = json.dumps({
                    "python": "3.13.9", "collected": 1, "tests": ["test_example"],
                })
            if name in self.wheel_receipts:
                contents["wheels.json"] = json.dumps(self.wheel_receipts[name])
            if contents:
                outputs = source / "test.outputs"
                outputs.mkdir(exist_ok=True)
                if zipped:
                    with zipfile.ZipFile(outputs / "outputs.zip", "w") as archive:
                        for filename, content in contents.items():
                            archive.writestr(filename, content)
                else:
                    for filename, content in contents.items():
                        (outputs / filename).write_text(content)

    def collect(self, architecture="amd64"):
        def export(workspace, directory, receipt, targets, **kwargs):
            paths = {}
            for name in targets:
                paths[name] = directory / name
                paths[name].write_bytes(name.encode())
            return paths

        with mock.patch.object(python_packages.build, "collect_archives", side_effect=export), \
             mock.patch.object(python_packages.platform, "machine", return_value=python_packages.MACHINES[architecture]):
            return python_packages.collect(self.workspace, self.directory, self.revision, architecture)

    def assert_collection_fails(self, message):
        with self.assertRaisesRegex(ValueError, message):
            self.collect()
        receipt = json.loads((self.directory / "receipt.json").read_text())
        self.assertEqual(receipt["status"], "failed")
        self.assertRegex(receipt["error"], message)

    def test_retain_native_results_with_direct_outputs(self):
        """Preserve each required result, collected test inventory and matching wheel hash."""
        self.fixture()
        receipt = self.collect()
        self.assertEqual(receipt["status"], "passed")
        self.assertEqual(receipt["revision"], self.revision)
        self.assertEqual(set(receipt["tests"]), set(python_packages.TESTS))
        self.assertEqual(receipt["python"], "3.13.9")
        for target, filename, receipt_key, _ in python_packages.WHEEL_CHECKS:
            output = self.directory / filename
            self.assertEqual(json.loads(output.read_text()), self.wheel_receipts[target])
            self.assertEqual(receipt[receipt_key], {
                "path": filename, "sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
            })
        for target, evidence in receipt["tests"].items():
            if target.endswith("_full_test"):
                self.assertEqual(json.loads((self.directory / evidence["inventory"]["path"]).read_text())["collected"], 1)

    def test_retain_bazel_zipped_outputs(self):
        """Read the same required receipts when Bazel archives undeclared outputs."""
        self.fixture(zipped=True)
        self.assertEqual(self.collect()["status"], "passed")

    def test_retain_native_arm64_results(self):
        """Each receipt must describe the requested native machine on arm64 too."""
        self.revision["architecture"] = "arm64"
        for receipt in self.wheel_receipts.values():
            receipt["architecture"] = "aarch64"
        self.fixture()
        self.assertEqual(self.collect("arm64")["status"], "passed")

    def test_each_declared_wheel_requires_a_check(self):
        """Every registration is required even when all remaining receipts pass."""
        self.fixture()
        for index, (target, _, _, _) in enumerate(python_packages.WHEEL_CHECKS):
            with self.subTest(target=target), mock.patch.object(
                    python_packages, "WHEEL_CHECKS",
                    python_packages.WHEEL_CHECKS[:index] + python_packages.WHEEL_CHECKS[index + 1:]):
                self.assert_collection_fails("exactly one installed-wheel check")

    def test_duplicate_wheel_check_fails(self):
        """A second registration cannot count as independent coverage of a wheel."""
        self.fixture()
        for check in python_packages.WHEEL_CHECKS:
            with self.subTest(target=check[0]), mock.patch.object(
                    python_packages, "WHEEL_CHECKS", [*python_packages.WHEEL_CHECKS, check]):
                self.assert_collection_fails("exactly one installed-wheel check")

    def test_duplicate_wheel_within_check_fails(self):
        """A wheel must occur once even within the same check's expected inputs."""
        self.fixture()
        for index, (target, filename, receipt_key, expected) in enumerate(python_packages.WHEEL_CHECKS):
            checks = list(python_packages.WHEEL_CHECKS)
            checks[index] = (target, filename, receipt_key, [*expected, expected[0]])
            with self.subTest(target=target), mock.patch.object(python_packages, "WHEEL_CHECKS", checks):
                self.assert_collection_fails("exactly one installed-wheel check")

    def test_undeclared_wheel_check_fails(self):
        """An unknown wheel cannot replace coverage for a declared artifact."""
        self.fixture()
        for index, (target, filename, receipt_key, expected) in enumerate(python_packages.WHEEL_CHECKS):
            checks = list(python_packages.WHEEL_CHECKS)
            checks[index] = (target, filename, receipt_key, ["undeclared.whl", *expected[1:]])
            with self.subTest(target=target), mock.patch.object(python_packages, "WHEEL_CHECKS", checks):
                self.assert_collection_fails("exactly one installed-wheel check")

    def test_wheel_checks_require_test_results(self):
        """A wheel receipt cannot bypass the required successful log and XML."""
        self.fixture()
        for target, _, _, _ in python_packages.WHEEL_CHECKS:
            tests = [test for test in python_packages.TESTS if test != "//tools/bazel/tests:" + target]
            with self.subTest(target=target), mock.patch.object(python_packages, "TESTS", tests):
                self.assert_collection_fails("no required test evidence")

    def test_missing_xml_fails_and_records_failure(self):
        """An artifact upload cannot turn missing source-suite evidence into success."""
        self.fixture()
        (self.workspace / "bazel-testlogs/tools/bazel/tests/sonic_py_common_full_test/test.xml").unlink()
        with self.assertRaisesRegex(ValueError, "Missing required.*test.xml"):
            self.collect()
        self.assertEqual(json.loads((self.directory / "receipt.json").read_text())["status"], "failed")

    def test_missing_inventory_fails(self):
        """A passing test log alone does not establish complete pytest collection."""
        self.fixture()
        (self.workspace / "bazel-testlogs/tools/bazel/tests/sonic_config_engine_full_test/test.outputs/pytest-inventory.json").unlink()
        with self.assertRaises(FileNotFoundError):
            self.collect()
        self.assertEqual(json.loads((self.directory / "receipt.json").read_text())["status"], "failed")

    def test_failed_xml_fails(self):
        """An existing report containing test failures cannot receive a passing receipt."""
        self.fixture()
        report = self.workspace / "bazel-testlogs/tools/bazel/tests/sonic_py_common_full_test/test.xml"
        report.write_text('<testsuite tests="1" failures="1"><testcase name="failed">'
                          '<failure message="failed"/></testcase></testsuite>')
        with self.assertRaisesRegex(ValueError, "unsuccessful test report"):
            self.collect()

    def test_xml_error_with_wrong_summary_fails(self):
        """An error element remains fatal even if its suite summary claims success."""
        self.fixture()
        report = self.workspace / "bazel-testlogs/tools/bazel/tests/sonic_py_common_full_test/test.xml"
        report.write_text('<testsuite tests="1" errors="0"><testcase name="error">'
                          '<error message="failed"/></testcase></testsuite>')
        with self.assertRaisesRegex(ValueError, "unsuccessful test report"):
            self.collect()

    def test_incomplete_xml_fails(self):
        """Summary counts must describe actual case results retained in the report."""
        self.fixture()
        report = self.workspace / "bazel-testlogs/tools/bazel/tests/sonic_py_common_full_test/test.xml"
        report.write_text('<testsuite tests="2"><testcase name="only_one"/></testsuite>')
        with self.assertRaisesRegex(ValueError, "Incomplete"):
            self.collect()

    def test_inventory_count_must_match_xml(self):
        """A valid report for a smaller run cannot represent the collected full suite."""
        self.fixture()
        inventory = self.workspace / "bazel-testlogs/tools/bazel/tests/sonic_py_common_full_test/test.outputs/pytest-inventory.json"
        inventory.write_text(json.dumps({"python": "3.13.9", "collected": 2,
                                         "tests": ["test_example", "test_other"]}))
        with self.assertRaisesRegex(ValueError, "inventory does not match"):
            self.collect()

    def test_inventory_must_include_each_collected_case(self):
        """The retained inventory must name every case represented by its count."""
        self.fixture()
        inventory = self.workspace / "bazel-testlogs/tools/bazel/tests/sonic_py_common_full_test/test.outputs/pytest-inventory.json"
        inventory.write_text(json.dumps({"python": "3.13.9", "collected": 1, "tests": []}))
        with self.assertRaisesRegex(ValueError, "inventory does not match"):
            self.collect()

    def test_source_suite_python_must_match(self):
        """A source result from another interpreter cannot represent native Python 3.13."""
        self.fixture()
        inventory = self.workspace / "bazel-testlogs/tools/bazel/tests/sonic_py_common_full_test/test.outputs/pytest-inventory.json"
        inventory.write_text(json.dumps({"python": "3.12.9", "collected": 1,
                                         "tests": ["test_example"]}))
        with self.assertRaisesRegex(ValueError, "different Python version"):
            self.collect()

    def test_different_wheel_bytes_fail(self):
        """Retained wheels must be the bytes exercised by the installation test."""
        for target, receipt in self.wheel_receipts.items():
            for wheel in receipt["wheels"]:
                with self.subTest(target=target, wheel=wheel["filename"]), mock.patch.dict(wheel, sha256="0" * 64):
                    self.fixture()
                    self.assert_collection_fails("Retained wheels differ")

    def test_other_architecture_receipt_fails(self):
        """A successful result from another architecture cannot stand in for this run."""
        for target, receipt in self.wheel_receipts.items():
            with self.subTest(target=target), mock.patch.dict(receipt, architecture="aarch64"):
                self.fixture()
                self.assert_collection_fails("successful native run")

    def test_failed_wheel_receipt_fails(self):
        """Matching wheel hashes cannot turn a failed installation test into success."""
        for target, receipt in self.wheel_receipts.items():
            with self.subTest(target=target), mock.patch.dict(receipt, status="failed"):
                self.fixture()
                self.assert_collection_fails("successful native run")

    def test_wheel_receipt_requires_python_313(self):
        """Each wheel check must use the required interpreter version."""
        for target, receipt in self.wheel_receipts.items():
            with self.subTest(target=target), mock.patch.dict(receipt, python="3.12.9"):
                self.fixture()
                self.assert_collection_fails("different Python version")

    def test_wheel_receipts_require_same_python_version(self):
        """Individually valid checks must still describe the same Python runtime."""
        for target, receipt in self.wheel_receipts.items():
            with self.subTest(target=target), mock.patch.dict(receipt, python="3.13.10"):
                self.fixture()
                self.assert_collection_fails("different Python versions")

    def test_missing_wheel_in_receipt_fails(self):
        """Each receipt must identify every wheel registered to its check."""
        for target, receipt in self.wheel_receipts.items():
            with self.subTest(target=target), mock.patch.dict(receipt, wheels=receipt["wheels"][1:]):
                self.fixture()
                self.assert_collection_fails("Retained wheels differ")

    def test_missing_dependency_wheel_in_receipt_fails(self):
        """An owning YANG check cannot replace its hash in config-engine's inputs."""
        receipt = self.wheel_receipts["sonic_python_wheels_test"]
        receipt["wheels"] = [wheel for wheel in receipt["wheels"]
                             if wheel["filename"] != "sonic_yang_mgmt-1.0-py3-none-any.whl"]
        self.fixture()
        self.assert_collection_fails("Retained wheels differ")

    def test_unexpected_wheel_in_receipt_fails(self):
        """A receipt cannot substitute an undeclared wheel for its expected input."""
        for target, receipt in self.wheel_receipts.items():
            wheel = receipt["wheels"][0]
            with self.subTest(target=target), mock.patch.dict(wheel, filename="unexpected.whl"):
                self.fixture()
                self.assert_collection_fails("Retained wheels differ")

    def test_duplicate_wheel_in_receipt_fails(self):
        """A duplicate entry must not disappear when receipt hashes are compared."""
        for target, receipt in self.wheel_receipts.items():
            with self.subTest(target=target), mock.patch.dict(
                    receipt, wheels=[*receipt["wheels"], receipt["wheels"][0]]):
                self.fixture()
                self.assert_collection_fails("Retained wheels differ")


if __name__ == "__main__":
    unittest.main()
