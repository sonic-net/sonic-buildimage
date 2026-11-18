import fnmatch
import importlib.util
import io
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import textwrap
import unittest
from contextlib import redirect_stdout
from unittest import mock

import yaml


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

    def test_relative_parser_imports_and_aliases(self):
        for module in sorted(guard.PARSER_MODULES):
            with self.subTest(module=module):
                source = (
                    "from . import {} as parser\n"
                    "parser.parse_xml(path)\n".format(module)
                )
                self.assertTrue(guard.python_dependencies(source))
                self.assertEqual(
                    guard.python_dependencies(source),
                    guard.python_dependencies(source.replace(
                        "as parser", "as graph"
                    ).replace("parser.parse_xml", "graph.parse_xml")),
                )
                self.assertTrue(
                    guard.python_dependencies(
                        source + "parser.parse_xml(other)\n"
                    ) - guard.python_dependencies(source)
                )
                self.assertTrue(guard.python_dependencies(
                    "from .. import {}".format(module)
                ))
        self.assertFalse(guard.python_dependencies(
            "from . import json\njson.loads(data)"
        ))

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
            "subprocess.run(f'sonic-cfggen -n {namespace} "
            "--minigraph=/tmp/graph.xml', shell=True)",
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

    def test_imported_file_and_helper_aliases_preserve_reference_counts(self):
        for original, renamed in [
            ("from builtins import open\nopen(file='/etc/sonic/minigraph.xml')",
             "from builtins import open as read_graph\n"
             "read_graph(file='/etc/sonic/minigraph.xml')"),
            ("from pathlib import Path\n"
             "GRAPH = Path('/etc/sonic')\nPath(GRAPH, 'minigraph.xml').read_text()",
             "from pathlib import Path as P\n"
             "GRAPH = P('/etc/sonic')\nP(GRAPH, 'minigraph.xml').read_text()"),
            ("from pathlib import PurePath\nPurePath('/etc/sonic/minigraph.xml')",
             "from pathlib import PurePath as P\nP('/etc/sonic/minigraph.xml')"),
            ("from tools import load_minigraph\nload_minigraph(host)",
             "from tools import load_minigraph as load\nload(host)"),
            ("from tools import reload_minigraph\nreload_minigraph(host)",
             "from tools import reload_minigraph as reload\nreload(host)"),
            ("import pathlib\npathlib.Path('/etc/sonic/minigraph.xml').exists()",
             "import pathlib as p\np.Path('/etc/sonic/minigraph.xml').exists()"),
        ]:
            with self.subTest(source=renamed):
                expected = guard.python_dependencies(original)
                self.assertTrue(expected)
                self.assertEqual(expected, guard.python_dependencies(renamed))
        for source in [
            "from builtins import open as read_graph\nread_graph = print\n"
            "read_graph('/etc/sonic/minigraph.xml')",
            "from pathlib import Path as P\nP = diagnostic\n"
            "P('/etc/sonic/minigraph.xml').read_text()",
            "from tools import load_minigraph as load\n"
            "def diagnostic(load):\n    load(host)",
            "from builtins import open as read_graph\n"
            "read_graph('/etc/sonic/config_db.json')",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))
        self.assertTrue(guard.python_dependencies(
            "from pathlib import Path as P\n"
            "subprocess.run(['cat', P('/etc/sonic/minigraph.xml')])"
        ))

    def test_noshell_helpers_preserve_each_pipeline_stage(self):
        for helper in (
            "getstatusoutput_noshell", "getstatusoutput_noshell_pipe",
            "check_output_pipe",
        ):
            prefix = "from sonic_py_common.general import {} as execute\n".format(helper)
            for arguments, expected in [
                ("['sonic-cfggen', '-m']", True),
                ("('sonic-cfggen', '-m')", True),
                ("'sonic-cfggen -m'", False),
                ("['echo', 'sonic-cfggen -m']", False),
                ("['printf', '%s\\n', '$(sonic-cfggen -m)']", False),
            ]:
                with self.subTest(helper=helper, arguments=arguments):
                    self.assertEqual(expected, bool(guard.python_dependencies(
                        prefix + "execute({})".format(arguments)
                    )))
            keyword = "cmd" if helper == "getstatusoutput_noshell" else "cmd0"
            self.assertTrue(guard.python_dependencies(
                prefix + "execute({}=['sonic-cfggen', '-m'])".format(keyword)
            ))
        for helper in ("getstatusoutput_noshell_pipe", "check_output_pipe"):
            for stages, expected in [
                ("['sonic-cfggen', '-m'], ['cat']", True),
                ("['echo', 'data'], ['sonic-cfggen', '-m']", True),
                ("['echo', 'data'], ['cat'], ['sonic-cfggen', '-m']", True),
                ("['echo', 'data'], 'sonic-cfggen -m'", False),
                ("'sonic-cfggen -m', ['cat']", False),
            ]:
                with self.subTest(helper=helper, stages=stages):
                    self.assertEqual(expected, bool(guard.python_dependencies(
                        "from sonic_py_common import general\n"
                        "general.{}({})".format(helper, stages)
                    )))
            self.assertEqual(2, guard.python_dependencies(
                "{}(['sonic-cfggen', '-m'], ['sonic-cfggen', '-m'])".format(helper)
            )[("command", "sonic-cfggen minigraph mode")])

    def test_python_bytes_are_normalized_only_at_supported_api_boundaries(self):
        for source in [
            "subprocess.run([b'sonic-cfggen', b'-m'])",
            "subprocess.run(['sonic-cfggen', b'-m'])",
            "from subprocess import run as execute\n"
            "CMD = b'sonic-' + b'cfggen'\nexecute([CMD] + [b'-m'])",
            "subprocess.run(b'sonic-cfggen -m', shell=True)",
            "subprocess.run([b'sonic-cfggen -m'], shell=True)",
            "subprocess.run([b'alias', b'-m'], executable=b'sonic-cfggen')",
            "subprocess.run(b'sonic-cfggen -m', shell=True, executable=b'/bin/bash')",
            "subprocess.getoutput(b'sonic-cfggen -m')",
            "os.system(b'sonic-cfggen -m')",
            "getstatusoutput_noshell([b'sonic-cfggen', b'-m'])",
            "check_output_pipe([b'echo', b'data'], [b'sonic-cfggen', b'-m'])",
        ]:
            with self.subTest(source=source):
                self.assertEqual(
                    guard.python_dependencies("subprocess.run(['sonic-cfggen', '-m'])"),
                    guard.python_dependencies(source),
                )
        for source in [
            "open(b'/etc/sonic/minigraph.xml')",
            "from builtins import open as read_graph\n"
            "BASE = b'/etc/sonic/'\nread_graph(BASE + b'minigraph.xml')",
            "from os.path import exists as present\npresent(b'/etc/sonic/minigraph.xml')",
            "ET.parse(b'/etc/sonic/minigraph.xml')",
        ]:
            with self.subTest(source=source):
                self.assertEqual(
                    guard.python_dependencies("open('/etc/sonic/minigraph.xml')"),
                    guard.python_dependencies(source),
                )
        for source in [
            "fixture = b'sonic-cfggen -m'",
            "subprocess.run(b'sonic-cfggen -m')",
            "subprocess.run([b'echo', b'$(sonic-cfggen -m)'])",
            "getstatusoutput_noshell(b'sonic-cfggen -m')",
            "os.popen(b'sonic-cfggen -m')",
            "Path(b'/etc/sonic/minigraph.xml')",
            "Path(b'/etc/sonic', 'minigraph.xml').read_text()",
            "from pathlib import PurePath as P\nP(b'/etc/sonic', 'minigraph.xml')",
            "import importlib\nimportlib.import_module(b'minigraph')",
            "Path('/tmp/output').write_bytes(b'minigraph.xml')",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))
        for source in [
            r"open(b'/tmp/\xff/minigraph.xml')",
            r"subprocess.run([b'/tmp/\xff/sonic-cfggen', b'-m'])",
            "host.shell(b'sonic-cfggen -m')",
            "host.command([b'sonic-cfggen', b'-m'])",
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.python_dependencies(source)

    def test_python_file_apis_exclude_payloads_and_nonpath_options(self):
        for source in [
            "Path('/tmp/notice.txt').write_text('minigraph.xml')",
            "Path('/tmp/notice.txt').write_text(data='minigraph.xml')",
            "TEXT = 'minigraph.xml'\nPath('/tmp/notice.txt').write_text(TEXT)",
            "from pathlib import Path as P\n"
            "notice = P('/tmp/notice.txt')\nnotice.write_text('minigraph.xml')",
            "Path.write_text(Path('/tmp/notice.txt'), 'minigraph.xml')",
            "Path.write_text(self=Path('/tmp/notice.txt'), data='minigraph.xml')",
            "Path('/tmp/notice.txt').write_bytes(b'minigraph.xml')",
            "Path('/tmp/notice.txt').read_text(errors='minigraph.xml')",
            "Path('/tmp/notice.txt').open(errors='minigraph.xml')",
            "Path.open(Path('/tmp/notice.txt'), errors='minigraph.xml')",
            "open('/tmp/notice.txt', errors='minigraph.xml')",
            "from builtins import open as read_file\n"
            "read_file('/tmp/notice.txt', errors='minigraph.xml')",
            "ET.parse('/tmp/notice.xml', parser='minigraph.xml')",
            "shutil.copyfile('/tmp/a', '/tmp/b', follow_symlinks='minigraph.xml')",
            "Path('/tmp/minigraph.xml', '/tmp/notice.txt').read_text()",
            "Path('/tmp/notice.txt', unused='minigraph.xml')",
            "import ast\nast.parse('minigraph.xml')",
            "from ast import parse as parse_source\nparse_source('minigraph.xml')",
            "import copy\ncopy.copy('minigraph.xml')",
            "from copy import copy as duplicate\nduplicate(b'minigraph.xml')",
            "host.copy(dest='/tmp/notice.txt', content='minigraph.xml')",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))
        for method, arguments in [
            ("write_text", "data='minigraph.xml'"),
            ("write_bytes", "data=b'minigraph.xml'"),
            ("read_text", "errors='minigraph.xml'"),
            ("open", "errors='minigraph.xml'"),
            ("read_bytes", ""), ("exists", ""), ("is_file", ""), ("unlink", "missing_ok=True"),
        ]:
            with self.subTest(method=method, arguments=arguments):
                self.assertEqual(
                    guard.python_dependencies("Path('/tmp/minigraph.xml').read_text()"),
                    guard.python_dependencies(
                        "Path('/tmp/minigraph.xml').{}({})".format(method, arguments)
                    ),
                )

    def test_python_file_api_path_selection_preserves_real_references(self):
        for source in [
            "open(file='/tmp/minigraph.xml')",
            "os.open(path=b'/tmp/minigraph.xml', flags=0)",
            "os.remove(path='/tmp/minigraph.xml')",
            "os.unlink(path='/tmp/minigraph.xml')",
            "os.path.exists(path='/tmp/minigraph.xml')",
            "os.path.isfile(path='/tmp/minigraph.xml')",
            "ET.parse(source='/tmp/minigraph.xml')",
            "minidom.parse(file='/tmp/minigraph.xml')",
            "shutil.copyfile(src='/tmp/minigraph.xml', dst='/tmp/notice.xml')",
            "shutil.copy2('/tmp/notice.xml', dst=b'/tmp/minigraph.xml')",
            "shutil.move(src='/tmp/notice.xml', dst='/tmp/minigraph.xml')",
            "host.copy(dest='/tmp/minigraph.xml', content='notice')",
            "open('/tmp/notice.txt', opener=lambda name, flags: os.open('/tmp/minigraph.xml', flags))",
        ]:
            with self.subTest(source=source):
                self.assertEqual(
                    guard.python_dependencies("open('/tmp/minigraph.xml')"),
                    guard.python_dependencies(source),
                )
        self.assertTrue(guard.python_dependencies(
            "Path('/tmp/notice.txt').write_text(subprocess.check_output(['sonic-cfggen', '-m']))"
        ))
        for source in [
            "Path('/tmp', 'minigraph.xml').read_text()",
            "from pathlib import Path as P\nP('/tmp', 'minigraph.xml').read_text()",
            "Path.read_text(Path('/tmp/minigraph.xml'))",
            "Path.open(self=Path('/tmp/minigraph.xml'))",
            "p = Path('/tmp/minigraph.xml')\np.write_text('notice')",
        ]:
            with self.subTest(source=source):
                self.assertEqual(
                    guard.python_dependencies("Path('/tmp/minigraph.xml').read_text()"),
                    guard.python_dependencies(source),
                )

    def test_unresolved_execution_boundaries_do_not_return_clean(self):
        for source in [
            "subprocess.run(['alias', '-m'], executable='sonic-cfggen', shell=mode)",
            "subprocess.run(['alias', '-m'], executable=b'sonic-cfggen', **options)",
            "subprocess.run(['bash', options, '-c', 'sonic-cfggen -m'])",
            "subprocess.run(['sudo', options, 'sonic-cfggen', '-m'])",
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.python_dependencies(source)
        for source in [
            "bash $OPTIONS -c 'sonic-cfggen -m'",
            "sudo $OPTIONS sonic-cfggen -m",
            "bash -- $SCRIPT <<'EOF'\nsonic-cfggen -m\nEOF\n",
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.shell_dependencies(source)
        for source in [
            "subprocess.run(['echo', options, 'sonic-cfggen -m'])",
            "subprocess.run(['bash', '/tmp/script.sh', 'sonic-cfggen -m'])",
            "subprocess.run(['alias', '-m'], executable='/bin/echo', shell=mode)",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))

    def test_cfggen_options_respect_bundles_abbreviations_and_operands(self):
        for arguments, expected in [
            (["-Hm"], True),
            (["-dHm/tmp/graph.xml"], True),
            (["--m"], True),
            (["--minig=/tmp/graph.xml"], True),
            (["-n", "asic0", "-Hm"], True),
            (["--plat", "--minig"], True),
            (["-nm"], False),
            (["-Hvm"], False),
            (["-p-m"], False),
            (["--var=-m"], False),
            (["--namespace=-m"], False),
            (["-Hm", "--help"], False),
        ]:
            words = ["sonic-cfggen", *arguments]
            with self.subTest(arguments=arguments):
                self.assertEqual(expected, bool(guard.command_dependencies(words)))
                self.assertEqual(expected, bool(guard.python_dependencies(
                    "subprocess.run({!r})".format(words)
                )))
                self.assertEqual(expected, bool(guard.yaml_dependencies(
                    "run: " + " ".join(words)
                )))
        for arguments in (["-xm"], ["--mi-unknown"], ["--p", "-m"]):
            with self.subTest(arguments=arguments):
                with self.assertRaises(guard.AnalysisError):
                    guard.command_dependencies(["sonic-cfggen", *arguments])

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
        ))

    def test_shell_and_yaml_entry_points(self):
        cases = [
            "sudo config load_minigraph -y",
            "/usr/local/bin/sonic-cfggen -H \\\n    --minigraph=/tmp/custom.xml",
            'HWSKU=`sonic-cfggen -m -v hwsku`',
            'echo "$(sonic-cfggen -m --print-data)"',
            'echo `sonic-cfggen -m --print-data`',
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
            "subprocess.run(['echo', ';', 'sonic-cfggen', '-m'])",
            "subprocess.run(['printf', '%s', '&&', 'config', "
            "'load_minigraph'])",
            "subprocess.run(['bash', '-c', 'echo done', 'name', ';', "
            "'sonic-cfggen', '-m'])",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))
        self.assertTrue(guard.python_dependencies(
            """subprocess.run(['bash', '-c', 'echo "$(sonic-cfggen -m)"'])"""
        ))

    def test_literal_command_concatenation_preserves_argument_boundaries(self):
        cases = [
            "subprocess.run(['sonic-cfggen'] + ['-m'])",
            "subprocess.run(('sonic-cfggen',) + ('-m',))",
            "prefix = ['sonic-cfggen']\nflags = ['-m']\n"
            "subprocess.run(prefix + flags)",
            "prefix = ['config']\ncmd = prefix + ['load_minigraph', '-y']\n"
            "subprocess.run(cmd)",
            "subprocess.run(['sonic-' + 'cfggen'] + ['-H'] + ['-m'])",
            "subprocess.run('sonic-cfggen' + ' -m', shell=True)",
        ]
        for source in cases:
            with self.subTest(source=source):
                self.assertTrue(guard.python_dependencies(source))
        for source in [
            "subprocess.run(['echo'] + [';', 'sonic-cfggen', '-m'])",
            "subprocess.run(['echo'] + ['$(sonic-cfggen -m)'])",
            "subprocess.run(['sonic-cfggen-'] + ['m'])",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.python_dependencies(source))

    def test_quoted_shell_operators_are_not_command_separators(self):
        for source in [
            "echo ';' sonic-cfggen -m",
            'printf "%s" "&&" config load_minigraph',
            r"echo \; sonic-cfggen -m",
            "logger '|' sonic-cfggen -m",
            "echo done # ; sonic-cfggen -m",
            """bash -c 'echo ";" sonic-cfggen -m'""",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.shell_dependencies(source))
        for source in [
            "echo done; sonic-cfggen -m",
            "echo ';' && sonic-cfggen -m",
            "true | sonic-cfggen -m",
            '(sonic-cfggen -m)',
            'echo ";" "$(sonic-cfggen -m)"',
            "bash -c 'sonic-cfggen -m' ignored",
            "bash -c 'echo done' ignored; sonic-cfggen -m",
            """bash -c 'cat "$1"' ignored /etc/sonic/minigraph.xml""",
        ]:
            with self.subTest(source=source):
                self.assertTrue(guard.shell_dependencies(source))

    def test_inline_yaml_preserves_shell_and_scalar_quoting(self):
        for source in [
            "run: echo '$(sonic-cfggen -m)'",
            "- ansible.builtin.shell: echo ';' sonic-cfggen -m",
            """run: "echo '$(sonic-cfggen -m)'" """,
            "run: 'echo ''$(sonic-cfggen -m)'''",
            r'run: "echo \"\\$(sonic-cfggen -m)\""',
            "run: echo done # $(sonic-cfggen -m)",
            "run: |\n  echo '$(sonic-cfggen -m)'\n",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.dependencies(
                    "example.yml", source.encode(), "100644"
                ))
        for source in [
            'run: echo "$(sonic-cfggen -m)"',
            "run: 'sonic-cfggen -m' # invoke the parser",
            r'run: "echo \"$(sonic-cfggen -m)\""',
            "command: config load_minigraph -y",
            "- ansible.builtin.shell: sonic-cfggen -m",
            "run: >\n  sonic-cfggen\n  -m\n",
            r'run: "sonic-cfggen\x20-m"',
        ]:
            with self.subTest(source=source):
                self.assertTrue(guard.dependencies(
                    "example.yml", source.encode(), "100644"
                ))
        for source in [
            "run: 'sonic-cfggen -m", 'run: "sonic-cfggen -m',
            r'run: "sonic-cfggen\q-m"',
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.dependencies(
                        "example.yml", source.encode(), "100644"
                    )

    def test_python_execution_modes_preserve_posix_semantics(self):
        cases = [
            ("['sonic-cfggen', '-m']", "", True),
            ("'sonic-cfggen -m'", "", False),
            ("'sonic-cfggen -m'", ", shell=False", False),
            ("'sonic-cfggen -m'", ", shell=True", True),
            ("['sonic-cfggen -m']", ", shell=True", True),
            ("['sonic-cfggen', '-m']", ", shell=True", False),
            ("['sonic-cfggen', '-m']", ", executable='/bin/echo'", False),
            ("['sonic-cfggen', '-m']", ", executable=None", True),
            ("'sonic-cfggen -m'", ", shell=True, executable='/bin/echo'", False),
        ]
        for args, options, expected in cases:
            for name in ("subprocess.run", "subprocess.Popen", "execute"):
                source = (
                    "import subprocess\n"
                    "from subprocess import check_call as execute\n"
                    "{}({}{})".format(name, args, options)
                )
                with self.subTest(source=source):
                    self.assertEqual(expected, bool(
                        guard.python_dependencies(source)
                    ))
        for name in ("os.system", "os.popen", "subprocess.getoutput",
                     "subprocess.getstatusoutput", "host.shell"):
            self.assertTrue(guard.python_dependencies(
                "{}('sonic-cfggen -m')".format(name)
            ))
        for name in ("subprocess.getoutput", "subprocess.getstatusoutput"):
            self.assertTrue(guard.python_dependencies(
                "{}(['sonic-cfggen -m'])".format(name)
            ))
            self.assertFalse(guard.python_dependencies(
                "{}(['sonic-cfggen', '-m'])".format(name)
            ))
        self.assertFalse(guard.python_dependencies(
            "host.command(\"echo '$(sonic-cfggen -m)'\")"
        ))
        self.assertTrue(guard.python_dependencies(
            "module.run_command('sonic-cfggen -m')"
        ))
        self.assertTrue(guard.python_dependencies(
            "USE_SHELL = True\n"
            "subprocess.run('sonic-cfggen -m', shell=USE_SHELL)"
        ))
        self.assertTrue(guard.python_dependencies(
            """module.run_command('echo "$(sonic-cfggen -m)"', """
            "use_unsafe_shell=True)"
        ))
        self.assertTrue(guard.python_dependencies(
            "module.run_command(['sonic-cfggen', '-m'], use_unsafe_shell=True)"
        ))
        self.assertFalse(guard.python_dependencies(
            "module.run_command(['echo', '$(sonic-cfggen -m)'], use_unsafe_shell=True)"
        ))
        with self.assertRaises(guard.AnalysisError):
            guard.python_dependencies(
                "subprocess.run('sonic-cfggen -m', shell=mode)"
            )
        with self.assertRaises(guard.AnalysisError):
            guard.python_dependencies(
                "subprocess.run('sonic-cfggen -m', shell=True, executable=program)"
            )
        with self.assertRaises(guard.AnalysisError):
            guard.python_dependencies(
                "subprocess.run('sonic-cfggen -m', **options)"
            )

    def test_python_argv_data_does_not_become_shell_source(self):
        for args in [
            ["printf", "%s\n", "sonic-cfggen -m"],
            ["echo", "example\nsonic-cfggen -m"],
            ["grep", "-e", "sonic-cfggen", "-m", "1", "script.sh"],
            ["grep", "-m", "1", "-e", "sonic-cfggen", "script.sh"],
            ["bash", "-c", "echo done", "name", "sonic-cfggen", "-m"],
        ]:
            with self.subTest(args=args):
                self.assertFalse(guard.python_dependencies(
                    "subprocess.run({!r})".format(args)
                ))
        self.assertFalse(guard.python_dependencies(
            "from tools import run\nrun('sonic-cfggen -m')"
        ))

    def test_non_string_argv_does_not_crash_command_analysis(self):
        for args in [
            ["bash", False],
            ["env", ["x"]],
            ["sudo", {"option": "value"}],
            ["nice", True],
        ]:
            with self.subTest(args=args):
                self.assertFalse(guard.python_dependencies(
                    "subprocess.run({!r})".format(args)
                ))

    def test_executable_positions_and_transparent_wrappers(self):
        for prefix in [
            "", "sudo ", "sudo -u root ", "sudo -nu root ",
            "env MODE=test ", "env -u UNUSED ",
            "command ", "exec ", "exec -a display ",
            "timeout 10 ", "timeout -k 2 10 ",
            "nice -n 10 ", "nohup ", "sudo env MODE=test ",
        ]:
            for command, expected in [
                ("sonic-cfggen -m", True),
                ("grep -e sonic-cfggen -m 1 script.sh", False),
                ("printf '%s\\n' 'sonic-cfggen -m'", False),
            ]:
                with self.subTest(prefix=prefix, command=command):
                    self.assertEqual(expected, bool(
                        guard.shell_dependencies(prefix + command)
                    ))
        self.assertFalse(guard.shell_dependencies(
            "command -v sonic-cfggen"
        ))
        for source in ("env --help", "nice --version", "sudo --version"):
            self.assertFalse(guard.shell_dependencies(source))

    def test_shell_options_identify_only_the_command_payload(self):
        for shell in ("bash", "sh", "dash"):
            for flags in ("-c", "-lc", "-ec", "-e -c", "-o errexit -c"):
                for command, expected in [
                    ("sonic-cfggen -m", True),
                    ("echo done", False),
                ]:
                    source = "{} {} {!r} name sonic-cfggen -m".format(
                        shell, flags, command
                    )
                    with self.subTest(source=source):
                        self.assertEqual(expected, bool(
                            guard.shell_dependencies(source)
                        ))
                        self.assertEqual(expected, bool(
                            guard.python_dependencies(
                                "subprocess.run({!r})".format(
                                    [shell] + flags.split() + [
                                        command, "name", "sonic-cfggen", "-m"
                                    ]
                                )
                            )
                        ))
        self.assertTrue(guard.shell_dependencies(
            "bash --noprofile --norc -c 'sonic-cfggen -m'"
        ))
        self.assertFalse(guard.shell_dependencies(
            "bash -nc 'sonic-cfggen -m'"
        ))

    def test_named_noexec_options_and_short_overrides_are_ordered(self):
        for flags, expected in [
            (["-o", "noexec", "-c"], False),
            (["-n", "+o", "noexec", "-c"], True),
            (["-o", "noexec", "+n", "-c"], True),
            (["-o", "noexec", "+o", "noexec", "-n", "-c"], False),
            (["-oc", "noexec"], False),
            (["-co", "noexec"], False),
            (["-nc", "+o", "noexec"], True),
            (["-o", "noexec", "+on", "noexec", "-c"], True),
            (["-o", "noexec", "+co", "noexec"], True),
            (["-o", "errexit", "-c"], True),
            (["-O", "extglob", "-c"], True),
        ]:
            with self.subTest(flags=flags):
                self.assertEqual(expected, bool(guard.shell_dependencies(
                    "bash {} 'sonic-cfggen -m'".format(" ".join(flags))
                )))
                self.assertEqual(expected, bool(guard.python_dependencies(
                    "subprocess.run({!r})".format(
                        ["bash", *flags, "sonic-cfggen -m"]
                    )
                )))
        self.assertTrue(guard.shell_dependencies(
            'bash -o noexec -c "$(sonic-cfggen -m)"'
        ))
        with self.assertRaises(guard.AnalysisError):
            guard.python_dependencies(
                "subprocess.run(['bash', '-n', '+o', mode, '-c', 'sonic-cfggen -m'])"
            )

    def test_multiline_shell_data_and_executable_substitutions(self):
        for source in [
            "echo 'example:\nsonic-cfggen -m\n'",
            'printf "%s" "example:\nsonic-cfggen -m\n"',
            "cat <<'EOF'\nsonic-cfggen -m\n$(sonic-cfggen -m)\nEOF\n",
            "cat <<EOF\nsonic-cfggen -m\nEOF\n",
            "CMD=sonic-cfggen\n'$CMD' -m",
            "CMD='sonic-cfggen -m'\n\"$CMD\"",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.shell_dependencies(source))
        for source in [
            'echo "example:\n$(sonic-cfggen -m)\n"',
            "cat <<EOF\n$(sonic-cfggen -m)\nEOF\n",
            "CMD='sonic-cfggen -m'\n$CMD",
            "CMD=sonic-cfggen\n\"$CMD\" -m",
        ]:
            with self.subTest(source=source):
                self.assertTrue(guard.shell_dependencies(source))

    def test_literal_shell_stdin_follows_the_consumer_and_options(self):
        for command in [
            "bash", "sh", "dash", "bash -s -- argument", "bash -",
            "bash -n +o noexec", "sudo -u root bash", "env MODE=test bash",
            "command bash", "timeout 10 bash",
        ]:
            for delimiter in ("EOF", "'EOF'", '"EOF"', r"\EOF"):
                source = "{} <<{}\nsonic-cfggen -m\nEOF\n".format(command, delimiter)
                with self.subTest(source=source):
                    self.assertTrue(guard.shell_dependencies(source))
        for source, expected in [
            ("bash <<-'EOF'\n\tsonic-cfggen -m\n\tEOF\n", True),
            ("bash <<'EOF'\necho 'sonic-cfggen -m'\nEOF\n", False),
            ("bash <<'EOF'\necho \"$(sonic-cfggen -m)\"\nEOF\n", True),
            ("bash <<EOF\nsonic-\\\ncfggen -m\nEOF\n", True),
            ("cat <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash -n <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash -o noexec <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash -n <<EOF\n$(sonic-cfggen -m)\nEOF\n", True),
            ("bash -c 'echo done' <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash -c cat <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash script.sh <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash /dev/null <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash --help <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash <<'EOF' </dev/null\nsonic-cfggen -m\nEOF\n", False),
            ("bash </dev/null <<'EOF'\nsonic-cfggen -m\nEOF\n", True),
            ("bash <<'EOF' >/dev/null\nsonic-cfggen -m\nEOF\n", True),
            ("bash <<'EOF' >\"$OUTPUT\"\nsonic-cfggen -m\nEOF\n", True),
            ("bash 3<<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash <<'EOF' | cat\nsonic-cfggen -m\nEOF\n", True),
            ("cat <<'EOF' | grep sonic\nsonic-cfggen -m\nEOF\n", False),
            ("bash <<< 'sonic-cfggen -m'", True),
            ("bash 0<<< 'sonic-cfggen -m'", True),
            ("bash '0'<<< 'sonic-cfggen -m'", False),
            ("bash 0 <<< 'sonic-cfggen -m'", False),
            ("bash <<< 'sonic-cfggen -m' </dev/null", False),
            ("bash -n <<< 'sonic-cfggen -m'", False),
            ("cat <<< 'sonic-cfggen -m'", False),
            ("bash <<'EOF'\nEOF\n", False),
            ("bash <<< ''", False),
            ("<<< 'sonic-cfggen -m'", False),
            ('<<< "$(sonic-cfggen -m)"', True),
            ("bash <<'EOF' <&0\nsonic-cfggen -m\nEOF\n", True),
            ("bash 3<<'EOF' <&3\nsonic-cfggen -m\nEOF\n", True),
            ("bash 3<<'EOF' <&3-\nsonic-cfggen -m\nEOF\n", True),
            ("bash <<'EOF' <&-\nsonic-cfggen -m\nEOF\n", False),
            ("bash <<'EOF' <&0-\nsonic-cfggen -m\nEOF\n", False),
            ("bash <<< 'sonic-cfggen -m' 0<&0", True),
            ("bash <<< 'sonic-cfggen -m' '0'<&0", False),
            ("bash <<< 'sonic-cfggen -m' 0 <&0", False),
            ("bash -n <<'EOF' <\"$INPUT\"\nsonic-cfggen -m\nEOF\n", False),
            ("bash -c 'echo done' <<'EOF' <\"$INPUT\"\nsonic-cfggen -m\nEOF\n", False),
            ("bash /dev/null <<'EOF' <\"$INPUT\"\nsonic-cfggen -m\nEOF\n", False),
            ("bash -c cat arg /dev/stdin <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
            ("bash script.sh /dev/stdin <<'EOF'\nsonic-cfggen -m\nEOF\n", False),
        ]:
            with self.subTest(source=source):
                self.assertEqual(expected, bool(guard.shell_dependencies(source)))

    def test_unresolved_executable_stdin_is_not_clean(self):
        for source in [
            "cat <<'EOF' | bash\nsonic-cfggen -m\nEOF\n",
            "{ bash; } <<'EOF'\nsonic-cfggen -m\nEOF\n",
            "bash -c bash <<'EOF'\nsonic-cfggen -m\nEOF\n",
            "bash --unknown <<'EOF'\nsonic-cfggen -m\nEOF\n",
            "sudo --unknown bash <<'EOF'\nsonic-cfggen -m\nEOF\n",
            "CMD='sonic-cfggen -m'\nbash <<EOF\n$CMD\nEOF\n",
            "bash /dev/fd/3 3<<'EOF'\nsonic-cfggen -m\nEOF\n",
            "bash <<EOF\n\\$(sonic-cfggen -m)\nEOF\n",
            "bash 0<<'EOF'\nsonic-cfggen -m\nEOF\n",
            "bash <<'EOF' <\"$INPUT\"\nsonic-cfggen -m\nEOF\n",
            "bash <<< \"$SCRIPT\"",
            "bash 3<<< 'sonic-cfggen -m'",
            "bash \"$SCRIPT\" <<'EOF'\nsonic-cfggen -m\nEOF\n",
            "bash -c \"$COMMAND\" <<'EOF'\nsonic-cfggen -m\nEOF\n",
            "bash <<'EOF' <>/dev/stdin\nsonic-cfggen -m\nEOF\n",
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.shell_dependencies(source)

    def test_mixed_quoted_and_expanded_shell_fields(self):
        for source, expected in [
            ("PREFIX='sonic-cfggen '\nSUFFIX=-m\n\"$PREFIX\"$SUFFIX", False),
            ("FLAGS=-m\n\"sonic-cfggen \"$FLAGS", False),
            ("PREFIX='sonic-cfggen '\nSUFFIX=' -m'\n\"$PREFIX\"$SUFFIX", False),
            ("PREFIX='sonic-cfggen '\nSUFFIX=-m\n$PREFIX\"$SUFFIX\"", True),
            ("PREFIX=sonic-\nSUFFIX=cfggen\n\"$PREFIX\"$SUFFIX -m", True),
            ("EMPTY=''\n$EMPTY sonic-cfggen -m", True),
            ("EMPTY=''\n\"$EMPTY\" sonic-cfggen -m", False),
            ("PREFIX='env '\nCMD=sonic-cfggen\n$PREFIX\"$CMD\" -m", True),
            ("PREFIX='env '\nCMD=sonic-cfggen\n\"$PREFIX\"$CMD -m", False),
        ]:
            with self.subTest(source=source):
                self.assertEqual(expected, bool(guard.shell_dependencies(source)))

    def test_xml_operands_are_not_lost_when_command_positions_are_restricted(self):
        for source in [
            "$READER /etc/sonic/minigraph.xml",
            "bash /etc/sonic/minigraph.xml",
            'bash -c \'cat "$1"\' name /etc/sonic/minigraph.xml',
            "cat < /etc/sonic/minigraph.xml",
            "cat /tmp/$DIRECTORY/minigraph.xml",
        ]:
            with self.subTest(source=source):
                self.assertTrue(guard.shell_dependencies(source))
        for source in [
            "bash -c 'echo minigraph.xml'",
            "bash -nc 'echo minigraph.xml'",
            "sudo echo minigraph.xml",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.shell_dependencies(source))

    def test_shell_file_checks_and_function_bindings_preserve_dependencies(self):
        for source in [
            "GRAPH=/etc/sonic/minigraph.xml\n"
            "initialize() { if [ -r \"$GRAPH\" ]; then echo ready; fi; }",
            "[[ -e /etc/sonic/minigraph.xml ]]",
            "[[ /etc/sonic/minigraph.xml -nt other ]]",
            "migrate() { files='minigraph.xml other.json'; copy_files $files; }",
            "migrate() { local GRAPH=minigraph.xml; cat \"$GRAPH\"; }",
            "load() { local CMD='sonic-cfggen -m'; $CMD; }",
        ]:
            with self.subTest(source=source):
                self.assertTrue(guard.shell_dependencies(source))
        for source in [
            "unused() { local CMD='sonic-cfggen -m'; }\n$CMD",
            "unused() { local GRAPH=minigraph.xml; }\ncat \"$GRAPH\"",
            "unused() { files='minigraph.xml other.json'; }",
            "[[ -n minigraph.xml ]]",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.shell_dependencies(source))
        before = "GRAPH=minigraph.xml\n[ -r \"$GRAPH\" ]\n"
        self.assertTrue(
            guard.shell_dependencies(before + '[ -e "$GRAPH" ]\n')
            - guard.shell_dependencies(before)
        )

    def test_continuations_preserve_tokens_and_literal_context(self):
        for source in [
            "sonic-\\\ncfggen -m",
            "sonic-cfggen -\\\nm",
            "sonic-cfggen \\\n  -m",
            "CMD=sonic-\\\ncfggen\n$CMD -m",
        ]:
            with self.subTest(source=source):
                self.assertTrue(guard.shell_dependencies(source))
        for source in [
            "'sonic-\\\ncfggen' -m",
            "echo 'example:\\\nsonic-cfggen -m'",
            "# comment \\\n# sonic-cfggen -m",
            "cat <<'EOF'\nsonic-\\\ncfggen -m\nEOF\n",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.shell_dependencies(source))

    def test_yaml_command_structure_and_scalar_semantics(self):
        for key in ("run", "shell", "ansible.builtin.shell",
                    "command", "ansible.builtin.command"):
            for header in (">", ">-", ">+", ">2", ">2-", "> # comment"):
                source = "{}: {}\n  sonic-cfggen\n  -m\n".format(key, header)
                with self.subTest(source=source):
                    self.assertTrue(guard.dependencies(
                        "example.yml", source.encode(), "100644"
                    ))
        cases = [
            ("run: >\n  echo done\n\n  sonic-cfggen -m\n", True),
            ("run: |\n  sonic-cfggen\n  -m\n", False),
            ("name: sonic-cfggen -m inspection\nrun: echo done\n", False),
            ("steps:\n  - name: sonic-cfggen -m inspection\n"
             "    run: echo done\n", False),
            ("env:\n  run: sonic-cfggen -m\nsteps:\n  - run: echo done\n", False),
            ("vars:\n  command: sonic-cfggen -m\n", False),
            ("jobs:\n  test:\n    steps:\n      - run: sonic-cfggen -m\n", True),
            ("- hosts: all\n  tasks:\n    - ansible.builtin.shell: "
             "sonic-cfggen -m\n", True),
            ("command: echo '$(sonic-cfggen -m)'\n", False),
            ("shell: echo '$(sonic-cfggen -m)'\n", False),
            ('command: echo "$(sonic-cfggen -m)"\n', False),
            ('shell: echo "$(sonic-cfggen -m)"\n', True),
            ("- ansible.builtin.command:\n    argv:\n"
             "      - sonic-cfggen\n      - -m\n", True),
            ("- ansible.builtin.shell:\n  args:\n"
             "    cmd: sonic-cfggen -m\n", True),
            ("steps:\n  - task: Bash@3\n    inputs:\n"
             "      targetType: inline\n      script: sonic-cfggen -m\n", True),
            ("steps:\n  - task: Download@1\n    inputs:\n"
             "      script: sonic-cfggen -m\n", False),
        ]
        for source, expected in cases:
            with self.subTest(source=source):
                self.assertEqual(expected, bool(guard.dependencies(
                    "example.yml", source.encode(), "100644"
                )))
        for source in [
            "&cycle {steps: [*cycle]}",
            "run: &cycle {cmd: *cycle}",
            "run: {cmd: {cmd: 'echo done'}}",
            "run: {cmd: ['sonic-cfggen', '-m']}",
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.dependencies("example.yml", source.encode(), "100644")

    def test_yaml_run_interpreter_and_default_precedence(self):
        for source, expected in [
            ("shell: python\nrun: import minigraph\n", True),
            ("shell: python\nrun: print('sonic-cfggen -m')\n", False),
            ("shell: python {0}\nrun: import minigraph\n", True),
            ("shell: bash {0}\nrun: sonic-cfggen -m\n", True),
            ("defaults:\n  run:\n    shell: python\njobs:\n"
             "  test:\n    steps:\n      - run: import minigraph\n", True),
            ("jobs:\n  test:\n    defaults:\n      run:\n        shell: python\n"
             "    steps:\n      - run: import minigraph\n", True),
            ("defaults:\n  run:\n    shell: python\njobs:\n"
             "  test:\n    defaults:\n      run:\n        shell: bash\n"
             "    steps:\n      - run: sonic-cfggen -m\n", True),
            ("defaults:\n  run:\n    shell: bash\njobs:\n"
             "  test:\n    steps:\n      - shell: python\n"
             "        run: import minigraph\n", True),
            ("defaults:\n  run:\n    shell: python\njobs:\n"
             "  test:\n    steps:\n      - shell: bash\n"
             "        run: echo 'import minigraph'\n", False),
            ("jobs:\n  first:\n    defaults:\n      run:\n        shell: python\n"
             "    steps:\n      - run: pass\n"
             "  second:\n    steps:\n      - run: sonic-cfggen -m\n", True),
            ("steps:\n  - shell: python\n    run: pass\n"
             "  - run: sonic-cfggen -m\n", True),
            ("- ansible.builtin.shell: sonic-cfggen -m\n", True),
            ("jobs:\n  test:\n    steps:\n      - shell: pwsh\n"
             "        run: Write-Output done\n", False),
        ]:
            with self.subTest(source=source):
                self.assertEqual(expected, bool(guard.yaml_dependencies(source)))
        for source in [
            "shell: pwsh\nrun: sonic-cfggen -m\n",
            "shell: sonic-cfggen -m {0}\nrun: echo done\n",
            "shell: ${{ matrix.shell }}\nrun: import minigraph\n",
            "shell: {cmd: python}\nrun: import minigraph\n",
            "shell: python -c {0}\nrun: import minigraph\n",
            "shell: python\nrun: 'import minigraph; if'\n",
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.yaml_dependencies(source)

    def test_yaml_azure_wrappers_preserve_command_context(self):
        for directive in ("if parameters.enabled", "elseif parameters.enabled",
                          "else", "each item in parameters.items"):
            for body in [
                "- script: sonic-cfggen -m\n",
                "- task: Bash@3\n  inputs:\n    targetType: inline\n    script: sonic-cfggen -m\n",
                "- ${{ if item.enabled }}:\n    - bash: sonic-cfggen -m\n",
            ]:
                source = "steps:\n  - ${{{{ {} }}}}:\n{}".format(
                    directive, textwrap.indent(body, "      ")
                )
                with self.subTest(source=source):
                    self.assertEqual(
                        guard.yaml_dependencies("steps:\n  - script: sonic-cfggen -m"),
                        guard.yaml_dependencies(source),
                    )
        for source in [
            "jobs:\n  - ${{ if parameters.enabled }}:\n"
            "      - job: build\n        steps:\n          - bash: sonic-cfggen -m\n",
            "jobs:\n  ${{ if parameters.enabled }}:\n"
            "    build:\n      steps:\n        - bash: sonic-cfggen -m\n",
            "defaults:\n  run:\n    shell: python\nsteps:\n"
            "  - ${{ if parameters.enabled }}:\n      - run: import minigraph\n",
            "steps:\n  - task: Bash@3\n    inputs:\n"
            "      targetType: inline\n"
            "      ${{ if parameters.enabled }}:\n        script: sonic-cfggen -m\n",
            "steps:\n  - task: Bash@3\n    ${{ if parameters.enabled }}:\n"
            "      inputs:\n        targetType: inline\n        script: sonic-cfggen -m\n",
        ]:
            with self.subTest(source=source):
                self.assertTrue(guard.yaml_dependencies(source))
        for source in [
            "steps:\n  - ${{ if parameters.enabled }}:\n"
            "      - script: echo 'sonic-cfggen -m'\n",
            "variables:\n  ${{ if parameters.enabled }}:\n    script: sonic-cfggen -m\n",
            "steps:\n  - env:\n      ${{ if parameters.enabled }}:\n"
            "        script: sonic-cfggen -m\n",
            "displayName:\n  ${{ if parameters.enabled }}:\n    script: sonic-cfggen -m\n",
            "steps:\n  - task: Download@1\n    ${{ if parameters.enabled }}:\n"
            "      inputs:\n        script: sonic-cfggen -m\n",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.yaml_dependencies(source))
        with self.assertRaises(guard.AnalysisError):
            guard.yaml_dependencies("&cycle { '${{ if parameters.enabled }}': *cycle }")

    def test_yaml_task_inputs_follow_the_selected_execution_mode(self):
        for inputs, expected in [
            ("targetType: inline\nscript: sonic-cfggen -m\n", True),
            ("targetType: INLINE\nscript: sonic-cfggen -m\n", True),
            ("filePath: notice.sh\nscript: sonic-cfggen -m\n", False),
            ("targetType: filePath\nfilePath: notice.sh\nscript: sonic-cfggen -m\n", False),
            ("targetType: filePath\nfilePath: /tmp/minigraph.xml\n", True),
            ("targetType: inline\nscript: echo done\nfilePath: /tmp/minigraph.xml\n", False),
            ("filePath: notice.sh\narguments: \"$(sonic-cfggen -m)\"\n", True),
            ("filePath: notice.sh\narguments: \"'sonic-cfggen -m'\"\n", False),
            ("targetType: inline\n${{ if parameters.enabled }}:\n  script: sonic-cfggen -m\n", True),
            ("filePath: notice.sh\n${{ if parameters.enabled }}:\n"
             "  arguments: \"$(sonic-cfggen -m)\"\n", True),
            ("targetType: inline\nscript: echo done\nrun: sonic-cfggen -m\n", False),
        ]:
            source = "steps:\n- task: Bash@3\n  inputs:\n" + textwrap.indent(inputs, "    ")
            with self.subTest(source=source):
                self.assertEqual(expected, bool(guard.yaml_dependencies(source)))
        self.assertTrue(guard.yaml_dependencies(
            "steps:\n- task: CmdLine@2\n  inputs:\n    script: sonic-cfggen -m\n"
        ))
        for source in [
            "steps:\n- task: Bash@3\n  inputs:\n"
            "    targetType: ${{ parameters.mode }}\n    script: sonic-cfggen -m\n",
            "steps:\n- task: Bash@3\n  inputs:\n"
            "    script: sonic-cfggen -m\n    ${{ if parameters.enabled }}:\n"
            "      targetType: inline\n",
            "steps:\n- task: Bash@99\n  inputs:\n    script: sonic-cfggen -m\n",
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.yaml_dependencies(source)

    def test_yaml_unresolved_merge_keys_fail_only_in_command_structures(self):
        for source in [
            "definition: &body {run: sonic-cfggen -m}\nsteps:\n- <<: *body\n",
            "jobs:\n  <<: {example: {steps: [{run: 'sonic-cfggen -m'}]}}\n",
            "defaults:\n  run:\n    <<: {shell: python}\nrun: import minigraph\n",
            "command:\n  <<: {argv: [sonic-cfggen, -m]}\n",
            "steps:\n- !!merge '<<': {run: sonic-cfggen -m}\n",
        ]:
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.yaml_dependencies(source)
        for source in [
            "env:\n  <<: {run: sonic-cfggen -m}\nrun: echo done\n",
            "'<<': {run: sonic-cfggen -m}\nrun: echo done\n",
            "!!str <<: {run: sonic-cfggen -m}\nrun: echo done\n",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.yaml_dependencies(source))

    def test_template_commands_and_comments_remain_distinct(self):
        for path, source, expected in [
            ("example.sh.j2", "{# sonic-cfggen -m #}\necho done", False),
            ("example.sh.j2", "{% if enabled %}\nsonic-cfggen -m "
             "{{ graph }}\n{% endif %}\n", True),
            ("example.yml.j2", "steps:\n{% if enabled %}\n"
             "  - run: sonic-cfggen -m {{ graph }}\n{% endif %}\n", True),
        ]:
            with self.subTest(path=path, source=source):
                self.assertEqual(expected, bool(guard.dependencies(
                    path, source.encode(), "100644"
                )))
        self.assertFalse(guard.dependencies(
            "Makefile", b"all: $(TARGET)\n\t@echo sonic-buildimage\n", "100644"
        ))

    def test_jinja_lexical_regions_and_whitespace_control(self):
        for source, expected in [
            ("{# Example: {% if enabled %}sonic-cfggen -m{% endif %} #}", False),
            ("{# Example:\n{% if enabled %}\nsonic-cfggen -m\n{% endif %}\n#}", False),
            ('{% set note = "#}sonic-cfggen -m" %}\necho done\n', False),
            ('{% set note = "%}sonic-cfggen -m" %}\necho done\n', False),
            ("# {{ '{#' }}\nsonic-cfggen -m\n# {{ '#}' }}\n", True),
            ("{% raw %}# {#\nsonic-cfggen -m\n# #}\n{% endraw %}", True),
            ("{% raw %}echo '{# sonic-cfggen -m #}'{% endraw %}", False),
            ("sonic- {#- ignored -#} cfggen -m", True),
            ("sonic- {% if enabled -%} cfggen -m{% endif %}", False),
            ("{% if enabled -%}\nsonic-cfggen -m\n{% endif %}", True),
            ("echo '{{ \"}} sonic-cfggen -m\" }}'\n", False),
        ]:
            with self.subTest(source=source):
                self.assertEqual(expected, bool(guard.dependencies(
                    "example.sh.j2", source.encode(), "100644"
                )))
        self.assertTrue(guard.dependencies(
            "example.yml.j2",
            b"run: >\n  sonic- {#- ignored -#} cfggen -m\n",
            "100644",
        ))
        for source in ("{# unfinished", "{{ value", "{% if enabled", "{% raw %}text"):
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.dependencies("example.sh.j2", source.encode(), "100644")

    def test_jinja_branch_alternatives_preserve_inline_fragments(self):
        for source in [
            "{% if flag %}echo {% else %}sonic-cfggen -m{% endif %}",
            "{% if a %}echo {% elif b %}sonic-cfggen -m{% else %}echo done{% endif %}",
            "{% if flag %}sonic-cfggen{% else %}printf{% endif %} -m",
            "sonic-cfg{% if flag %}ignored{% endif %}gen -m",
            "{% if a %}{% if b %}sonic-cfggen -m{% endif %}{% else %}echo done{% endif %}",
            "{% for item in items %}echo done{% else %}sonic-cfggen -m{% endfor %}",
            "'sonic-{% if flag %}cfggen{% else %}example{% endif %}' -m",
        ]:
            for path in ("example.sh.j2", "example.j2"):
                with self.subTest(path=path, source=source):
                    self.assertEqual(
                        guard.shell_dependencies("sonic-cfggen -m"),
                        guard.dependencies(path, source.encode(), "100644"),
                    )
        self.assertTrue(guard.dependencies(
            "example.yml.j2",
            b"run: {% if flag %}echo done{% else %}sonic-cfggen -m{% endif %}\n",
            "100644",
        ))
        for source in [
            "{% if flag %}echo 'sonic-cfggen -m'{% else %}printf done{% endif %}",
            "echo '{% if flag %}$(sonic-cfggen -m){% else %}done{% endif %}'",
        ]:
            with self.subTest(source=source):
                self.assertFalse(guard.dependencies("example.sh.j2", source.encode(), "100644"))

    def test_jinja_ambiguous_counts_and_expansion_limits_fail_closed(self):
        for source in [
            "{% if flag %}sonic-cfggen -m{% else %}sonic-cfggen -m{% endif %}",
            "sonic-cfggen -m\n{% if flag %}echo done{% endif %}",
            "{% if a %}sonic-cfggen -m\n{% endif %}"
            "{% if b %}sonic-cfggen -m\n{% endif %}",
            "{% for item in items %}sonic-cfggen -m{% else %}sonic-cfggen -m{% endfor %}",
        ]:
            with self.subTest(source=source):
                with self.assertRaisesRegex(guard.AnalysisError, "ambiguous"):
                    guard.dependencies("example.sh.j2", source.encode(), "100644")
        many_branches = "{% if flag %}echo yes\n{% else %}echo no\n{% endif %}" * 7
        with self.assertRaisesRegex(guard.AnalysisError, "limit"):
            guard.dependencies("example.sh.j2", many_branches.encode(), "100644")
        self.assertFalse(guard.dependencies(
            "example.json.j2", many_branches.encode(), "100644"
        ))
        for source in ("{% if flag %}echo done", "{% else %}echo done{% endif %}"):
            with self.subTest(source=source):
                with self.assertRaises(guard.AnalysisError):
                    guard.dependencies("example.sh.j2", source.encode(), "100644")

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


class GitFixtureTest(unittest.TestCase):
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
            mode = "100644"
            if isinstance(content, tuple):
                mode, content = content
            object_id = self.run_git("hash-object", "-w", "--stdin", data=content.encode())
            self.run_git(
                "update-index", "--add", "--cacheinfo", mode, object_id, path
            )
        return self.run_git("write-tree")

    def compare(self, before, after):
        return guard.check_changes(self.repo, self.tree(before), self.tree(after))


class GitComparisonTest(GitFixtureTest):
    def test_context_regressions_through_the_full_checker(self):
        cases = [
            ("script.py", "subprocess.run(['sonic-cfggen -m'], shell=True)", 1),
            ("script.py", "subprocess.run(['sonic-cfggen','-m'], shell=True)", 0),
            ("script.py", "subprocess.run(['printf','%s\\n','sonic-cfggen -m'])", 0),
            ("script.sh", "grep -e sonic-cfggen -m 1 README.md", 0),
            ("script.sh", "bash -lc 'sonic-cfggen -m'", 1),
            ("script.sh", "sonic-\\\ncfggen -m", 1),
            ("script.sh", "echo 'example:\nsonic-cfggen -m\n'", 0),
            ("script.sh", "cat <<'EOF'\nsonic-cfggen -m\nEOF\n", 0),
            ("script.sh", "PREFIX='sonic-cfggen '\nSUFFIX=-m\n"
             "\"$PREFIX\"$SUFFIX", 0),
            ("script.py", "subprocess.run(['bash', False])", 0),
            ("script.yml", "run: {cmd: {cmd: 'echo done'}}", 2),
            ("workflow.yml", "run: >\n  echo done\n\n  sonic-cfggen -m\n", 1),
            ("workflow.yml", "ansible.builtin.shell: > # comment\n"
             "  sonic-cfggen\n  -m\n", 1),
            ("workflow.yml", "name: sonic-cfggen -m inspection\nrun: echo done", 0),
            ("workflow.yml", 'run: "sonic-cfggen\\q-m"', 2),
            ("script.sh", "sonic-cfggen -Hm", 1),
            ("script.sh", "sonic-cfggen --minig=/tmp/graph.xml", 1),
            ("script.sh", "sonic-cfggen --var=-m", 0),
            ("script.sh", "sonic-cfggen --p -m", 2),
            ("script.py", "check_output_pipe(['echo', 'data'], "
             "['sonic-cfggen', '-m'])", 1),
            ("script.py", "getstatusoutput_noshell('sonic-cfggen -m')", 0),
            ("script.py", "from builtins import open as read_graph\n"
             "read_graph('/etc/sonic/minigraph.xml')", 1),
            ("script.py", "from pathlib import Path as P\n"
             "P('/etc/sonic/minigraph.xml').read_text()", 1),
            ("script.py", "from tools import load_minigraph as load\nload(host)", 1),
            ("workflow.yml", "defaults:\n  run:\n    shell: python\n"
             "jobs:\n  test:\n    steps:\n      - run: import minigraph\n", 1),
            ("workflow.yml", "shell: python\nrun: print('sonic-cfggen -m')", 0),
            ("workflow.yml", "shell: pwsh\nrun: sonic-cfggen -m", 2),
            ("workflow.yml", "shell: sonic-cfggen -m {0}\nrun: echo done", 2),
            ("script.sh", "bash -o noexec -c 'sonic-cfggen -m'", 0),
            ("script.sh", "bash -n +o noexec -c 'sonic-cfggen -m'", 1),
            ("script.sh", "bash <<'EOF'\nsonic-cfggen -m\nEOF\n", 1),
            ("script.py", "subprocess.run(\"bash <<'EOF'\\nsonic-cfggen -m\\nEOF\\n\", "
             "shell=True)", 1),
            ("workflow.yml", "run: |\n  bash <<'EOF'\n  sonic-cfggen -m\n  EOF\n", 1),
            ("script.sh", "cat <<'EOF'\nsonic-cfggen -m\nEOF\n", 0),
            ("script.sh", "bash -n <<'EOF'\nsonic-cfggen -m\nEOF\n", 0),
            ("script.sh", "cat <<'EOF' | bash\nsonic-cfggen -m\nEOF\n", 2),
            ("script.sh", "bash <<EOF\n\\$(sonic-cfggen -m)\nEOF\n", 2),
            ("script.sh", "bash <<'EOF' </dev/null\nsonic-cfggen -m\nEOF\n", 0),
            ("script.sh", "bash 0<<< 'sonic-cfggen -m'", 1),
            ("script.sh", "<<< 'sonic-cfggen -m'", 0),
            ("script.sh", "bash 3<<'EOF' <&3-\nsonic-cfggen -m\nEOF\n", 1),
            ("script.sh", "bash <<< \"$SCRIPT\"", 2),
            ("script.sh", "bash <<'EOF' <\"$INPUT\"\nsonic-cfggen -m\nEOF\n", 2),
            ("script.sh.j2", "{# {% if enabled %}sonic-cfggen -m{% endif %} #}", 0),
            ("script.sh.j2", "sonic- {#- ignored -#} cfggen -m", 1),
            ("script.sh.j2", "{{ value", 2),
            ("script.py", "subprocess.run([b'sonic-cfggen', b'-m'])", 1),
            ("script.py", "open(b'/etc/sonic/minigraph.xml')", 1),
            ("script.py", "Path(b'/etc/sonic', 'minigraph.xml')", 0),
            ("script.py", r"open(b'/tmp/\xff/minigraph.xml')", 2),
            ("workflow.yml", "steps:\n  - ${{ if parameters.enabled }}:\n"
             "      - bash: sonic-cfggen -m\n", 1),
            ("script.sh.j2", "{% if flag %}echo {% else %}sonic-cfggen -m{% endif %}", 1),
            ("script.sh.j2", "{% if flag %}sonic-cfggen -m"
             "{% else %}sonic-cfggen -m{% endif %}", 2),
            ("script.py", "Path('/tmp/notice.txt').write_text('minigraph.xml')", 0),
            ("script.py", "Path('/tmp/minigraph.xml').write_text('notice')", 1),
            ("script.py", "Path('/tmp/minigraph.xml', '/tmp/notice.txt').read_text()", 0),
            ("script.py", "subprocess.run(['alias', '-m'], "
             "executable='sonic-cfggen', shell=mode)", 2),
            ("script.sh", "bash $OPTIONS -c 'sonic-cfggen -m'", 2),
            ("script.sh", "bash -- $SCRIPT <<'EOF'\nsonic-cfggen -m\nEOF\n", 2),
            ("workflow.yml", "definition: &body {run: sonic-cfggen -m}\n"
             "steps:\n- <<: *body\n", 2),
            ("workflow.yml", "steps:\n- task: Bash@3\n  inputs:\n"
             "    filePath: notice.sh\n    script: sonic-cfggen -m\n", 0),
        ]
        for path, source, expected in cases:
            with self.subTest(path=path, source=source):
                base = self.tree({})
                head = self.tree({path: source})
                with mock.patch.object(guard.Path, "cwd", return_value=self.repo), \
                        redirect_stdout(io.StringIO()):
                    self.assertEqual(expected, guard.main([
                        "--base", base, "--head", head
                    ]))

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

    def test_jinja_reference_counts_and_removals_are_compared(self):
        path = "files/example.sh.j2"
        source = "{% if flag %}sonic-cfggen -m\n{% endif %}"
        self.assertFalse(self.compare({path: source}, {path: source + "\n# unchanged"}))
        self.assertFalse(self.compare({path: source}, {path: "echo done\n"}))
        self.assertTrue(self.compare({}, {path: source}))
        added = source.replace("sonic-cfggen -m\n", "sonic-cfggen -m\n" * 2)
        self.assertTrue(self.compare({path: source}, {path: added}))
        ambiguous = source.replace("{% endif %}", "{% else %}sonic-cfggen -m{% endif %}")
        with self.assertRaises(guard.AnalysisError):
            self.compare({path: source}, {path: ambiguous})

    def test_python_file_payload_changes_do_not_hide_or_invent_paths(self):
        path = "scripts/example.py"
        source = "Path('/tmp/notice.txt').write_text('notice')"
        payload = source.replace("write_text('notice')", "write_text('minigraph.xml')")
        self.assertFalse(self.compare({path: source}, {path: payload}))
        real = payload.replace("/tmp/notice.txt", "/tmp/minigraph.xml")
        self.assertTrue(self.compare({path: payload}, {path: real}))
        self.assertFalse(self.compare({path: real}, {path: payload}))
        self.assertFalse(self.compare({path: real}, {path: real.replace("'minigraph.xml'", "'notice'")}))
        self.assertTrue(self.compare({path: real}, {path: real + "\n" + real}))

    def test_non_utf8_git_path_is_a_controlled_analysis_failure(self):
        base = self.tree({})
        object_id = self.run_git("hash-object", "-w", "--stdin", data=b"echo done\n")
        self.run_git(
            "update-index", "--index-info",
            data=b"100644 " + object_id.encode() + b"\tinvalid-\xff.sh\n",
        )
        head = self.run_git("write-tree")
        with mock.patch.object(guard.Path, "cwd", return_value=self.repo), \
                redirect_stdout(io.StringIO()):
            self.assertEqual(2, guard.main(["--base", base, "--head", head]))

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

    def test_shipped_xml_symlinks_are_compared(self):
        path = "device/vendor/minigraph.xml"
        link = ("120000", "../shared/base.xml")
        self.assertTrue(self.compare({}, {path: link}))
        self.assertFalse(self.compare(
            {path: link}, {path: link, "notes.md": "unchanged dependency"}
        ))
        self.assertFalse(self.compare({path: link}, {}))
        fixture = {"tests/fixtures/minigraph.xml": link}
        self.assertFalse(self.compare({}, fixture))
        self.assertTrue(self.compare(fixture, {path: link}))
        self.assertTrue(self.compare({}, {
            "scripts/parser.py": ("120000", "../minigraph.py"),
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


class WorkflowComparisonTest(GitFixtureTest):
    def setUp(self):
        super().setUp()
        self.bash = shutil.which("bash")
        if os.name == "nt":
            git_path = Path(shutil.which("git"))
            self.bash = str(git_path.parents[1] / "bin" / "bash.exe")
        if not self.bash or not Path(self.bash).is_file():
            self.skipTest("Bash is required to exercise the workflow")
        workflow = (
            MODULE_PATH.parents[1] / ".github/workflows/protect-file.yml"
        )
        self.workflow = workflow.read_text()

    def test_event_selection_including_branch_retargeting(self):
        workflow = yaml.load(self.workflow, Loader=yaml.BaseLoader)
        trigger = workflow["on"]["pull_request"]
        activities = trigger.get("types", ["opened", "synchronize", "reopened"])
        self.assertEqual(
            "github.base_ref == 'master'",
            workflow["jobs"]["minigraph-freeze"]["if"],
        )
        for action, branch, expected in [
            ("opened", "master", (True, True)),
            ("synchronize", "master", (True, True)),
            ("reopened", "master", (True, True)),
            ("edited", "master", (True, True)),
            ("opened", "202605", (False, True)),
            ("edited", "202605", (False, True)),
            ("edited", "feature/example", (False, False)),
        ]:
            selected = action in activities and any(
                fnmatch.fnmatchcase(branch, pattern)
                for pattern in trigger["branches"]
            )
            with self.subTest(action=action, branch=branch):
                self.assertEqual(
                    expected, (selected and branch == "master", selected)
                )

    def step_script(self, job, name):
        job_body = re.search(
            r"(?ms)^  " + re.escape(job) + r":\n(.*?)(?=^  \S|\Z)",
            self.workflow,
        )[1]
        step = re.search(
            r"(?ms)^      - name: " + re.escape(name)
            + r"\n(.*?)(?=^      - |\Z)", job_body,
        )
        self.assertIsNotNone(step, "Missing workflow step: " + name)
        script = re.search(r"(?ms)^        run: \|\n(.*)", step[1])
        self.assertIsNotNone(script, "Expected an inline Bash script")
        return textwrap.dedent(script[1])

    def commit(self, files, *parents):
        arguments = [
            "-c", "user.name=Fixture", "-c", "user.email=fixture@example.com",
            "commit-tree", self.tree(files),
        ]
        for parent in parents:
            arguments.extend(["-p", parent])
        return self.run_git(*arguments, data=b"Workflow test fixture\n")

    def shallow_merge(self, changes=None, depth=2):
        files = {
            "src/sonic-config-engine/minigraph.py": "original parser",
            "files/build_templates/init_cfg.json.j2": "{}",
        }
        old_base = self.commit(files)
        changes = changes or {"scripts/example.sh": "echo done"}
        head = self.commit(dict(files, **changes), old_base)
        files.update({
            "src/sonic-config-engine/minigraph.py": "upstream parser change",
            "files/build_templates/init_cfg.json.j2": '{"upstream": true}',
        })
        base = self.commit(files, old_base)
        merge = self.commit(dict(files, **changes), base, head)
        self.run_git("update-ref", "refs/heads/candidate", merge)
        checkout = self.repo / "checkout"
        self.run_git(
            "clone", "--quiet", "--no-local", "--depth", str(depth),
            "--branch", "candidate", self.repo.as_uri(), str(checkout),
        )
        self.repo = checkout
        return old_base, base, head, merge

    def run_step(self, job, name, **env):
        return subprocess.run(
            [self.bash, "--noprofile", "--norc", "-c",
             self.step_script(job, name)],
            cwd=self.repo, env=dict(os.environ, **env),
            capture_output=True, text=True, check=False,
        )

    def resolve(self, job, head, merge):
        output = self.repo / "comparison-output.txt"
        output.write_text("")
        result = self.run_step(
            job, "Resolve pull request comparison",
            PR_HEAD_SHA=head, MERGE_SHA=merge, GITHUB_OUTPUT=output.name,
        )
        values = dict(line.split("=", 1) for line in
                      output.read_text().splitlines())
        return result, values

    def test_advanced_base_and_shallow_checkout_use_matching_snapshots(self):
        old_base, base, head, merge = self.shallow_merge()
        with self.assertRaises(guard.AnalysisError):
            guard.git(self.repo, "cat-file", "-e", old_base)
        for job in ("minigraph-freeze", "check-file"):
            with self.subTest(job=job):
                result, values = self.resolve(job, head, merge)
                self.assertEqual(0, result.returncode, result.stderr)
                self.assertEqual({"base": base, "head": merge}, values)
                self.assertFalse(guard.check_changes(
                    self.repo, values["base"], values["head"]
                ))
        for branch in ("master", "202605"):
            result = self.run_step(
                "check-file", "Check protected files",
                BASE_SHA=base, HEAD_SHA=merge, BASE_BRANCH=branch,
                GUIDE_URL=guard.GUIDE_URL,
            )
            self.assertEqual(0, result.returncode, result.stderr)

    def test_pr_parser_changes_are_still_blocked(self):
        _, base, head, merge = self.shallow_merge({
            "src/sonic-config-engine/minigraph.py": "PR parser change",
        })
        for job in ("minigraph-freeze", "check-file"):
            result, values = self.resolve(job, head, merge)
            self.assertEqual(0, result.returncode, result.stderr)
            self.assertTrue(guard.check_changes(
                self.repo, values["base"], values["head"]
            ))
        for branch, expected in (("master", 0), ("202605", 1)):
            result = self.run_step(
                "check-file", "Check protected files",
                BASE_SHA=base, HEAD_SHA=merge, BASE_BRANCH=branch,
                GUIDE_URL=guard.GUIDE_URL,
            )
            self.assertEqual(expected, result.returncode, result.stderr)

    def test_mismatched_or_missing_revisions_fail_closed(self):
        _, base, head, merge = self.shallow_merge()
        for job in ("minigraph-freeze", "check-file"):
            for expected_head, expected_merge in (
                (base, merge), (head, base), ("", merge),
                (head, ""), (head, "missing"),
            ):
                with self.subTest(job=job, head=expected_head,
                                  merge=expected_merge):
                    result, values = self.resolve(
                        job, expected_head, expected_merge
                    )
                    self.assertNotEqual(0, result.returncode)
                    self.assertFalse(values)
            self.run_git("checkout", "--quiet", "--detach", head)
            result, values = self.resolve(job, head, head)
            self.assertNotEqual(0, result.returncode)
            self.assertFalse(values)
            self.run_git("checkout", "--quiet", "--detach", merge)

    def test_missing_merge_parents_fail_closed(self):
        _, _, head, merge = self.shallow_merge(depth=1)
        for job in ("minigraph-freeze", "check-file"):
            result, values = self.resolve(job, head, merge)
            self.assertNotEqual(0, result.returncode)
            self.assertFalse(values)


if __name__ == "__main__":
    unittest.main()
