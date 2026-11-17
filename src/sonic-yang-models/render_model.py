"""Render the two model variants selected by setup.py's build_py hook."""
import argparse
import ast
from pathlib import Path
import jinja2


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--validate', action='store_true')
    parser.add_argument('--setup')
    parser.add_argument('--source', action='append', default=[])
    parser.add_argument('--receipt')
    parser.add_argument('--template')
    parser.add_argument('--py')
    parser.add_argument('--cvl')
    args = parser.parse_args()
    if args.validate:
        tree = ast.parse(Path(args.setup).read_text())
        declared = next(ast.literal_eval(node.value) for node in tree.body
                        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'yang_files' for t in node.targets))
        expected = {Path(path).name.removesuffix('.j2') for path in args.source}
        if set(declared) != expected:
            raise ValueError('setup.py YANG manifest mismatch: missing=' + repr(sorted(expected - set(declared))) +
                             '; extra=' + repr(sorted(set(declared) - expected)))
        Path(args.receipt).write_text('YANG manifest matches declared model inputs\n')
        return
    source = Path(args.template)
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(str(source.parent)), trim_blocks=True)
    template = env.get_template(source.name)
    for kind, output in [('py', args.py), ('cvl', args.cvl)]:
        Path(output).write_text(template.render(yang_model_type=kind))


if __name__ == '__main__':
    main()
