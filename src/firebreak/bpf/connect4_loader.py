#!/usr/bin/env python3
"""Load a cgroup/connect4 allow-list (deny-by-default) using the bpf() syscall.

Returns a kernel-observed attachment identity. Never silently substitutes a
different policy backend when BPF installation fails.
"""

from __future__ import annotations

import argparse
import array
import ctypes
import ctypes.util
import fcntl
import ipaddress
import json
import os
import socket
import struct
import subprocess
import syslog


libc = ctypes.CDLL(ctypes.util.find_library("c"), use_errno=True)
libc.syscall.restype = ctypes.c_long

SYS_BPF = 321  # x86_64
BPF_MAP_CREATE = 0
BPF_MAP_UPDATE_ELEM = 2
BPF_PROG_LOAD = 5
BPF_PROG_ATTACH = 8
BPF_MAP_TYPE_HASH = 1
BPF_PROG_TYPE_CGROUP_SOCK_ADDR = 18
BPF_CGROUP_INET4_CONNECT = 10  # linux/bpf.h, kernel 5.x+
BPF_ANY = 0
BPF_F_ALLOW_OVERRIDE = 1
BPF_F_ALLOW_MULTI = 2
BPF_PROG_DETACH = 9
BPF_PROG_QUERY = 16
BPF_OBJ_GET_INFO_BY_FD = 15
BPF_PSEUDO_MAP_FD = 1

BPF_LD = 0x00
BPF_LDX = 0x01
BPF_STX = 0x03
BPF_ALU64 = 0x07
BPF_JMP = 0x05
BPF_MOV = 0xb0
BPF_ADD = 0x00
BPF_AND = 0x50
BPF_JEQ = 0x10
BPF_CALL = 0x80
BPF_EXIT = 0x90
BPF_IMM = 0x00
BPF_MEM = 0x60
BPF_DW = 0x18
BPF_W = 0x00
BPF_K = 0x00
BPF_X = 0x08
BPF_FUNC_map_lookup_elem = 1

# bpf_sock_addr field offsets (bytes)
OFF_USER_IP4 = 4     # __u32 user_ip4  (network order)
OFF_USER_PORT = 24   # __u32 user_port (network order, low 16 bits)


def insn(code, dst, src, off, imm):
    return struct.pack("<BBhi", code, dst | (src << 4), off, imm)


def ld_map_fd(dst_reg, map_fd):
    return insn(BPF_LD | BPF_DW | BPF_IMM, dst_reg, BPF_PSEUDO_MAP_FD, 0, map_fd) + insn(
        0, 0, 0, 0, 0
    )


class AttrMapCreate(ctypes.Structure):
    _fields_ = [
        ("map_type", ctypes.c_uint32),
        ("key_size", ctypes.c_uint32),
        ("value_size", ctypes.c_uint32),
        ("max_entries", ctypes.c_uint32),
        ("map_flags", ctypes.c_uint32),
        ("inner_map_fd", ctypes.c_uint32),
        ("numa_node", ctypes.c_uint32),
        ("map_name", ctypes.c_char * 16),
    ]


class AttrMapUpdate(ctypes.Structure):
    _fields_ = [
        ("map_fd", ctypes.c_uint32),
        ("_pad", ctypes.c_uint32),
        ("key", ctypes.c_uint64),
        ("value", ctypes.c_uint64),
        ("flags", ctypes.c_uint64),
    ]


class AttrProgLoad(ctypes.Structure):
    _fields_ = [
        ("prog_type", ctypes.c_uint32),
        ("insn_cnt", ctypes.c_uint32),
        ("insns", ctypes.c_uint64),
        ("license", ctypes.c_uint64),
        ("log_level", ctypes.c_uint32),
        ("log_size", ctypes.c_uint32),
        ("log_buf", ctypes.c_uint64),
        ("kern_version", ctypes.c_uint32),
        ("prog_flags", ctypes.c_uint32),
        ("prog_name", ctypes.c_char * 16),
        ("prog_ifindex", ctypes.c_uint32),
        ("expected_attach_type", ctypes.c_uint32),
    ]


class AttrProgAttach(ctypes.Structure):
    _fields_ = [
        ("target_fd", ctypes.c_uint32),
        ("attach_bpf_fd", ctypes.c_uint32),
        ("attach_type", ctypes.c_uint32),
        ("attach_flags", ctypes.c_uint32),
    ]


class AttrProgQuery(ctypes.Structure):
    _fields_ = [('target_fd', ctypes.c_uint32), ('attach_type', ctypes.c_uint32),
                ('query_flags', ctypes.c_uint32), ('attach_flags', ctypes.c_uint32),
                ('prog_ids', ctypes.c_uint64), ('prog_cnt', ctypes.c_uint32)]


class AttrInfo(ctypes.Structure):
    _fields_ = [('bpf_fd', ctypes.c_uint32), ('info_len', ctypes.c_uint32),
                ('info', ctypes.c_uint64)]


def program_id(fd):
    info = (ctypes.c_uint32 * 2)()  # bpf_prog_info starts with type and id.
    attr = AttrInfo(fd, ctypes.sizeof(info), ctypes.addressof(info))
    bpf(BPF_OBJ_GET_INFO_BY_FD, attr)
    return info[1]


def inspect(cgroup_path):
    cgfd = os.open(cgroup_path, os.O_RDONLY | os.O_DIRECTORY)
    try:
        ids = (ctypes.c_uint32 * 64)()
        attr = AttrProgQuery(cgfd, BPF_CGROUP_INET4_CONNECT, 0, 0,
                             ctypes.addressof(ids), len(ids))
        bpf(BPF_PROG_QUERY, attr)
        return {'mode': 'bpf', 'cgroup': cgroup_path,
                'cgroup_id': os.fstat(cgfd).st_ino,
                'program_ids': list(ids[:attr.prog_cnt])}
    finally:
        os.close(cgfd)


def bpf(cmd, attr, size=None):
    if size is None:
        size = ctypes.sizeof(attr)
    ret = libc.syscall(SYS_BPF, cmd, ctypes.byref(attr), size)
    if ret < 0:
        err = ctypes.get_errno()
        raise OSError(err, os.strerror(err))
    return ret


def create_allow_map():
    attr = AttrMapCreate()
    attr.map_type = BPF_MAP_TYPE_HASH
    attr.key_size = 4
    attr.value_size = 4
    attr.max_entries = 256
    attr.map_name = b"fb_allow4"
    return bpf(BPF_MAP_CREATE, attr)


def map_update(map_fd, ip_u32):
    key = ctypes.c_uint32(ip_u32)
    val = ctypes.c_uint32(1)
    attr = AttrMapUpdate()
    attr.map_fd = map_fd
    attr.key = ctypes.cast(ctypes.pointer(key), ctypes.c_void_p).value
    attr.value = ctypes.cast(ctypes.pointer(val), ctypes.c_void_p).value
    attr.flags = BPF_ANY
    bpf(BPF_MAP_UPDATE_ELEM, attr)


def build_prog(map_fd: int, *, redirect_port: int = 0) -> bytes:
    """Build eBPF bytecode for cgroup/connect4.

    The program:
    1. Loads user_ip4, looks it up in the allow map.
    2. If the IP is NOT allowed → return 0 (EPERM).
    3. If the IP IS allowed AND redirect_port is set:
       a. Load user_port (network order).
       b. If port == htons(6379): rewrite user_port to htons(redirect_port).
    4. Return 1 (allow, possibly with rewritten port).
    """
    BPF_JNE = 0x50
    BPF_ST = 0x02  # *(size *)(dst + off) = imm

    parts = []

    # --- Phase 1: IP allow-list check ---
    # r2 = ctx->user_ip4  (offset 4)
    parts.append(insn(BPF_LDX | BPF_MEM | BPF_W, 2, 1, OFF_USER_IP4, 0))
    # Save ctx pointer in r6 for later use
    parts.append(insn(BPF_ALU64 | BPF_MOV | BPF_X, 6, 1, 0, 0))
    # *(u32 *)(r10 - 4) = r2  (stack key for map lookup)
    parts.append(insn(BPF_STX | BPF_MEM | BPF_W, 10, 2, -4, 0))
    # r1 = map_fd
    parts.append(ld_map_fd(1, map_fd))
    # r2 = r10 - 4  (pointer to stack key)
    parts.append(insn(BPF_ALU64 | BPF_MOV | BPF_X, 2, 10, 0, 0))
    parts.append(insn(BPF_ALU64 | BPF_ADD | BPF_K, 2, 0, 0, -4))
    # call bpf_map_lookup_elem
    parts.append(insn(BPF_JMP | BPF_CALL, 0, 0, 0, BPF_FUNC_map_lookup_elem))

    if redirect_port:
        # r0 == 0 → IP not in map → deny (jump to deny block)
        # We need to know how many instructions are in the redirect block to jump over them.
        # Redirect block: 3 insns (load port, compare, rewrite) + allow exit
        # deny: 2 insns (mov 0, exit)
        # allow: 2 insns (mov 1, exit)
        # Layout after lookup:
        #   JEQ r0,0 → deny  (skip redirect + allow)
        #   load port
        #   JNE port,htons(6379) → allow (skip rewrite)
        #   rewrite port
        #   allow: mov r0,1; exit
        #   deny:  mov r0,0; exit
        # user_port is a __u32 in network byte order with the port in the
        # low 16 bits (like sin_port zero-extended).  A BPF LDX W reads it
        # as a little-endian u32, so htons(6379) = 0xEB18 appears as
        # 0x0000EB18 on LE.  The STX W / ST W must write the same layout.
        redis_port_ne = socket.htons(6379)       # 0xEB18 on LE
        proxy_port_ne = socket.htons(redirect_port)

        # if r0 == 0: jump +5 → deny block
        parts.append(insn(BPF_JMP | BPF_JEQ | BPF_K, 0, 0, 5, 0))
        # r7 = *(u32 *)(r6 + 24)  ; user_port (network order)
        parts.append(insn(BPF_LDX | BPF_MEM | BPF_W, 7, 6, OFF_USER_PORT, 0))
        # if r7 != htons(6379): jump +1 → allow (skip rewrite)
        parts.append(insn(BPF_JMP | BPF_JNE | BPF_K, 7, 0, 1, redis_port_ne))
        # *(u32 *)(r6 + 24) = htons(redirect_port)  ; rewrite port
        parts.append(insn(BPF_ST | BPF_MEM | BPF_W, 6, 0, OFF_USER_PORT, proxy_port_ne))
        # allow: r0 = 1; exit
        parts.append(insn(BPF_ALU64 | BPF_MOV | BPF_K, 0, 0, 0, 1))
        parts.append(insn(BPF_JMP | BPF_EXIT, 0, 0, 0, 0))
        # deny: r0 = 0; exit
        parts.append(insn(BPF_ALU64 | BPF_MOV | BPF_K, 0, 0, 0, 0))
        parts.append(insn(BPF_JMP | BPF_EXIT, 0, 0, 0, 0))
    else:
        # Simple allow/deny based on IP only (original behavior)
        # if r0 == 0: jump +2 → deny
        parts.append(insn(BPF_JMP | BPF_JEQ | BPF_K, 0, 0, 2, 0))
        # allow: r0 = 1; exit
        parts.append(insn(BPF_ALU64 | BPF_MOV | BPF_K, 0, 0, 0, 1))
        parts.append(insn(BPF_JMP | BPF_EXIT, 0, 0, 0, 0))
        # deny: r0 = 0; exit
        parts.append(insn(BPF_ALU64 | BPF_MOV | BPF_K, 0, 0, 0, 0))
        parts.append(insn(BPF_JMP | BPF_EXIT, 0, 0, 0, 0))

    return b"".join(parts)


def load_prog(insns: bytes) -> int:
    license = b"GPL\x00"
    log = ctypes.create_string_buffer(65536)
    insn_buf = ctypes.create_string_buffer(insns)
    attr = AttrProgLoad()
    attr.prog_type = BPF_PROG_TYPE_CGROUP_SOCK_ADDR
    attr.insn_cnt = len(insns) // 8
    attr.insns = ctypes.addressof(insn_buf)
    license_buf = ctypes.create_string_buffer(license)
    attr.license = ctypes.addressof(license_buf)
    attr.log_level = 1
    attr.log_size = 65536
    attr.log_buf = ctypes.addressof(log)
    attr.prog_name = b"fb_conn4"
    attr.expected_attach_type = BPF_CGROUP_INET4_CONNECT
    try:
        return bpf(BPF_PROG_LOAD, attr)
    except OSError as exc:
        syslog.syslog(syslog.LOG_ERR, "FIREBREAK bpf load log: %s" % log.value.decode(errors="replace")[:800])
        raise exc


def attach_prog(prog_fd: int, cgroup_path: str) -> None:
    """Replace any prior connect4 program on this cgroup (do not stack MULTI).

    Stacking BPF_F_ALLOW_MULTI left old deny-all programs attached, so new
    allow-lists never took effect (any program returning 0 denies connect).
    """
    cgfd = os.open(cgroup_path, os.O_RDONLY | os.O_DIRECTORY)
    try:
        attr = AttrProgAttach()
        attr.target_fd = cgfd
        attr.attach_bpf_fd = prog_fd
        attr.attach_type = BPF_CGROUP_INET4_CONNECT
        # OVERRIDE replaces an existing program at this attach point
        attr.attach_flags = BPF_F_ALLOW_OVERRIDE
        bpf(BPF_PROG_ATTACH, attr)
    finally:
        os.close(cgfd)


def ipv4_to_net_u32(ip: str) -> int:
    packed = socket.inet_aton(ip)
    return struct.unpack("!I", packed)[0]


def attach(cgroup_path: str, allow_ips: list[str], *, redirect_port: int = 0) -> dict:
    map_fd = prog_fd = None
    try:
        map_fd = create_allow_map()
        for ip in allow_ips:
            # user_ip4 is network-order bytes loaded as LE u32
            map_update(map_fd, struct.unpack("<I", socket.inet_aton(ip))[0])
        insns = build_prog(map_fd, redirect_port=redirect_port)
        prog_fd = load_prog(insns)
        expected_id = program_id(prog_fd)
        attach_prog(prog_fd, cgroup_path)
        observed = inspect(cgroup_path)
        if observed['program_ids'] != [expected_id]:
            raise RuntimeError('BPF attachment did not match installed program')
        syslog.syslog(syslog.LOG_INFO,
                       "FIREBREAK bpf connect4 attached at %s allow=%s redirect=%s"
                       % (cgroup_path, allow_ips, redirect_port or "none"))
        return observed
    finally:
        for fd in (prog_fd, map_fd):
            if fd is not None:
                os.close(fd)


def docker_cgroup(container: str) -> str:
    out = subprocess.check_output(
        ["docker", "inspect", "--format", "{{.Id}}", container], text=True
    ).strip()
    path = "/sys/fs/cgroup/system.slice/docker-%s.scope" % out
    if not os.path.isdir(path):
        raise FileNotFoundError(path)
    return path


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--container", required=True)
    p.add_argument("--allow-ip", action="append", default=[])
    p.add_argument("--redis-redirect-port", type=int, default=0,
                   help="Redirect 127.0.0.1:6379 to this port (proxy TCP listener)")
    p.add_argument("--inspect", action="store_true", help="Read actual attached program IDs without changing policy")
    args = p.parse_args()
    cg = docker_cgroup(args.container)
    if args.inspect:
        result = inspect(cg)
    else:
        result = attach(cg, args.allow_ip, redirect_port=args.redis_redirect_port)
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
