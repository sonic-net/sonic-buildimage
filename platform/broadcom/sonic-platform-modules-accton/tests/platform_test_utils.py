"""Load platform definitions without executing device discovery or monitor loops."""

import ast
import logging
import math
from pathlib import Path
import sys
import time
from unittest.mock import Mock

SOURCE = Path(__file__).resolve().parents[4]
BASE = "platform/broadcom/sonic-platform-modules-accton/"
DEVICE = "device/accton/x86_64-accton_"
MODELS = ("as5835_54x", "as7326_56x", "as7726_32x", "as7816_64x")


def definitions(path, **environment):
    tree = ast.parse((SOURCE / path).read_text(), filename=path)
    tree.body = [
        node
        for node in tree.body
        if isinstance(node, (ast.FunctionDef, ast.ClassDef, ast.Assign, ast.AnnAssign))
    ]
    scope = {
        "logging": logging,
        "math": math,
        "time": time,
        "subprocess": Mock(),
        "sys": sys,
        **environment,
    }
    exec(compile(tree, path, "exec"), scope)
    return scope


def platform_directory(model):
    return (
        DEVICE + model + "-r0/sonic_platform/"
        if model == "as5835_54x"
        else BASE + model.replace("_", "-") + "/sonic_platform/"
    )
