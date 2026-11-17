"""Adapt generated vtysh command definitions to the upstream CLI test."""
from pathlib import Path
import re
import sys


source, destination = sys.argv[1:]
text = Path(source).read_text()
text = text.replace('"vtysh/vtysh.h"', '"tests/helpers/c/tests.h"')
text = text.replace('vtysh_init_cmd', 'test_init_cmd')
Path(destination).write_text(re.sub(r'VTYSH_[A-Z][A-Z_0-9]*', '0', text))
