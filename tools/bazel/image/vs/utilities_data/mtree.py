"""Map the original Debian install recipe without copying its templates."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--completions', type=Path, required=True)
parser.add_argument('--template-root', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--doc', action='append', default=[])
parser.add_argument('templates', nargs='*', type=Path)
args = parser.parse_args()

def escape(path):
    return str(path).replace('\\', '\\134').replace(' ', '\\040').replace('\t', '\\011').replace('\n', '\\012')

mapping = {'etc/bash_completion.d/' + file.name: file for file in sorted(args.completions.iterdir())}
mapping.update({'usr/share/sonic/templates/' + str(file.relative_to(args.template_root)): file for file in args.templates})
mapping.update({'usr/share/doc/sonic-utilities-data/' + value.split('=', 1)[0]: Path(value.split('=', 1)[1]) for value in args.doc})
args.output.write_text(''.join(escape(name) + ' type=file uid=0 gid=0 mode=0644 time=0 content=' + escape(file) + '\n' for name, file in sorted(mapping.items())))
