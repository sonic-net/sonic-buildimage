# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Ephemeral HTTP server that receives diagnostic dump uploads from DPUs over the
per-install tmfifo bridge during a sonic-bfb-installer run.

Endpoint shape (PUT or POST, with the file as the raw request body):
    /<dpu_name>/<filename>

Files are saved to ``<output_dir>/<dpu_name>/<filename>``. ``<dpu_name>`` must
match ``dpu<digits>`` and ``<filename>`` must be a single safe basename
(no path separators, no parent refs). Anything else returns HTTP 400.

The server is intentionally minimal:
    * No authentication. It is meant to listen only on the internal tmfifo
      bridge IP (e.g. 192.168.100.254), reachable solely by attached DPUs
      during installation.
    * Single-process, threaded request handler.
    * Bounded by ``--max-bytes`` per upload (default 256 MiB) to avoid
      runaway writes on bad input.
    * SIGTERM / SIGINT trigger a clean shutdown.

Run as a module:
    python3 -m mellanox_bfb_installer.dump_receiver \\
        --bind-ip 192.168.100.254 --port 8090 \\
        --output-dir /var/log/sonic-bfb-installer/dumps
"""

import argparse
import logging
import os
import re
import signal
import socket
import sys
import threading
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Optional, Tuple


DEFAULT_BIND_IP = "192.168.100.254"
DEFAULT_PORT = 8090
DEFAULT_OUTPUT_DIR = "/var/log/sonic-bfb-installer/dumps"
DEFAULT_MAX_BYTES = 256 * 1024 * 1024  # 256 MiB
READ_CHUNK_SIZE = 64 * 1024
# Per-read socket timeout for the request body. Prevents a client that sends
# headers and then stalls from blocking a handler thread indefinitely.
BODY_READ_TIMEOUT_SEC = 30

# A DPU name like "dpu0", "dpu12". This must match the convention used by
# device_selection / main._dpu_name_to_id.
_DPU_NAME_RE = re.compile(r"^dpu\d+$")
# A filename must be a single basename: no path separators, no parent refs,
# no leading dot. Allow letters, digits, dot, dash, underscore.
_FILENAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,254}$")


logger = logging.getLogger("dump_receiver")


def _parse_target_path(url_path: str) -> Tuple[str, str]:
    """Validate and split ``/<dpu_name>/<filename>`` from the request URL.

    Returns (dpu_name, filename). Raises ValueError on any malformed input.
    """
    # Strip query string / fragment defensively even though we don't use them.
    path = url_path.split("?", 1)[0].split("#", 1)[0]
    if not path.startswith("/"):
        raise ValueError("path must start with '/'")
    parts = path.lstrip("/").split("/")
    if len(parts) != 2:
        raise ValueError("expected /<dpu_name>/<filename>")
    dpu_name, filename = parts
    if not _DPU_NAME_RE.match(dpu_name):
        raise ValueError(f"invalid dpu name: {dpu_name!r}")
    if not _FILENAME_RE.match(filename):
        raise ValueError(f"invalid filename: {filename!r}")
    return dpu_name, filename


class _DumpReceiverConfig:
    """Server-wide config that the handler instances read."""

    def __init__(self, output_dir: str, max_bytes: int) -> None:
        self.output_dir = output_dir
        self.max_bytes = max_bytes


class _DumpReceiverHandler(BaseHTTPRequestHandler):
    # Attached by the server factory below.
    receiver_config: Optional[_DumpReceiverConfig] = None

    # Keep the canned protocol so 1.1 keepalive isn't attempted; each upload
    # is a one-shot connection from curl in the BFB installer's ex_chroot.
    protocol_version = "HTTP/1.0"

    server_version = "sonic-bfb-installer-dump-receiver/1.0"

    # ---- Logging ----------------------------------------------------------

    def log_message(self, format: str, *args) -> None:  # noqa: A002 - stdlib API
        # Route the access log through the module logger so the parent
        # process's logging pipeline picks it up via stdout/stderr.
        logger.info("%s - - %s", self.client_address[0], format % args)

    # ---- Helpers ----------------------------------------------------------

    def _reply(self, status: HTTPStatus, body: str = "") -> None:
        body_bytes = body.encode("utf-8") if body else b""
        try:
            self.send_response(status)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body_bytes)))
            self.send_header("Connection", "close")
            self.end_headers()
            if body_bytes:
                self.wfile.write(body_bytes)
        except (BrokenPipeError, ConnectionResetError):
            # Client (DPU curl) hung up. Nothing actionable.
            pass

    def _save_upload(self) -> None:
        cfg = self.receiver_config
        assert cfg is not None  # set by server factory

        try:
            dpu_name, filename = _parse_target_path(self.path)
        except ValueError as e:
            self._reply(HTTPStatus.BAD_REQUEST, f"bad target path: {e}\n")
            return

        # Content-Length is required so we can enforce the size cap up front
        # and read exactly the right number of bytes.
        cl_header = self.headers.get("Content-Length")
        if cl_header is None:
            self._reply(HTTPStatus.LENGTH_REQUIRED, "Content-Length required\n")
            return
        try:
            content_length = int(cl_header)
        except ValueError:
            self._reply(HTTPStatus.BAD_REQUEST, "invalid Content-Length\n")
            return
        if content_length < 0:
            self._reply(HTTPStatus.BAD_REQUEST, "negative Content-Length\n")
            return
        if content_length > cfg.max_bytes:
            self._reply(
                HTTPStatus.REQUEST_ENTITY_TOO_LARGE,
                f"upload too large ({content_length} > {cfg.max_bytes})\n",
            )
            return

        dest_dir = os.path.join(cfg.output_dir, dpu_name)
        try:
            os.makedirs(dest_dir, mode=0o755, exist_ok=True)
        except OSError as e:
            logger.error("mkdir failed for %s: %s", dest_dir, e)
            self._reply(HTTPStatus.INTERNAL_SERVER_ERROR, "mkdir failed\n")
            return

        dest_path = os.path.join(dest_dir, filename)
        # Belt-and-suspenders: confirm the resolved destination stays under
        # the configured output directory.
        out_real = os.path.realpath(cfg.output_dir)
        dest_real = os.path.realpath(dest_path)
        if not (dest_real == out_real or dest_real.startswith(out_real + os.sep)):
            logger.error("rejecting escape attempt: dest=%s base=%s", dest_real, out_real)
            self._reply(HTTPStatus.BAD_REQUEST, "invalid destination\n")
            return

        tmp_path = dest_path + ".part"
        bytes_remaining = content_length
        # Bound each body read so a stalled client can't hold a handler thread forever.
        self.connection.settimeout(BODY_READ_TIMEOUT_SEC)
        try:
            with open(tmp_path, "wb") as f:
                while bytes_remaining > 0:
                    chunk = self.rfile.read(min(READ_CHUNK_SIZE, bytes_remaining))
                    if not chunk:
                        raise ConnectionError("short read from client")
                    f.write(chunk)
                    bytes_remaining -= len(chunk)
                f.flush()
                try:
                    os.fsync(f.fileno())
                except OSError:
                    pass
            os.replace(tmp_path, dest_path)
        except (ConnectionError, OSError, socket.timeout) as e:
            logger.error(
                "upload failed dpu=%s file=%s client=%s err=%s",
                dpu_name,
                filename,
                self.client_address[0],
                e,
            )
            try:
                os.unlink(tmp_path)
            except OSError:
                pass
            self._reply(HTTPStatus.INTERNAL_SERVER_ERROR, f"write failed: {e}\n")
            return

        logger.info(
            "saved upload dpu=%s file=%s bytes=%d client=%s dest=%s",
            dpu_name,
            filename,
            content_length,
            self.client_address[0],
            dest_path,
        )
        self._reply(HTTPStatus.CREATED, f"ok: {dest_path}\n")

    # ---- HTTP method dispatch --------------------------------------------

    def do_POST(self) -> None:  # noqa: N802 - stdlib API
        self._save_upload()

    def do_PUT(self) -> None:  # noqa: N802 - stdlib API
        self._save_upload()

    def do_GET(self) -> None:  # noqa: N802 - stdlib API
        # Allow a trivial GET / for liveness checks. We don't expose listings.
        if self.path == "/" or self.path == "":
            self._reply(HTTPStatus.OK, "dump-receiver ok\n")
            return
        self._reply(HTTPStatus.NOT_FOUND, "not found\n")


class _ReuseAddrThreadingHTTPServer(ThreadingHTTPServer):
    """Bind to AF_INET with SO_REUSEADDR so quick restarts (test/CI, retries)
    don't fail with EADDRINUSE on the previous socket's TIME_WAIT.

    Avoids mutating ``ThreadingHTTPServer`` class attributes globally.
    """

    address_family = socket.AF_INET
    allow_reuse_address = True


def _build_server(
    *, bind_ip: str, port: int, output_dir: str, max_bytes: int
) -> ThreadingHTTPServer:
    """Construct the HTTP server. Output dir is created if missing."""
    os.makedirs(output_dir, mode=0o755, exist_ok=True)

    handler_cls = type(
        "DumpReceiverHandlerInstance",
        (_DumpReceiverHandler,),
        {"receiver_config": _DumpReceiverConfig(output_dir, max_bytes)},
    )
    return _ReuseAddrThreadingHTTPServer((bind_ip, port), handler_cls)


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="sonic-bfb-installer dump-receiver",
        description="HTTP receiver for DPU diagnostic dumps over tmfifo (ephemeral, used by sonic-bfb-installer).",
    )
    parser.add_argument("--bind-ip", default=DEFAULT_BIND_IP)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--output-dir", default=DEFAULT_OUTPUT_DIR)
    parser.add_argument(
        "--max-bytes",
        type=int,
        default=DEFAULT_MAX_BYTES,
        help="Per-upload size cap in bytes (default 256 MiB).",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable DEBUG-level logging.",
    )
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="dump-receiver: %(message)s",
        stream=sys.stderr,
    )

    try:
        server = _build_server(
            bind_ip=args.bind_ip,
            port=args.port,
            output_dir=args.output_dir,
            max_bytes=args.max_bytes,
        )
    except OSError as e:
        logger.error(
            "failed to bind %s:%d: %s", args.bind_ip, args.port, e
        )
        return 1

    logger.info(
        "listening on %s:%d, saving to %s (max %d bytes/upload)",
        args.bind_ip,
        args.port,
        args.output_dir,
        args.max_bytes,
    )

    stop_event = threading.Event()

    def _shutdown(_signum=None, _frame=None) -> None:
        if stop_event.is_set():
            return
        stop_event.set()
        # serve_forever() must be stopped from a thread other than the
        # request-handling thread.
        threading.Thread(target=server.shutdown, daemon=True).start()

    signal.signal(signal.SIGTERM, _shutdown)
    signal.signal(signal.SIGINT, _shutdown)
    try:
        signal.signal(signal.SIGHUP, _shutdown)
    except (AttributeError, ValueError):
        # Some environments (Windows, threaded reload) don't have SIGHUP.
        pass

    try:
        server.serve_forever(poll_interval=0.5)
    finally:
        server.server_close()
        logger.info("dump receiver stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
