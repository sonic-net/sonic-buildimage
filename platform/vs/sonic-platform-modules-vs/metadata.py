"""Generate native setuptools metadata and bytecode from the original setup data."""
import ast
from pathlib import Path
import py_compile
import sys
from setuptools import Distribution

setup, output, *sources = map(Path, sys.argv[1:])
tree = ast.parse(setup.read_text())
call = next(node for node in ast.walk(tree) if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == 'setup')
values = {keyword.arg: ast.literal_eval(keyword.value) for keyword in call.keywords}
if values['packages'] != ['sonic_platform']:
    raise ValueError('review changed platform package installation layout')
output.mkdir(parents=True, exist_ok=True)
with (output / 'PKG-INFO').open('w') as stream:
    Distribution(values).metadata.write_pkg_file(stream)
for source in sources:
    py_compile.compile(str(source), cfile=str(output / (source.stem + '.cpython-313.pyc')),
        dfile='/usr/lib/python3/dist-packages/sonic_platform/' + source.name,
        doraise=True, invalidation_mode=py_compile.PycInvalidationMode.CHECKED_HASH)
