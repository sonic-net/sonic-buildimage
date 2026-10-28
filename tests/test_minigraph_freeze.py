import importlib.util
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from contextlib import redirect_stdout
from unittest import mock


MODULE_PATH = Path(__file__).parents[1] / "scripts" / "check_minigraph_freeze.py"
SPEC = importlib.util.spec_from_file_location("check_minigraph_freeze", MODULE_PATH)
guard = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(guard)


class DependencyTest(unittest.TestCase):
    def test_python_imports_and_aliases(self):
        before = "from minigraph_ext import (\n    parse_xml as read_graph,\n)\nread_graph(path)\n"
        after = before + "read_graph(other_path)\n"
        added = guard.python_dependencies(after) - guard.python_dependencies(before)
        self.assertTrue(added)
        renamed = before.replace("read_graph", "load_graph")
        self.assertEqual(guard.python_dependencies(before), guard.python_dependencies(renamed))
        self.assertTrue(guard.python_dependencies(
            "from package import minigraph as graph\ngraph.parse_xml(path)"
        ))

    def test_python_literal_importlib_imports(self):
        cases = [
            "import importlib\nparser = importlib.import_module('minigraph')",
            "import importlib as loader\n"
            "loader.import_module(name='minigraph_ext')",
            "from importlib import import_module as load\n"
            "load('package.minigraph_custom')",
            "from importlib import import_module\n"
            "MODULE = 'minigraph'\nimport_module(MODULE)",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertTrue(guard.python_dependencies(source))
        for source in [
            "import importlib\nimportlib.import_module('json')",
            "from tools import import_module as load\nload('minigraph')",
            "import tools as importlib\nimportlib.import_module('minigraph')",
            "from . import importlib\nimportlib.import_module('minigraph')",
            "import importlib as loader\nloader = tools\n"
            "loader.import_module('minigraph')",
            'example = "importlib.import_module(\'minigraph\')"',
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))

    def test_added_reference_to_importlib_parser(self):
        before = (
            "from importlib import import_module as load\n"
            "parser = load('minigraph')\nparser.parse_xml(path)\n"
        )
        after = before + "parser.parse_xml(other_path)\n"
        self.assertTrue(
            guard.python_dependencies(after)
            - guard.python_dependencies(before)
        )
        self.assertEqual(
            guard.python_dependencies(before),
            guard.python_dependencies(before.replace("parser", "graph")),
        )

    def test_python_command_and_xml_consumers(self):
        cases = [
            "import subprocess\nsubprocess.run(['sonic-cfggen', '-H', '-m'])",
            "subprocess.run(args=['sonic-cfggen', '-m'])",
            "cmd = 'config load_minigraph -y'\nhost.shell(cmd)",
            "subprocess.run(f'sonic-cfggen -n {namespace} --minigraph=/tmp/graph.xml')",
            "GRAPH = '/etc/sonic/minigraph.xml'\nET.parse(GRAPH)",
            "Path('/etc/sonic/minigraph.xml').read_text()",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertTrue(guard.python_dependencies(source))

    def test_python_command_import_aliases(self):
        cases = [
            "from subprocess import run as execute\n"
            "execute(['config', 'load_minigraph', '-y'])",
            "from subprocess import check_output as capture\n"
            "capture(args=['sonic-cfggen', '-m'])",
            "import subprocess as process\n"
            "process.run(['sonic-cfggen', '-m'])",
            "from os import system as execute\n"
            "execute('config load_minigraph -y')",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertTrue(guard.python_dependencies(source))
        for source in [
            "from tools import run as execute\n"
            "execute('config load_minigraph -y')",
            "from subprocess import run as execute\n"
            "host.execute('config load_minigraph -y')",
            "from subprocess import run as execute\n"
            "execute(['echo', '$(sonic-cfggen -m)'])",
            "from subprocess import run as execute\nexecute = print\n"
            "execute('config load_minigraph -y')",
            "from subprocess import run as execute\n"
            "def log(execute):\n    execute('config load_minigraph -y')",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))

    def test_comments_docstrings_fixture_strings_and_negative_assertions(self):
        source = '''
"""import minigraph; sonic-cfggen -m"""
# config load_minigraph
fixture = "from minigraph import parse_xml"
expected_command = "config load_minigraph -y"
assert "config load_minigraph" not in commands
'''
        self.assertFalse(guard.python_dependencies(source))
        self.assertFalse(guard.shell_dependencies(
            '# config load_minigraph\n'
            'echo "Do not use sonic-cfggen -m or minigraph.xml"\n'
            '{# minigraph.xml #}\n'
        ))

    def test_shell_and_yaml_entry_points(self):
        cases = [
            "sudo config load_minigraph -y",
            "/usr/local/bin/sonic-cfggen -H \\\n    --minigraph=/tmp/custom.xml",
            'HWSKU=`sonic-cfggen -m -v hwsku`',
            'echo "$(sonic-cfggen -m --print-data)"',
            'echo `sonic-cfggen -m --print-data`',
            "shell: 'config load_minigraph -y'",
            "shell: >\n  sonic-cfggen\n  -H\n  -m\n",
            'bash -c "sonic-cfggen -m"',
            """bash -c 'echo "$(sonic-cfggen -m)"'""",
            "sed -n '/HwSku/p' /etc/sonic/minigraph.xml",
            "cp /host/minigraph.xml /etc/sonic/minigraph.xml",
            "reload_minigraph",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertTrue(guard.shell_dependencies(source))

    def test_literal_shell_diagnostics_do_not_execute_substitutions(self):
        cases = [
            "echo '$(sonic-cfggen -m)'",
            "printf '%s' '`sonic-cfggen -m`'",
            r'echo "\$(sonic-cfggen -m)"',
            r'echo "\`sonic-cfggen -m\`"',
            "echo done # $(sonic-cfggen -m)",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertFalse(guard.shell_dependencies(source))

    def test_python_argv_preserves_literal_arguments(self):
        cases = [
            "subprocess.run(['echo', '$(sonic-cfggen -m)'])",
            "cmd = ['echo', '$(sonic-cfggen -m)']\nsubprocess.run(cmd)",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))
        self.assertTrue(guard.python_dependencies(
            """subprocess.run(['bash', '-c', 'echo "$(sonic-cfggen -m)"'])"""
        ))

    def test_added_use_of_existing_shell_path_variable(self):
        before = 'MINGRAPH_FILE="/etc/sonic/minigraph.xml"\ncat "$MINGRAPH_FILE"\n'
        after = before + 'cat "${MINGRAPH_FILE}"\n'
        self.assertTrue(guard.shell_dependencies(after) - guard.shell_dependencies(before))
        self.assertFalse(guard.shell_dependencies("reload_minigraph()\n{\n}\n"))

    def test_unrelated_commands_do_not_share_flags(self):
        self.assertFalse(guard.shell_dependencies("sonic-cfggen -d; grep -m 1 pattern file"))

    def test_unanalyzable_input_is_not_clean(self):
        with self.assertRaises(guard.AnalysisError):
            guard.python_dependencies("import minigraph\nif")
        with self.assertRaises(guard.AnalysisError):
            guard.shell_dependencies('sonic-cfggen -m "unfinished')


class GitComparisonTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="minigraph-freeze-")
        self.addCleanup(self.directory.cleanup)
        self.repo = Path(self.directory.name)
        self.run_git("init", "--quiet")

    def run_git(self, *args, data=None):
        result = subprocess.run(
            ["git", "-C", str(self.repo), *args], input=data,
            capture_output=True, check=True,
        )
        return result.stdout.decode().strip()

    def tree(self, files):
        self.run_git("read-tree", "--empty")
        for path, content in files.items():
            object_id = self.run_git("hash-object", "-w", "--stdin", data=content.encode())
            self.run_git("update-index", "--add", "--cacheinfo", "100644", object_id, path)
        return self.run_git("write-tree")

    def compare(self, before, after):
        return guard.check_changes(self.repo, self.tree(before), self.tree(after))

    def test_frozen_parser_and_adapters_cannot_be_changed_or_renamed(self):
        for path in sorted(guard.FROZEN_FILES):
            with self.subTest(path=path):
                original = {path: "original"}
                for changed in ({}, {path: "updated"}, {"renamed.py": "original"}):
                    self.assertTrue(self.compare(original, changed))
                self.assertTrue(self.compare({}, original))

    def test_init_defaults_are_not_part_of_minigraph_freeze(self):
        path = "files/build_templates/init_cfg.json.j2"
        self.assertFalse(self.compare({path: "{}"}, {path: '{"FEATURE": {}}'}))

    def test_existing_dependency_and_its_removal_are_allowed(self):
        path = "files/example.sh"
        source = "config load_minigraph -y\n"
        self.assertFalse(self.compare({path: source}, {path: source + "echo done\n"}))
        self.assertFalse(self.compare({path: source}, {path: "config reload -y\n"}))
        self.assertFalse(self.compare({path: source}, {}))

    def test_added_dependency_in_existing_or_new_file_fails(self):
        path = "files/example.sh"
        source = "config load_minigraph -y\n"
        self.assertTrue(self.compare({}, {path: source}))
        self.assertTrue(self.compare({path: source}, {path: source * 2}))
        self.assertTrue(self.compare({path: source}, {"files/new.sh": source}))

    def test_literal_imports_and_command_aliases_are_compared(self):
        path = "scripts/example.py"
        cases = [
            "import importlib\n"
            "parser = importlib.import_module('minigraph')\n",
            "from subprocess import run as execute\n"
            "execute(['sonic-cfggen', '-m'])\n",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertTrue(self.compare({}, {path: source}))
                self.assertFalse(self.compare(
                    {path: source}, {path: source + "# unchanged usage\n"}
                ))
                self.assertFalse(self.compare({path: source}, {}))

    def test_new_shipped_xml_but_not_fixture_or_documentation_fails(self):
        self.assertTrue(self.compare({}, {"device/vendor/minigraph.xml": "<Graph/>"}))
        self.assertFalse(self.compare({}, {
            "tests/fixtures/minigraph.xml": "<Graph/>",
            "docs/retirement.md": "Do not add config load_minigraph.",
        }))

    def test_invalid_revision_fails_closed(self):
        tree = self.tree({})
        with self.assertRaises(guard.AnalysisError):
            guard.check_changes(self.repo, "missing-base", tree)

    def test_failure_explains_rationale_and_links_guide(self):
        base = self.tree({})
        head = self.tree({"files/new.sh": "config load_minigraph -y"})
        output = io.StringIO()
        with mock.patch.object(guard.Path, "cwd", return_value=self.repo), redirect_stdout(output):
            self.assertEqual(1, guard.main(["--base", base, "--head", head]))
        self.assertIn("Minigraph is being retired", output.getvalue())
        self.assertIn("sonic-mgmt", output.getvalue())
        self.assertIn(guard.GUIDE_URL, output.getvalue())

    def test_ci_guide_links_to_the_pr_revision(self):
        output = io.StringIO()
        url = "https://github.com/example/fork/blob/123/.github/workflows/README.md"
        base = self.tree({})
        head = self.tree({"files/new.sh": "config load_minigraph"})
        with mock.patch.dict(guard.os.environ, {"MINIGRAPH_GUIDE_URL": url}), \
                mock.patch.object(guard.Path, "cwd", return_value=self.repo), redirect_stdout(output):
            self.assertEqual(1, guard.main(["--base", base, "--head", head]))
        self.assertIn(url, output.getvalue())

    def test_analysis_failure_does_not_claim_policy_success(self):
        output = io.StringIO()
        with mock.patch.object(guard.Path, "cwd", return_value=self.repo), redirect_stdout(output):
            self.assertEqual(2, guard.main(["--base", "missing", "--head", "missing"]))
        self.assertIn("analysis failure", output.getvalue())
        self.assertNotIn("No frozen parser edits", output.getvalue())


if __name__ == "__main__":
    unittest.main()
