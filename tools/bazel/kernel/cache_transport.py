"""Stream verified kernel artifacts through the existing Bazel gRPC CAS."""

from contextlib import contextmanager
import hashlib
import os
from pathlib import Path
import re
import tempfile
from urllib.parse import urlsplit
import uuid


class BlobMissing(FileNotFoundError):
    """The requested digest has been evicted or was never published."""


class CacheUnavailable(OSError):
    """The configured cache or client could not serve the request."""


class CacheIntegrityError(ValueError):
    """The bytes or size returned by the cache do not match the digest."""


def _resource_name(sha256, size):
    if (not isinstance(sha256, str) or not re.fullmatch(r"[0-9a-f]{64}", sha256)
            or type(size) is not int or not 0 <= size < 2**63):
        raise ValueError("Invalid SHA-256 CAS digest or size")
    return f"blobs/{sha256}/{size}"


class BlobCache:
    """Access the default REAPI instance used by the VS kernel producer."""

    def __init__(self, endpoint, *, timeout=300):
        parsed = urlsplit(endpoint)
        if (parsed.scheme not in {"grpc", "grpcs"} or not parsed.hostname
                or parsed.username is not None or parsed.password is not None
                or parsed.path not in {"", "/"} or parsed.query or parsed.fragment
                or any(char.isspace() for char in endpoint)):
            raise ValueError("Expected a gRPC cache endpoint without credentials or instance path")
        # Evaluating port rejects nonnumeric ports before passing the URI to gRPC.
        if parsed.port is None:
            raise ValueError("Expected an explicit gRPC cache port")
        # Local-only kernel builds also work on hosts without the Trixie slave's
        # pinned grpcio/grpcio-tools. Load the client only when a cache is used.
        import grpc
        self.grpc = grpc
        try:
            self.wire = grpc.protos("tools/bazel/kernel/cache_wire.proto")
        except NotImplementedError as error:
            raise CacheUnavailable("Kernel CAS requires grpcio-tools") from error
        # The shared build proxy is for source downloads; the cache is direct.
        options = [("grpc.enable_http_proxy", 0)]
        if parsed.scheme == "grpcs":
            self.channel = grpc.secure_channel(parsed.netloc, grpc.ssl_channel_credentials(), options=options)
        else:
            self.channel = grpc.insecure_channel(parsed.netloc, options=options)
        self.timeout = timeout
        self._read = self.channel.unary_stream(
            "/google.bytestream.ByteStream/Read",
            request_serializer=self.wire.ReadRequest.SerializeToString,
            response_deserializer=self.wire.ReadResponse.FromString)
        self._write = self.channel.stream_unary(
            "/google.bytestream.ByteStream/Write",
            request_serializer=self.wire.WriteRequest.SerializeToString,
            response_deserializer=self.wire.WriteResponse.FromString)
        self._find_missing = self.channel.unary_unary(
            "/build.bazel.remote.execution.v2.ContentAddressableStorage/FindMissingBlobs",
            request_serializer=self.wire.FindMissingBlobsRequest.SerializeToString,
            response_deserializer=self.wire.FindMissingBlobsResponse.FromString)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.channel.close()

    @contextmanager
    def _rpc(self, *, read=False):
        try:
            yield
        except self.grpc.RpcError as error:
            code = error.code()
            status = self.grpc.StatusCode
            if read and code == status.NOT_FOUND:
                raise BlobMissing("Kernel CAS blob was not found") from error
            if code == status.DATA_LOSS:
                raise CacheIntegrityError("Kernel CAS reported data loss") from error
            raise CacheUnavailable(f"Kernel CAS request failed: {code.name}") from error

    def missing(self, blobs):
        """Check a set of (sha256, size) locators without transferring their data."""
        requested = set(blobs)
        request = self.wire.FindMissingBlobsRequest(digest_function=1)  # REAPI SHA256
        for sha256, size in sorted(requested):
            _resource_name(sha256, size)
            request.blob_digests.add(hash=sha256, size_bytes=size)
        with self._rpc():
            response = self._find_missing(request, timeout=self.timeout)
        missing = {(blob.hash, blob.size_bytes) for blob in response.missing_blob_digests}
        if not missing.issubset(requested):
            raise CacheIntegrityError("Kernel CAS reported a digest that was not requested")
        return missing

    def download(self, sha256, size, dest):
        """Atomically install a verified blob; a failed read preserves dest."""
        request = self.wire.ReadRequest(resource_name=_resource_name(sha256, size))
        dest = Path(dest)
        with tempfile.TemporaryDirectory(prefix=".kernel-cas-", dir=dest.parent) as staging:
            incoming = Path(staging) / "blob"
            digest = hashlib.sha256()
            received = 0
            with incoming.open("xb") as stream, self._rpc(read=True):
                responses = self._read(request, timeout=self.timeout)
                try:
                    for response in responses:
                        chunk = response.data
                        received += len(chunk)
                        if received > size:
                            raise CacheIntegrityError("Kernel CAS blob exceeds its declared size")
                        digest.update(chunk)
                        stream.write(chunk)
                finally:
                    responses.cancel()
            if received != size or digest.hexdigest() != sha256:
                raise CacheIntegrityError("Kernel CAS blob failed size or SHA-256 verification")
            os.replace(incoming, dest)

    def upload(self, sha256, size, source):
        """Publish exactly the supplied file under its SHA-256 digest."""
        resource = f"uploads/{uuid.uuid4()}/{_resource_name(sha256, size)}"
        with Path(source).open("rb") as stream:
            digest = hashlib.sha256()
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
            if stream.tell() != size or digest.hexdigest() != sha256:
                raise CacheIntegrityError("Kernel CAS upload source failed size or SHA-256 verification")
            stream.seek(0)

            def requests():
                offset = 0
                for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                    yield self.wire.WriteRequest(resource_name=resource, write_offset=offset, data=chunk)
                    offset += len(chunk)
                yield self.wire.WriteRequest(resource_name=resource, write_offset=offset, finish_write=True)

            with self._rpc():
                response = self._write(requests(), timeout=self.timeout)
            if response.committed_size != size:
                raise CacheIntegrityError("Kernel CAS acknowledged an unexpected upload size")
