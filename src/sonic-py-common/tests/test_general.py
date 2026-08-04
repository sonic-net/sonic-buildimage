import sys
import pytest
import subprocess
from sonic_py_common.general import getstatusoutput_noshell, getstatusoutput_noshell_pipe, check_output_pipe


def test_getstatusoutput_noshell(tmp_path):
    exitcode, output = getstatusoutput_noshell(['echo', 'sonic'])
    assert (exitcode, output) == (0, 'sonic')

    exitcode, output = getstatusoutput_noshell([sys.executable, "-c", "import sys; sys.exit(6)"])
    assert exitcode != 0

def test_getstatusoutput_noshell_pipe():
    exitcode, output = getstatusoutput_noshell_pipe(['echo', 'sonic'], ['awk', '{print $1}'])
    assert (exitcode, output) == ([0, 0], 'sonic')

    exitcode, output = getstatusoutput_noshell_pipe([sys.executable, "-c", "import sys; sys.exit(6)"], [sys.executable, "-c", "import sys; sys.exit(8)"])
    assert exitcode == [6, 8]

def test_check_output_pipe():
    output = check_output_pipe(['echo', 'sonic'], ['awk', '{print $1}'])
    assert output == 'sonic\n'

    with pytest.raises(subprocess.CalledProcessError) as e:
        check_output_pipe([sys.executable, "-c", "import sys; sys.exit(6)"], [sys.executable, "-c", "import sys; sys.exit(0)"])
        assert e.returncode == [6, 0]

    with pytest.raises(subprocess.CalledProcessError) as e:
        check_output_pipe([sys.executable, "-c", "import sys; sys.exit(0)"], [sys.executable, "-c", "import sys; sys.exit(6)"])
        assert e.returncode == [0, 6]
