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


def by_purl(frag, prefix):
    return [c for c in frag["components"] if c["purl"].startswith(prefix)]


def prop(component, name):
    return [p["value"] for p in component["properties"] if p["name"] == name]


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
        sha = self.tree.submodule(
            "src/sonic-sysmgr/gnoi", "https://github.com/openconfig/gnoi")
        frag = fragment(self.tree, "sysmgr_1.0.0_amd64.deb", "DPKG_DEB",
                        "src/sonic-sysmgr")
        ancestor = frag["components"][0]["pedigree"]["ancestors"][0]
        self.assertTrue(ancestor["purl"].startswith("pkg:github/openconfig/gnoi@"))
        self.assertEqual(ancestor["externalReferences"][0]["url"],
                         "https://github.com/openconfig/gnoi")
        self.assertEqual(
            [c["purl"] for c in by_purl(frag, "pkg:github/")],
            [f"pkg:github/openconfig/gnoi@{sha[:12]}"],
        )

    def test_a_wheel_names_the_repository_it_was_built_from(self):
        sha = self.tree.submodule(
            "src/sonic-utilities", "https://github.com/sonic-net/sonic-utilities")
        whl = "sonic_utilities-1.2-py3-none-any.whl"
        frag = fragment(self.tree, whl, "PYTHON_WHEEL", "src/sonic-utilities")
        self.assertEqual(frag["components"][0]["purl"],
                         "pkg:pypi/sonic-utilities@1.2")
        [source] = by_purl(frag, "pkg:github/")
        self.assertEqual(source["purl"],
                         f"pkg:github/sonic-net/sonic-utilities@{sha[:12]}")
        self.assertEqual(source["externalReferences"], [{
            "type": "vcs",
            "url": "https://github.com/sonic-net/sonic-utilities",
            "comment": f"submodule pinned to {sha}",
        }])
        self.assertEqual(prop(source, "sonic:source_deb"), [whl])

    def test_a_subdirectory_of_a_submodule_is_built_from_it(self):
        # sonic-dhcp-relay holds a .gitmodules of its own, which does not
        # list sonic-dhcp-relay; the superproject's does.
        sha = self.tree.submodule(
            "src/dhcprelay", "https://github.com/sonic-net/sonic-dhcp-relay.git")
        self.tree.submodule("src/dhcprelay/gmock_global",
                            "https://github.com/sonic-net/gmock-global.git",
                            gitmodules_dir="src/dhcprelay")
        self.tree.directory("src/dhcprelay/dhcp6relay")
        frag = fragment(self.tree, "sonic-dhcp6relay_1.0.0-0_amd64.deb",
                        "DPKG_DEB", "src/dhcprelay/dhcp6relay")
        main = frag["components"][0]
        self.assertEqual(main["purl"],
                         "pkg:deb/sonic/sonic-dhcp6relay@1.0.0-0?arch=amd64")
        self.assertEqual(main["externalReferences"][0]["url"],
                         "https://github.com/sonic-net/sonic-dhcp-relay")
        self.assertEqual(prop(main, "sonic:submodule_commit"), [sha])
        self.assertEqual(
            [c["purl"] for c in by_purl(frag, "pkg:github/")],
            [f"pkg:github/sonic-net/sonic-dhcp-relay@{sha[:12]}"],
        )

    def test_a_wrapper_names_the_submodule_inside_it(self):
        self.tree.directory("src/wpasupplicant")
        sha = self.tree.submodule(
            "src/wpasupplicant/sonic-wpa-supplicant",
            "https://github.com/sonic-net/sonic-wpa-supplicant.git")
        frag = fragment(self.tree, "wpasupplicant_2.9.0-14_amd64.deb",
                        "MAKE_DEB", "src/wpasupplicant")
        self.assertEqual(
            [c["purl"] for c in by_purl(frag, "pkg:github/")],
            [f"pkg:github/sonic-net/sonic-wpa-supplicant@{sha[:12]}"],
        )

    def test_a_debian_rebuild_names_its_repository_beside_its_own_name(self):
        sha = self.tree.submodule(
            "src/sonic-linux-kernel",
            "https://github.com/sonic-net/sonic-linux-kernel")
        frag = fragment(
            self.tree, "linux-image-6.12.41+deb13-sonic-amd64-unsigned_6.12.41-1_amd64.deb",
            "MAKE_DEB", "src/sonic-linux-kernel")
        self.assertTrue(frag["components"][0]["purl"].startswith(
            "pkg:deb/sonic/linux-image-"))
        self.assertEqual(
            [c["purl"] for c in by_purl(frag, "pkg:github/")],
            [f"pkg:github/sonic-net/sonic-linux-kernel@{sha[:12]}"],
        )

    def test_a_deb_from_the_top_of_a_submodule_is_its_repository(self):
        sha = self.tree.submodule(
            "src/sonic-swss", "https://github.com/sonic-net/sonic-swss")
        frag = fragment(self.tree, "swss_1.0.0_amd64.deb", "DPKG_DEB",
                        "src/sonic-swss")
        self.assertEqual(
            [c["purl"] for c in by_purl(frag, "pkg:github/")],
            [f"pkg:github/sonic-net/sonic-swss@{sha[:12]}"],
        )
        self.assertEqual(frag["components"][0]["name"], "swss")

    def test_a_tree_outside_any_submodule_names_none(self):
        # sonic-buildimage checked out as a submodule of something else.
        # src/bash is built from sonic-buildimage, which the build does
        # not record as a submodule of anything.
        outer = self.tree.root
        self.tree.root = os.path.join(outer, "sonic-buildimage")
        os.makedirs(self.tree.root)
        with open(os.path.join(outer, ".gitmodules"), "w") as f:
            f.write('[submodule "sonic-buildimage"]\n\tpath = sonic-buildimage\n'
                    '\turl = https://github.com/sonic-net/sonic-buildimage\n')
        self.tree.submodule(
            "src/sonic-swss", "https://github.com/sonic-net/sonic-swss")
        self.tree.directory("src/bash")
        frag = fragment(self.tree, "bash_5.2.37-2_amd64.deb", "MAKE_DEB",
                        "src/bash")
        self.assertEqual(by_purl(frag, "pkg:github/"), [])
        self.assertNotIn("externalReferences", frag["components"][0])


if __name__ == "__main__":
    unittest.main()
