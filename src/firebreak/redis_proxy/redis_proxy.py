#!/usr/bin/env python3
"""Unix-socket Redis proxy: AUTH as a named ACL user, then splice RESP."""

from __future__ import annotations

import argparse
import logging
import os
import select
import socket
import sys
import syslog
import threading


def _resp_bulk(s: str) -> bytes:
    b = s.encode()
    return b"$%d\r\n" % len(b) + b + b"\r\n"


def auth_payload(user: str, secret: str) -> bytes:
    return b"*3\r\n$4\r\nAUTH\r\n" + _resp_bulk(user) + _resp_bulk(secret)


def read_one_resp(sock: socket.socket, timeout: float = 5.0) -> bytes:
    sock.settimeout(timeout)
    buf = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("upstream closed during AUTH")
        buf += chunk
        if b"\r\n" in buf:
            return buf


def splice(a: socket.socket, b: socket.socket) -> None:
    a.settimeout(None)
    b.settimeout(None)
    pair = {a: b, b: a}
    socks = [a, b]
    try:
        while True:
            r, _, x = select.select(socks, [], socks, 60)
            if x:
                break
            if not r:
                continue
            for src in r:
                data = src.recv(65536)
                if not data:
                    return
                dst = pair[src]
                dst.sendall(data)
                if b"NOPERM" in data:
                    syslog.syslog(
                        syslog.LOG_WARNING,
                        "FIREBREAK deny db NOPERM forwarded on redis proxy",
                    )
    except OSError:
        return


def handle_client(client: socket.socket, upstream_path: str, user: str, secret: str) -> None:
    up = None
    try:
        up = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        up.connect(upstream_path)
        up.sendall(auth_payload(user, secret))
        reply = read_one_resp(up)
        if not reply.startswith(b"+OK") and b"-ERR" in reply:
            syslog.syslog(syslog.LOG_ERR, "FIREBREAK redis AUTH failed: %r" % reply[:80])
            client.sendall(b"-ERR firebreak AUTH failed\r\n")
            return
        leftover = reply
        # AUTH response is typically +OK\r\n only; if extra RESP arrived, send it on.
        if leftover not in (b"+OK\r\n",) and leftover.startswith(b"+OK\r\n") and len(leftover) > 5:
            client.sendall(leftover[5:])
        splice(client, up)
    except Exception as exc:
        syslog.syslog(syslog.LOG_ERR, "FIREBREAK redis proxy client error: %s" % exc)
    finally:
        try:
            client.close()
        except OSError:
            pass
        if up is not None:
            try:
                up.close()
            except OSError:
                pass


def serve(listen: str, upstream: str, user: str, secret: str,
          tcp_addr: str = "", tcp_port: int = 0) -> None:
    if os.path.exists(listen):
        os.unlink(listen)
    os.makedirs(os.path.dirname(listen), exist_ok=True)
    srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    srv.bind(listen)
    os.chmod(listen, 0o777)
    srv.listen(128)
    syslog.syslog(syslog.LOG_INFO, "FIREBREAK redis-proxy listen=%s user=%s" % (listen, user))

    tcp_srv = None
    if tcp_port:
        tcp_srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp_srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        tcp_srv.bind((tcp_addr or "127.0.0.1", tcp_port))
        tcp_srv.listen(128)
        syslog.syslog(
            syslog.LOG_INFO,
            "FIREBREAK redis-proxy tcp=%s:%d user=%s" % (tcp_addr or "127.0.0.1", tcp_port, user),
        )

    def _accept_loop(server: socket.socket) -> None:
        while True:
            client, _ = server.accept()
            t = threading.Thread(
                target=handle_client, args=(client, upstream, user, secret), daemon=True
            )
            t.start()

    if tcp_srv:
        t = threading.Thread(target=_accept_loop, args=(tcp_srv,), daemon=True)
        t.start()
    _accept_loop(srv)


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--listen", required=True, help="Unix socket path")
    p.add_argument("--upstream", required=True, help="Upstream Redis Unix socket")
    p.add_argument("--user", required=True, help="Redis ACL user")
    p.add_argument("--secret-file", required=True, help="Path to secret file")
    p.add_argument("--tcp-port", type=int, default=0,
                   help="Also listen on 127.0.0.1:<port> for TCP redirect")
    args = p.parse_args()
    logging.basicConfig(level=logging.INFO)
    with open(args.secret_file, "r", encoding="utf-8") as f:
        secret = f.read().strip()
    serve(args.listen, args.upstream, args.user, secret,
          tcp_port=args.tcp_port)
    return 0


if __name__ == "__main__":
    sys.exit(main())
