"""Reject unreviewed Python port drift and conflicting PyO3 requirements."""
import ast
import hashlib
import json
from pathlib import Path
import sys
import tomllib

HERE = Path(__file__).resolve().parent


def syntax_fingerprint(tree):
    # ast.dump changed its empty-field output in Python 3.13. Encode fields
    # explicitly, omitting only empty type_params introduced in Python 3.12.
    def encode(value):
        if isinstance(value, ast.AST):
            return [type(value).__name__, [(key, encode(item))
                    for key, item in ast.iter_fields(value)
                    if key != 'type_params' or item]]
        if isinstance(value, list):
            return [encode(item) for item in value]
        return value
    data = json.dumps(encode(tree), ensure_ascii=True, default=repr)
    return hashlib.sha256(data.encode()).hexdigest()


def fingerprint(path):
    tree = ast.parse(path.read_text())
    return syntax_fingerprint(tree)


def source_problems(source_root, manifest):
    problems = []
    for name, expected in manifest['sources'].items():
        path = source_root / name
        if not path.is_file():
            problems.append('missing Python port source: ' + str(path))
        elif fingerprint(path) != expected:
            problems.append('Python port changed; review the native implementation before updating '
                            'python_port_manifest.json: ' + name)
    return problems


def pyo3_requirement(path):
    with path.open('rb') as stream:
        dependency = tomllib.load(stream)['dependencies']['pyo3']
    version = dependency['version'] if isinstance(dependency, dict) else dependency
    return tuple(version.lstrip('^~=').split('.')[:2])


def version_problems(common_manifest, vendor_manifest):
    if pyo3_requirement(common_manifest) != pyo3_requirement(vendor_manifest):
        return ['PyO3 requirement differs between platform-pyo3 and platform-native; '
                'update both before building a provider']
    return []


def main():
    root = HERE.parents[2]
    manifest = json.loads((HERE / 'python_port_manifest.json').read_text())
    try:
        problems = source_problems(HERE.parent / 'mlnx-platform-api' / 'sonic_platform', manifest)
        problems += version_problems(root / 'src/sonic-platform-common/crates/platform-pyo3/Cargo.toml',
                                     HERE / 'Cargo.toml')
    except (OSError, KeyError, ValueError, SyntaxError) as error:
        problems = ['Cannot check native port inputs: ' + str(error)]
    for problem in problems:
        print(problem, file=sys.stderr)
    if not problems:
        print('Python port sources and PyO3 requirements match the reviewed baseline')
    return bool(problems)


if __name__ == '__main__':
    raise SystemExit(main())
