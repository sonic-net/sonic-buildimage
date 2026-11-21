"""Keep compatible parent wheel versions and the actual APT setuptools source."""
from pathlib import Path
import sys
from runtime_lock import locked_packages

# The base installs Debian setuptools78.1.1. Its Python development lock's
# setuptools80.9.0 is absent from the parent runtime; grpcio-tools needs >=77.
packages = locked_packages(Path(sys.argv[1]).read_text())
Path(sys.argv[2]).write_text("# Parent wheels; setuptools comes from the declared APT producer.\n" + "".join(
    packages[name]["requirement"] + "\n" for name in sorted(packages) if name != "setuptools"))
