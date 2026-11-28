"""Exercise the kernel CAS protocol and verify interrupted or corrupt reads."""

from concurrent import futures
from contextlib import ExitStack
import hashlib
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.kernel.cache_transport import BlobCache, BlobMissing, CacheIntegrityError, CacheUnavailable


@unittest.skipUnless(importlib.util.find_spec("grpc") and importlib.util.find_spec("grpc_tools"),
                     "requires grpcio and grpcio-tools from the Trixie build slave")
class KernelCacheTransportTest(unittest.TestCase):
    def setUp(self):
        import grpc
        self.grpc = grpc
        self.wire = grpc.protos("tools/bazel/kernel/cache_wire.proto")
        self.objects = {}
        self.fault = None
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=2))
        server.add_generic_rpc_handlers((
            grpc.method_handlers_generic_handler("google.bytestream.ByteStream", {
                "Read": grpc.unary_stream_rpc_method_handler(
                    self.read, request_deserializer=self.wire.ReadRequest.FromString,
                    response_serializer=self.wire.ReadResponse.SerializeToString),
                "Write": grpc.stream_unary_rpc_method_handler(
                    self.write, request_deserializer=self.wire.WriteRequest.FromString,
                    response_serializer=self.wire.WriteResponse.SerializeToString),
            }),
            grpc.method_handlers_generic_handler("build.bazel.remote.execution.v2.ContentAddressableStorage", {
                "FindMissingBlobs": grpc.unary_unary_rpc_method_handler(
                    self.find_missing, request_deserializer=self.wire.FindMissingBlobsRequest.FromString,
                    response_serializer=self.wire.FindMissingBlobsResponse.SerializeToString),
            }),
        ))
        port = server.add_insecure_port("127.0.0.1:0")
        server.start()
        self.addCleanup(lambda: server.stop(0).wait())
        resources = ExitStack()
        self.addCleanup(resources.close)
        self.cache = resources.enter_context(BlobCache(f"grpc://127.0.0.1:{port}", timeout=5))
        self.directory = Path(resources.enter_context(tempfile.TemporaryDirectory()))

    def find_missing(self, request, context):
        self.assertEqual(request.digest_function, 1)
        return self.wire.FindMissingBlobsResponse(missing_blob_digests=[
            blob for blob in request.blob_digests if f"blobs/{blob.hash}/{blob.size_bytes}" not in self.objects])

    def write(self, requests, context):
        data = bytearray()
        for chunk in requests:
            self.assertEqual(chunk.write_offset, len(data))
            upload, _, blobs, sha256, size = chunk.resource_name.split("/")
            self.assertEqual((upload, blobs), ("uploads", "blobs"))
            data.extend(chunk.data)
            if chunk.finish_write:
                self.assertEqual((sha256, int(size)), (hashlib.sha256(data).hexdigest(), len(data)))
                self.objects[f"blobs/{sha256}/{size}"] = bytes(data)
                return self.wire.WriteResponse(committed_size=len(data))
        self.fail("upload ended without finish_write")

    def read(self, request, context):
        if request.resource_name not in self.objects:
            context.abort(self.grpc.StatusCode.NOT_FOUND, "absent")
        data = self.objects[request.resource_name]
        yield self.wire.ReadResponse(data=data[:2])
        if self.fault:
            context.abort(self.fault, "read interrupted")
        yield self.wire.ReadResponse(data=data[2:])

    def test_find_upload_download(self):
        source = self.directory / "source"
        result = self.directory / "installed"
        for data in (b"", b"abcd" * 600000):
            with self.subTest(size=len(data)):
                locator = (hashlib.sha256(data).hexdigest(), len(data))
                source.write_bytes(data)
                self.assertEqual(self.cache.missing([locator]), {locator})
                self.cache.upload(*locator, source)
                self.assertEqual(self.cache.missing([locator]), set())
                self.cache.download(*locator, result)
                self.assertEqual(result.read_bytes(), data)

    def test_wrong_upload_digest_is_rejected_before_publication(self):
        source = self.directory / "source"
        source.write_bytes(b"abcd")
        with self.assertRaises(CacheIntegrityError):
            self.cache.upload(hashlib.sha256(b"abce").hexdigest(), 4, source)
        self.assertFalse(self.objects)

    def test_failed_download_keeps_destination_and_removes_partial_data(self):
        dest = self.directory / "installed"
        dest.write_bytes(b"existing")
        sha256 = hashlib.sha256(b"abcd").hexdigest()
        resource = f"blobs/{sha256}/4"
        with self.assertRaises(BlobMissing):
            self.cache.download(sha256, 4, dest)
        for returned in (b"abc", b"wrong", b"abce"):
            with self.subTest(returned=returned):
                self.objects[resource] = returned
                with self.assertRaises(CacheIntegrityError):
                    self.cache.download(sha256, 4, dest)
        self.objects[resource] = b"abcd"
        for code in (self.grpc.StatusCode.UNAVAILABLE, self.grpc.StatusCode.PERMISSION_DENIED):
            with self.subTest(code=code):
                self.fault = code
                with self.assertRaises(CacheUnavailable):
                    self.cache.download(sha256, 4, dest)
        self.assertEqual(dest.read_bytes(), b"existing")
        self.assertEqual(list(self.directory.iterdir()), [dest])


if __name__ == "__main__":
    unittest.main()
