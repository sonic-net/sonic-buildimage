"""SONiC config plugin — installed by sonic-firebreak.deb into config/plugins/."""

from __future__ import annotations

import importlib.util
from pathlib import Path


def _load_impl():
    try:
        from config import firebreak as fb  # type: ignore
        return fb
    except ImportError:
        pass
    path = Path("/opt/firebreak/cli/config_firebreak.py")
    if not path.is_file():
        raise ImportError("FIREBREAK config implementation not found")
    spec = importlib.util.spec_from_file_location("config_firebreak_impl", path)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def register(cli):
    fb = _load_impl()
    node = fb.firebreak
    if node.name in cli.commands:
        return
    cli.add_command(node)
