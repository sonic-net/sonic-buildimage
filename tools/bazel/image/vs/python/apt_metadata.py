"""Read the distribution identity shipped in a declared Debian Python payload."""
import argparse
from email.parser import BytesParser
import json
from pathlib import Path
import tarfile
from packaging.utils import canonicalize_name


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--archive', required=True)
    parser.add_argument('--distribution', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    wanted = canonicalize_name(args.distribution)
    matches = []
    with tarfile.open(args.archive) as archive:
        for member in archive:
            if not member.isfile() or not member.name.endswith(('.dist-info/METADATA', '.egg-info/PKG-INFO', '.egg-info')):
                continue
            metadata = BytesParser().parsebytes(archive.extractfile(member).read())
            if canonicalize_name(metadata['Name'] or '') != wanted:
                continue
            if not metadata['Version']:
                raise ValueError('Debian distribution has no Version: ' + args.distribution)
            matches.append({
                'distribution': metadata['Name'], 'version': metadata['Version'],
                'requires_dist': metadata.get_all('Requires-Dist', []),
                'requires_python': metadata['Requires-Python'] or '',
                'extras': metadata.get_all('Provides-Extra', []),
            })
    if len(matches) != 1:
        raise ValueError(f'Expected one {args.distribution} metadata record, got {len(matches)}')
    Path(args.output).write_text(json.dumps(matches[0], sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
