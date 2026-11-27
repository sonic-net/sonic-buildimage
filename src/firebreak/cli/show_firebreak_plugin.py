"""SONiC show plugin — installed by sonic-firebreak.deb into show/plugins/.

Registers the same commands as show/firebreak.py in sonic-utilities so
`show firebreak` works as soon as the host package is installed, even before
a sonic-utilities rebuild that vendors the first-class module.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path


def _load_impl():
    # Prefer first-class module if sonic-utilities already ships it.
    try:
        from show import firebreak as fb  # type: ignore
        return fb
    except ImportError:
        pass
    # Fallback: copy shipped next to this plugin or under /opt/firebreak
    candidates = [
        Path("/opt/firebreak/cli/show_firebreak.py"),
        Path(__file__).resolve().parent / "show_firebreak.py",
    ]
    for path in candidates:
        if not path.is_file():
            continue
        spec = importlib.util.spec_from_file_location("show_firebreak_impl", path)
        mod = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(mod)
        return mod
    raise ImportError("FIREBREAK show implementation not found")


def register(cli):
    fb = _load_impl()
    node = fb.firebreak
    if node.name in cli.commands:
        return
    cli.add_command(node)
