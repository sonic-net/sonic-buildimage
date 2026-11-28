"""Run sonic-yang-models' wheel preparation in an isolated output directory."""

import argparse
import os
from pathlib import Path
import runpy
import shutil
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--setup", required=True)
    parser.add_argument("--readme", required=True)
    parser.add_argument("--source", action="append", default=[])
    parser.add_argument("--output", required=True)
    parser.add_argument("--wheel", required=True)
    args = parser.parse_args()

    setup = Path(args.setup).absolute()
    source_root = setup.parent
    output = Path(args.output).absolute()
    inputs = [setup, Path(args.readme).absolute()]
    inputs.extend(Path(source).absolute() for source in args.source)

    with tempfile.TemporaryDirectory(prefix="yang-models-", dir=output.parent) as temporary:
        workdir = Path(temporary)
        for source in sorted(inputs):
            destination = workdir / source.relative_to(source_root)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)

        # setup.py owns the manifest validation and the py/cvl template choice.
        # Its build inputs and setup requirements are all supplied by Bazel.
        previous_cwd = Path.cwd()
        previous_argv = sys.argv
        try:
            os.chdir(workdir)
            # The distribution has no Python modules, so bdist_wheel alone skips
            # build_py. Invoke its model-rendering hook explicitly first.
            sys.argv = ["setup.py", "build_py", "bdist_wheel", "--dist-dir", str(workdir / "dist")]
            runpy.run_path(str(workdir / "setup.py"), run_name="__main__")
        finally:
            sys.argv = previous_argv
            os.chdir(previous_cwd)

        # Bazel creates the declared tree output before launching the action.
        shutil.copytree(workdir / "yang-models", output, dirs_exist_ok=True)
        wheels = list((workdir / "dist").glob("*.whl"))
        if len(wheels) != 1:
            raise ValueError(f"Expected one source wheel, found {wheels}")
        shutil.copyfile(wheels[0], Path(args.wheel).absolute())


if __name__ == "__main__":
    main()
