"""Stream OCI layers into a Docker overlay2 store TAR without extracting them.

Store layout follows Docker's image/overlay2 layerdb and overlay2 driver.
Chain/link naming and metadata layout follow sonic_rules/dockerfs at
132009e6b3402d3711e1426c7567c611b8dd4db6; unlike that implementation, ownership,
whiteouts and opaque xattrs are emitted directly as TAR metadata.
"""
import argparse
import base64
import copy
import gzip
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import subprocess
import tarfile
import tempfile
from contextlib import ExitStack

from sonic_oci.layout import validate_layout
from image_config import compressed_output


def relative(name):
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError("unsafe layer member: " + name)
    return str(path) if str(path) != "." else ""


def entry(archive, name, data=b"", mode=0o644, kind=tarfile.REGTYPE, link=""):
    info = tarfile.TarInfo(name)
    info.mode, info.type, info.linkname = mode, kind, link
    info.size = len(data)
    archive.addfile(info, io.BytesIO(data))


class StoreWriter:
    """Emit parents before their children, as libarchive population requires."""
    def __init__(self, archive):
        self.archive, self.directories = archive, set()

    def addfile(self, info, data=None):
        for parent in reversed(PurePosixPath(info.name).parents):
            name = str(parent)
            if name != "." and name not in self.directories:
                self.directories.add(name)
                entry(self, name, mode=0o755, kind=tarfile.DIRTYPE)
        if info.isdir():
            self.directories.add(info.name)
        self.archive.addfile(info, data)


class HashReader:
    def __init__(self, source, tee=None):
        self.source, self.size, self.hash = source, 0, hashlib.sha256()
        self.tee = tee

    def read(self, size=-1):
        data = self.source.read(size)
        self.size += len(data)
        self.hash.update(data)
        if self.tee is not None:
            self.tee.write(data)
        return data


def stream_layer(archive, blob, diff_id, prefix, zstd, tar_split_json, metadata_path):
    """Stream exact layer bytes to tar-split and return Moby header-data size."""
    process = None
    with Path(blob).open("rb") as raw:
        magic = raw.read(4)
        raw.seek(0)
        if magic[:2] == b"\x1f\x8b":
            source = gzip.GzipFile(fileobj=raw)
        elif magic == b"\x28\xb5\x2f\xfd":
            if not zstd:
                raise ValueError("zstd layer requires a declared zstd executable")
            process = subprocess.Popen([zstd, "-dc", str(blob)], stdout=subprocess.PIPE)
            source = process.stdout
        else:
            source = raw
        splitter = subprocess.Popen([tar_split_json, str(metadata_path)], stdin=subprocess.PIPE)
        reader = HashReader(source, splitter.stdin)
        directories, opaque = {}, set()
        size = 0
        failed = False
        try:
            with tarfile.open(fileobj=reader, mode="r|") as layer:
                for original in layer:
                    size += original.size
                    name = relative(original.name)
                    if not name:
                        continue
                    path = PurePosixPath(name)
                    if path.name == ".wh..wh..opq":
                        opaque.add(str(path.parent))
                        continue
                    info = copy.copy(original)
                    info.pax_headers = dict(original.pax_headers)
                    info.pax_headers.pop("path", None)
                    info.pax_headers.pop("linkpath", None)
                    info.name = prefix + "/" + name
                    info.uname = info.gname = ""
                    if path.name.startswith(".wh."):
                        if path.name.startswith(".wh..wh."):
                            raise ValueError("unsupported OCI whiteout " + name)
                        target = path.name[4:]
                        if target in ("", ".", ".."):
                            raise ValueError("invalid OCI whiteout target " + name)
                        info.name = prefix + "/" + str(path.with_name(target))
                        info.type, info.size = tarfile.CHRTYPE, 0
                        info.devmajor = info.devminor = 0
                        archive.addfile(info)
                        continue
                    if info.islnk():
                        info.linkname = prefix + "/" + relative(info.linkname)
                    if info.isdir():
                        directories[name] = info
                    archive.addfile(info, layer.extractfile(original) if info.isfile() else None)
            while reader.read(1024 * 1024):
                pass
            for name in sorted(opaque):
                key = "" if name == "." else name
                info = copy.copy(directories.get(key, tarfile.TarInfo(prefix + ("/" + key if key else ""))))
                if key not in directories:
                    info.type, info.mode = tarfile.DIRTYPE, 0o755
                info.pax_headers = dict(info.pax_headers)
                info.pax_headers["SCHILY.xattr.trusted.overlay.opaque"] = "y"
                archive.addfile(info)
        except BaseException:
            failed = True
            splitter.terminate()
            if process:
                process.terminate()
            raise
        finally:
            try:
                splitter.stdin.close()
            except BrokenPipeError:
                if not failed:
                    raise ValueError("tar-split closed its input stream")
            if splitter.wait() != 0 and not failed:
                raise ValueError("tar-split metadata generation failed")
            if process and process.wait() != 0 and not failed:
                raise ValueError("zstd layer decompression failed")
        if "sha256:" + reader.hash.hexdigest() != diff_id:
            raise ValueError("OCI uncompressed diff digest mismatch: " + str(blob))
        return size


def build(images, version, platform, output, zstd=None, tar_split_json=None, compression="gzip", pigz=None):
    if not tar_split_json:
        raise ValueError("a declared tar_split_json executable is required")
    if not pigz:
        raise ValueError("a declared pigz executable is required for Docker metadata")
    if compression not in ("none", "gzip", "zstd"):
        raise ValueError("unsupported Docker store compression: " + compression)
    repositories, seen = {}, set()
    with ExitStack() as stack:
        compressed = stack.enter_context(compressed_output(output, compression, zstd, pigz))
        tar = stack.enter_context(tarfile.open(fileobj=compressed, mode="w|", format=tarfile.PAX_FORMAT))
        scratch = Path(stack.enter_context(tempfile.TemporaryDirectory(prefix="tar-split-")))
        archive = StoreWriter(tar)
        entry(archive, "overlay2/l", mode=0o755, kind=tarfile.DIRTYPE)
        entry(archive, "image/overlay2/layerdb/tmp", mode=0o755, kind=tarfile.DIRTYPE)
        for layout_path, tags in images:
            layout = validate_layout(Path(layout_path), platform)
            config_path = Path(layout_path) / "blobs/sha256" / layout.manifest["config"]["digest"].split(":")[1]
            config_bytes = config_path.read_bytes()
            image_id = hashlib.sha256(config_bytes).hexdigest()
            entry(archive, "image/overlay2/imagedb/content/sha256/" + image_id, config_bytes)
            chains, shorts = [], []
            for blob, diff_id in zip(layout.layers, layout.config["rootfs"]["diff_ids"]):
                chain = diff_id[7:] if not chains else hashlib.sha256(("sha256:" + chains[-1] + " " + diff_id).encode()).hexdigest()
                short = base64.b32encode(hashlib.sha256(chain.encode()).digest()).decode()[:26]
                chains.append(chain)
                shorts.append(short)
                if chain in seen:
                    continue
                seen.add(chain)
                root = "overlay2/" + chain
                entry(archive, root + "/diff", mode=0o755, kind=tarfile.DIRTYPE)
                metadata_path = scratch / (chain + ".json")
                size = stream_layer(archive, blob, diff_id, root + "/diff", zstd, tar_split_json, metadata_path)
                entry(archive, root + "/link", short.encode())
                entry(archive, root + "/committed")
                if len(chains) > 1:
                    entry(archive, root + "/lower", ":".join("l/" + s for s in reversed(shorts[:-1])).encode())
                entry(archive, "overlay2/l/" + short, kind=tarfile.SYMTYPE, mode=0o777, link="../" + chain + "/diff")
                db = "image/overlay2/layerdb/sha256/" + chain
                for key, value in [("diff", diff_id), ("cache-id", chain), ("size", str(size))]:
                    entry(archive, db + "/" + key, value.encode())
                compressed_metadata = metadata_path.with_suffix(".json.gz")
                with metadata_path.open("rb") as source, compressed_metadata.open("wb") as destination:
                    subprocess.run([pigz, "-n", "-c"], stdin=source, stdout=destination, check=True)
                metadata_path.unlink()
                info = tarfile.TarInfo(db + "/tar-split.json.gz")
                info.mode, info.size = 0o644, compressed_metadata.stat().st_size
                with compressed_metadata.open("rb") as stream:
                    archive.addfile(info, stream)
                compressed_metadata.unlink()
                if len(chains) > 1:
                    entry(archive, db + "/parent", ("sha256:" + chains[-2]).encode())
            for tag in tags.split(","):
                tag = tag.replace("{version}", version)
                if ":" not in tag or "{version}" in tag:
                    raise ValueError("invalid Docker tag " + tag)
                repo = tag.rsplit(":", 1)[0]
                previous = repositories.setdefault(repo, {}).get(tag)
                if previous and previous != "sha256:" + image_id:
                    raise ValueError("conflicting Docker tag " + tag)
                repositories[repo][tag] = "sha256:" + image_id
        entry(archive, "image/overlay2/repositories.json", json.dumps({"Repositories": repositories}, sort_keys=True, separators=(",", ":")).encode())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("spec")
    args = parser.parse_args()
    spec = json.loads(Path(args.spec).read_text())
    version = Path(spec["version_file"]).read_text().strip() if spec.get("version_file") else ""
    build(spec["images"], version, spec["platform"], spec["out"], spec.get("zstd"), spec.get("tar_split_json"), spec.get("compression", "gzip"), spec.get("pigz"))


if __name__ == "__main__":
    main()
