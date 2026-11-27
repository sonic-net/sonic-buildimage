"""Exercise gitlink-based source selection with real parent and producer commits."""

import ast
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from tools.bazel import submodule_provenance as sources


class RuntimeImageGitlinksTest(unittest.TestCase):
    def test_runtime_receipts_use_the_selected_producer_gitlinks(self):
        root = Path(__file__).resolve().parents[3]

        def bindings(path, target, field):
            for statement in ast.parse(path.read_text()).body:
                if not isinstance(statement, ast.Expr) or not isinstance(statement.value, ast.Call):
                    continue
                args = {kw.arg: kw.value for kw in statement.value.keywords}
                if isinstance(args.get("name"), ast.Constant) and args["name"].value == target:
                    return ast.literal_eval(args[field])
            self.fail(f"Missing {target} in {path}")

        module = root / "MODULE.bazel"
        identities = bindings(module, "image_gitlinks", "modules")
        pins = sources.gitlinks(root)
        for image, owner in (("docker-database", "sonic-dash-api"), ("docker-dash-ha", "sonic-dash-ha"),
                             ("docker-dhcp-relay", "sonic-dhcp-relay"), ("docker-mux", "sonic-linkmgrd")):
            with self.subTest(image=image, owner=owner):
                build = root / "dockers" / image / "BUILD.bazel"
                label = bindings(build, "installed_packages", "source_modules")[owner]
                repo, filename = label.removeprefix("@").split("//:")
                self.assertEqual(filename, owner + ".MODULE.bazel")
                self.assertEqual(bindings(module, repo, "modules")["@image_gitlinks//:" + filename], owner)
                self.assertEqual(identities["@" + owner.replace("-", "_") + "//:MODULE.bazel"], owner)
                self.assertIn(owner, pins)


class GitlinkSourcesTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.base = Path(temp.name)
        self.root = self.base / "parent"
        self.owner = self.base / "producer"
        self.checkout = self.root / "src/producer"
        self.module = self.checkout / "native/MODULE.bazel"
        for repo in (self.root, self.owner):
            repo.mkdir()
            self.git(repo, "init", "-q")
            self.git(repo, "config", "user.name", "Gitlink test")
            self.git(repo, "config", "user.email", "test@example.com")
        (self.owner / "native").mkdir()
        (self.owner / "native/MODULE.bazel").write_text('module(name = "example", version = "0.0.0")\n')
        self.git(self.owner, "add", ".")
        self.git(self.owner, "commit", "-qm", "producer")
        self.git(self.root, "-c", "protocol.file.allow=always", "submodule", "add", "-q",
                 str(self.owner), "src/producer")
        (self.root / "MODULE.bazel").write_text(
            'local_path_override(module_name = "example", path = "src/producer/native")\n'
            'local_path_override(module_name = "in-tree", path = "src/in-tree")\n')
        self.git(self.root, "add", ".")
        self.git(self.root, "commit", "-qm", "pin")
        self.revision = self.git(self.owner, "rev-parse", "HEAD")

    @staticmethod
    def git(root, *args):
        return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()

    def test_reads_nested_module_at_committed_gitlink(self):
        self.assertEqual(sources.gitlinks(self.root), {"example": ("src/producer", "src/producer/native", self.revision)})
        receipt = sources.identity(self.root, "example", self.module)
        self.assertIn('version = "gitlink-' + self.revision + '"', receipt["manifest"])
        self.assertIn(self.git(self.checkout, "rev-parse", "--path-format=absolute", "--git-path", "HEAD"),
                      receipt["watch"])
        with self.assertRaisesRegex(ValueError, "different checkout"):
            sources.identity(self.root, "example", self.owner / "native/MODULE.bazel")

    def test_receipt_after_git_submodule_update(self):
        fresh = self.base / "fresh"
        self.git(self.base, "clone", "-q", str(self.root), str(fresh))
        module = fresh / "src/producer/native/MODULE.bazel"
        with self.assertRaisesRegex(ValueError, "git submodule update --init -- src/producer"):
            sources.identity(fresh, "example", module)
        self.git(fresh, "-c", "protocol.file.allow=always", "submodule", "update", "--quiet", "--init", "--", "src/producer")
        receipt = json.loads(subprocess.check_output(
            [sys.executable, sources.__file__, "--root", fresh, "example", module], text=True))
        self.assertIn("gitlink-" + self.revision, receipt["manifest"])

    def test_identity_rejects_modified_module(self):
        self.module.write_text("module(name = 'modified')\n")
        with self.assertRaisesRegex(ValueError, "Modified producer module"):
            sources.identity(self.root, "example", self.module)

    def test_new_gitlink_changes_identity_even_with_same_module_bytes(self):
        (self.owner / "source.c").write_text("int value;\n")
        self.git(self.owner, "add", "source.c")
        self.git(self.owner, "commit", "-qm", "source-only update")
        self.git(self.checkout, "fetch", "-q", "origin")
        self.git(self.checkout, "checkout", "-q", "FETCH_HEAD")
        with self.assertRaisesRegex(ValueError, "must be checked out"):
            sources.identity(self.root, "example", self.module)
        self.git(self.root, "add", "src/producer")
        self.git(self.root, "commit", "-qm", "advance gitlink")
        revision = self.git(self.owner, "rev-parse", "HEAD")
        self.assertIn("gitlink-" + revision, sources.identity(self.root, "example", self.module)["manifest"])


if __name__ == "__main__":
    unittest.main()
