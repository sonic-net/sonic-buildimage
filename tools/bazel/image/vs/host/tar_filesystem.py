"""Read the image rules' merged TAR tree without extracting payload files."""
import fnmatch
from pathlib import PurePosixPath
from types import SimpleNamespace

from merge_layers import Dir, split_path


class VirtualPath:
    def __init__(self, fs, path):
        self.fs, self.path = fs, path

    def __truediv__(self, name):
        return VirtualPath(self.fs, self.path.rstrip("/") + "/" + name)

    def stat(self):
        node = self.fs.node(self.path)
        return SimpleNamespace(st_dev=0, st_ino=id(node))

    def is_dir(self):
        return isinstance(self.fs.node(self.path), Dir)

    def is_file(self):
        node = self.fs.node(self.path)
        return not isinstance(node, Dir) and node.src is not None

    def is_symlink(self):
        node = self.fs.node(self.path, follow=False)
        return not isinstance(node, Dir) and node.is_symlink()

    def read_bytes(self):
        node = self.fs.node(self.path)
        if isinstance(node, Dir) or node.src is None:
            raise IsADirectoryError(self.path)
        if isinstance(node.src, bytes):
            return node.src
        archive, member = node.src
        return archive.extractfile(member).read()


class TarFilesystem:
    def __init__(self, tree):
        self.tree = tree

    def node(self, path, follow=True):
        parts = list(PurePosixPath(path).parts)
        parts = [part for part in parts if part != "/"]
        seen = set()
        while True:
            node = self.tree.lookup(parts)
            if node is None:
                raise FileNotFoundError(path)
            if not follow or isinstance(node, Dir) or not node.is_symlink():
                return node
            key = tuple(parts)
            if key in seen:
                raise OSError("TAR symlink cycle: " + path)
            seen.add(key)
            parts = split_path(node.linkname) if node.linkname.startswith("/") else parts[:-1] + node.linkname.split("/")

    def resolve(self, path):
        return VirtualPath(self, path)

    def read(self, path):
        return self.resolve(path).read_bytes()

    def listdir(self, path):
        try:
            node = self.node(path)
        except (FileNotFoundError, OSError):
            return []
        return sorted(node.children) if isinstance(node, Dir) else []

    def glob(self, pattern):
        paths = [""]
        for part in pattern.split("/"):
            if part:
                paths = [base + "/" + child for base in paths for child in self.listdir(base)
                         if (not child.startswith(".") or part.startswith(".")) and fnmatch.fnmatchcase(child, part)]
        return sorted(paths)
