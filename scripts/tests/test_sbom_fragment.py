import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPTS = Path(__file__).parents[1]
sys.path.insert(0, str(SCRIPTS))
SPEC = importlib.util.spec_from_file_location(
    "sbom_fragment", SCRIPTS / "sbom_fragment.py"
)
sbom_fragment = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(sbom_fragment)


def git(cwd, *args):
    return subprocess.run(
        ["git", "-c", "user.name=t", "-c", "user.email=t@example.com",
         "-c", "init.defaultBranch=main", *args],
        cwd=cwd, check=True, capture_output=True, text=True,
    ).stdout.strip()


class Tree:
    """A superproject on disk with submodules checked out in it.

    Each submodule is a repository of its own with one commit, listed in
    the .gitmodules of the directory that holds it, which is what
    sbom_fragment.py reads; nothing is cloned.
    """

    def __init__(self, root):
        self.root = root
        self.commits = {}

    def submodule(self, path, url, gitmodules_dir=""):
        work = os.path.join(self.root, path)
        os.makedirs(work, exist_ok=True)
        git(work, "init", "-q")
        git(work, "commit", "-q", "--allow-empty", "-m", "c")
        self.commits[path] = git(work, "rev-parse", "HEAD")
        holder = os.path.join(self.root, gitmodules_dir)
        rel = os.path.relpath(work, holder)
        with open(os.path.join(holder, ".gitmodules"), "a") as f:
            f.write(f'[submodule "{rel}"]\n\tpath = {rel}\n\turl = {url}\n')
        return self.commits[path]

    def directory(self, path):
        os.makedirs(os.path.join(self.root, path), exist_ok=True)


def fragment(tree, artifact, recipe_type, src_path):
    env = {"SRC_PATH": src_path, "DEPENDS": "", "RDEPENDS": "",
           "MAIN_DEB": "", "URL": "", "ARTIFACT_LICENSE": ""}
    cwd = os.getcwd()
    os.chdir(tree.root)
    try:
        with mock.patch.dict(os.environ, env):
            return sbom_fragment.build_fragment(
                os.path.join("target", artifact), recipe_type,
            )
    finally:
        os.chdir(cwd)


class SubmoduleRepositoryTest(unittest.TestCase):

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tree = Tree(os.path.realpath(self._tmp.name))

    def tearDown(self):
        self._tmp.cleanup()

    def test_only_a_literal_git_suffix_is_removed(self):
        sha = self.tree.submodule(
            "src/sonic-gnmi", "https://github.com/sonic-net/sonic-gnmi.git")
        frag = fragment(self.tree, "sonic-gnmi_0.1_amd64.deb", "DPKG_DEB",
                        "src/sonic-gnmi")
        main = frag["components"][0]
        self.assertEqual(main["purl"],
                         f"pkg:github/sonic-net/sonic-gnmi@{sha[:12]}")
        self.assertEqual(main["externalReferences"][0]["url"],
                         "https://github.com/sonic-net/sonic-gnmi")

    def test_a_nested_upstream_keeps_its_name(self):
        self.tree.directory("src/sonic-sysmgr")
        self.tree.submodule(
            "src/sonic-sysmgr/gnoi", "https://github.com/openconfig/gnoi")
        frag = fragment(self.tree, "sysmgr_1.0.0_amd64.deb", "DPKG_DEB",
                        "src/sonic-sysmgr")
        ancestor = frag["components"][0]["pedigree"]["ancestors"][0]
        self.assertTrue(ancestor["purl"].startswith("pkg:github/openconfig/gnoi@"))
        self.assertEqual(ancestor["externalReferences"][0]["url"],
                         "https://github.com/openconfig/gnoi")
        self.assertEqual(ancestor["name"], "gnoi")


if __name__ == "__main__":
    unittest.main()
