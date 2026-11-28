"""Exercise direct Make requests against archives left by the other producer."""
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
INCLUDE = "include tools/bazel/direct_image_goals.mk"


class DirectImageGoalsTest(unittest.TestCase):
    def test_forward_existing_archives_in_both_build_modes(self):
        fragment = (ROOT / "tools/bazel/direct_image_goals.mk").read_text()
        images = next(line.split(":=", 1)[1].split() for line in fragment.splitlines()
                      if line.startswith("SONIC_DIRECT_BAZEL_IMAGES :="))
        self.assertTrue(images)
        for filename in ("Makefile", "Makefile.work"):
            self.assertIn(INCLUDE, (ROOT / filename).read_text().splitlines())
        # Reproduce the outer implicit rule and the worker rule with its
        # order-only setup prerequisite. Keep expensive worker startup out of the test.
        with tempfile.TemporaryDirectory() as tmp:
            work = Path(tmp)
            (work / "tools").symlink_to(ROOT / "tools", target_is_directory=True)
            (work / "target").mkdir()
            goals = [f"target/{image}{suffix}.gz" for image in images for suffix in ("", "-dbg")]
            unrelated = "target/unrelated.gz"
            for goal in [*goals, unrelated]:
                (work / goal).touch()
            for prereq in ("", " | setup"):
                (work / "Makefile").write_text(
                    "%::" + prereq + "\n\t@echo forwarded-$@\n.PHONY: setup\nsetup:\n" + INCLUDE + "\n")
                for mode in ("y", "n"):
                    with self.subTest(prereq=prereq, bazel=mode):
                        result = subprocess.run(
                            ["make", "--no-print-directory", "-r", "BUILD_WITH_BAZEL_WHEN_AVAILABLE=" + mode,
                             *goals, unrelated], cwd=work, text=True, capture_output=True, check=True)
                        forwarded = [s.removeprefix("forwarded-") for s in result.stdout.splitlines()
                                     if s.startswith("forwarded-")]
                        self.assertEqual(forwarded, goals)
