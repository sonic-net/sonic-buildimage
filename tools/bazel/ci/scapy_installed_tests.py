"""Run the pinned Scapy wheel contract and upstream campaigns against the wheel installed in VS."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET

from tools.bazel.ci.vs_python_packages import package_path, SCAPY_WHEEL


def stage_tests(bazel, package):
    """Use the original test entry point, fixtures, dependencies and Python version in both probes."""
    package = package_path(package)
    if not (package / SCAPY_WHEEL).is_file():
        raise ValueError("Missing staged VS Scapy wheel")
    owner_tests = ("wheel_test", "upstream_test")
    query = "set(" + " ".join("@scapy//:" + name for name in (*owner_tests, "validation.py")) + ")"
    owner = ET.fromstring(subprocess.check_output([bazel, "query", "--output=xml", query]))
    entry = owner.find("source-file[@name='@scapy//:validation.py']")
    if entry is None:
        raise ValueError("Scapy owner no longer contains validation.py")
    staged = package / "scapy"
    staged.mkdir()
    # The pinned registry owns validation.py but does not export it across packages.
    source = Path(entry.attrib["location"].rsplit(":", 2)[0])
    shutil.copyfile(source, staged / "validation.py")
    wheel = f"//{package}:{SCAPY_WHEEL}"
    definitions = ['load("@rules_python//python:py_test.bzl", "py_test")']
    for name in owner_tests:
        rule = owner.find(f"rule[@name='@scapy//:{name}'][@class='py_test']")
        if rule is None or rule.find("label[@name='main'][@value='@scapy//:validation.py']") is None:
            raise ValueError("Missing original Scapy validation entry point for " + name)
        args = [arg.attrib["value"] for arg in rule.findall("list[@name='args']/string")]
        if args != [name.removesuffix("_test"), "$(rootpath :wheel)", "$(rootpath setup.py)"]:
            raise ValueError("Scapy owner changed the arguments for " + name)
        data = [item.attrib["value"] for item in rule.findall("list[@name='data']/label")]
        if "@scapy//:wheel" not in data:
            raise ValueError("Scapy owner test no longer consumes its wheel: " + name)
        attributes = {
            "name": name, "srcs": ["validation.py"], "main": "validation.py",
            "args": [args[0], f"$(rootpath {wheel})", "$(rootpath @scapy//:setup.py)"],
            "data": [wheel if item == "@scapy//:wheel" else item for item in data],
            "deps": [item.attrib["value"] for item in rule.findall("list[@name='deps']/label")],
        }
        for attr in ("size", "timeout", "python_version"):
            value = rule.find(f"string[@name='{attr}']")
            if value is not None:
                attributes[attr] = value.attrib["value"]
        definitions.append("py_test(\n" + "".join(f"    {key} = {json.dumps(value)},\n"
                                                   for key, value in attributes.items()) + ")")
    (staged / "BUILD.bazel").write_text("\n\n".join(definitions) + "\n")
    return [f"//{staged}:{name}" for name in owner_tests]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bazel", required=True)
    parser.add_argument("--package", type=Path, required=True)
    arguments = parser.parse_args()
    print(*stage_tests(arguments.bazel, arguments.package), sep="\n")
