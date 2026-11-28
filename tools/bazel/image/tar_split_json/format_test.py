"""Official CLI metadata and lossless TAR reconstruction are the format oracle."""
import gzip
import hashlib
import io
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest

TOOLS = [str(Path(p).absolute()) for p in sys.argv[1:]]
sys.argv = sys.argv[:1]


class TarSplitFormatTest(unittest.TestCase):
    def test_metadata_and_native_reassembly(self):
        frontend, apt_tar_split, pigz = TOOLS
        for archive_format, name in ((tarfile.USTAR_FORMAT, "plain"),
                                     (tarfile.PAX_FORMAT, "long-" + "x" * 160 + "-λ")):
            with self.subTest(format=archive_format), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                files = root / "payload"
                files.mkdir()
                data = b"payload\x00" * 129
                (files / name).write_bytes(data)
                (files / "empty").write_bytes(b"")
                stream = io.BytesIO()
                with tarfile.open(fileobj=stream, mode="w", format=archive_format) as archive:
                    for path, content in ((name, data), ("empty", b"")):
                        member = tarfile.TarInfo(path)
                        member.size, member.mode, member.uid, member.gid = len(content), 0o640, 123, 456
                        member.mtime = 123456789
                        if archive_format == tarfile.PAX_FORMAT:
                            member.pax_headers = {"SCHILY.xattr.user.contract": "raw tar metadata"}
                        archive.addfile(member, io.BytesIO(content))
                    for path, kind in (("symbolic", tarfile.SYMTYPE), ("hard", tarfile.LNKTYPE)):
                        member = tarfile.TarInfo(path)
                        member.type, member.linkname = kind, name
                        archive.addfile(member)
                # Bytes after the TAR terminator also belong to the caller's exact stream.
                original = stream.getvalue() + b"trailing bytes\x00\xff\x00"
                raw_json, cli_json = root / "raw.json", root / "apt.json.gz"
                result = subprocess.run([frontend, str(raw_json)], input=original,
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
                self.assertEqual(result.stdout, b"")
                subprocess.run([apt_tar_split, "disasm", "--no-stdout", "--output", str(cli_json), "-"],
                               input=original, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
                self.assertEqual(raw_json.read_bytes(), gzip.decompress(cli_json.read_bytes()))
                metadata, rebuilt = root / "pigz.json.gz", root / "rebuilt.tar"
                with metadata.open("wb") as output:
                    subprocess.run([pigz, "-n", "-c", str(raw_json)], stdout=output, check=True)
                subprocess.run([apt_tar_split, "asm", "--input", str(metadata), "--path", str(files),
                                "--output", str(rebuilt)], stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, check=True)
                self.assertEqual(hashlib.sha256(original).hexdigest(),
                                 hashlib.sha256(rebuilt.read_bytes()).hexdigest())

    def test_invalid_input_or_output_fails(self):
        frontend = TOOLS[0]
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertNotEqual(subprocess.run([frontend, str(root / "invalid.json")],
                                input=b"truncated tar", capture_output=True).returncode, 0)
            self.assertNotEqual(subprocess.run([frontend, str(root / "missing" / "out.json")],
                                input=b"\x00" * 1024, capture_output=True).returncode, 0)
        self.assertNotEqual(subprocess.run([frontend], input=b"", capture_output=True).returncode, 0)
        self.assertNotEqual(subprocess.run([frontend, "one", "two"], input=b"", capture_output=True).returncode, 0)


if __name__ == "__main__":
    unittest.main()
