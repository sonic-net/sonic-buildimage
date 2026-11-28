"""Generate only final cache files from ordered declared installation TARs."""
from contextlib import ExitStack
import io
import json
from pathlib import Path
import sys
import tarfile

from ldcache import cache_entries, empty_aux_cache, generate_cache_from_fs
from merge_layers import Tree, apply_layer
from tar_filesystem import TarFilesystem


def main(spec_path, output, audit):
    spec = json.loads(Path(spec_path).read_text())
    tree = Tree()
    with ExitStack() as stack:
        for path in spec["layers"]:
            archive = stack.enter_context(tarfile.open(path, "r:*"))
            apply_layer(tree, archive, path)
        cache = generate_cache_from_fs(TarFilesystem(tree), spec["architecture"])
        with tarfile.open(output, "w", format=tarfile.PAX_FORMAT) as archive:
            for name, contents in [("etc/ld.so.cache", cache), ("var/cache/ldconfig/aux-cache", empty_aux_cache())]:
                member = tarfile.TarInfo(name)
                member.mode, member.size = 0o644, len(contents)
                archive.addfile(member, io.BytesIO(contents))
    Path(audit).write_text(json.dumps({"architecture": spec["architecture"], "entries": cache_entries(cache)}, indent=2) + "\n")


if __name__ == "__main__":
    main(*sys.argv[1:])
