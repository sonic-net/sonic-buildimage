# SPDX-License-Identifier: GPL-2.0-or-later
"""Run with python3 -m unittest discover -s src/sonic-frr/dplane_fpm_sonic/tests.

Compiles the production adapter against getter-backed context doubles. Exact
wire expectations are independent Python vectors; no live FRR/DVS claim.
"""
import ctypes as C
import os
from pathlib import Path
import socket
import struct
import subprocess
import tempfile
import unittest


class AddressUnion(C.Union):
    _fields_ = [('raw', C.c_uint8 * 16), ('alignment', C.c_uint32)]


class Address(C.Structure):
    _fields_ = [('type', C.c_int), ('value', AddressUnion)]


class Member(C.Structure):
    _fields_ = [('id', C.c_uint32), ('type', C.c_uint8),
                ('vtep', Address), ('mac', C.c_uint8 * 6)]


class Context(C.Structure):
    _fields_ = [('op', C.c_int), ('deleting', C.c_bool), ('id', C.c_uint32),
                ('type', C.c_uint8), ('vtep', Address), ('mac', C.c_uint8 * 6),
                ('count', C.c_uint32), ('members', C.POINTER(Member)),
                ('esi', C.c_uint8 * 10), ('nulls', C.c_uint)]


def address(text='10.0.0.2'):
    family = socket.AF_INET6 if ':' in text else socket.AF_INET
    result = Address(type=family)
    data = socket.inet_pton(family, text)
    result.value.raw[:len(data)] = data
    return result


def member(identifier=0x40000001, ip='10.0.0.2'):
    return Member(identifier, 1, address(ip), (C.c_uint8 * 6)(2, 0, 0, 0, 0, 2))


def member_wire(identifier=0x40000001, ip='10.0.0.2'):
    family = socket.AF_INET6 if ':' in ip else socket.AF_INET
    return struct.pack('=IBB2x16s6s2x', identifier, 1, family,
                       socket.inet_pton(family, ip), bytes([2, 0, 0, 0, 0, 2]))


def netlink(kind, payload=b''):
    return struct.pack('=IHHII', 16 + len(payload), kind, 0x401, 0, 0) + payload


class ProtectionEncoderTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = None if os.environ.get("BPP_TEST_BUILD_DIR") else tempfile.TemporaryDirectory()
        build = Path(os.environ.get("BPP_TEST_BUILD_DIR") or cls.temp.name)
        build.mkdir(parents=True, exist_ok=True)
        source = Path(__file__).with_name('bpp_fixture.c')
        library = build / 'bpp.so'
        subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', '-Wall',
                        '-Wextra', '-Werror', '-fPIC', '-shared',
                        *os.environ.get('BPP_TEST_CFLAGS', '').split(),
                        str(source), '-o', str(library)], check=True)
        cls.library = C.CDLL(str(library))
        cls.library.encode.argtypes = [C.POINTER(Context), C.c_void_p, C.c_size_t]
        cls.library.encode.restype = C.c_ssize_t
        cls.library.binding.argtypes = [C.c_uint32, C.c_uint32, C.c_bool, C.c_void_p, C.c_size_t]
        cls.library.binding.restype = C.c_ssize_t

    @classmethod
    def tearDownClass(cls):
        if cls.temp: cls.temp.cleanup()

    def context(self, op=1):
        c = Context(op=op, id=0x40000001 if op == 1 else 0x80000001,
                    type=1, vtep=address(), mac=(C.c_uint8 * 6)(2, 0, 0, 0, 0, 2))
        c.esi[:] = range(10)
        return c

    def encode(self, c, capacity=16384):
        # A canary proves rejected sizes never write beyond the caller's buffer.
        buf = (C.c_uint8 * 16400)(*([0xa5] * 16400))
        size = self.library.encode(C.byref(c), buf, capacity)
        self.assertEqual(bytes(buf)[capacity:], b'\xa5' * (16400-capacity))
        return size, bytes(buf)[:max(0, size)]

    def test_wire_header_builds_without_companion_frr_api(self):
        header = Path(__file__).resolve().parents[1] / 'bridge_port_protection.h'
        source = ('#define RTM_FPM_ADD_EVPN_ES_BACKUP_NHG 147\n'
                  '#define RTM_FPM_DEL_EVPN_ES_BACKUP_NHG 148\n'
                  '#include "' + str(header) + '"\n'
                  'int main(void) { return 0; }\n')
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall',
                        '-Wextra', '-Werror', '-fsyntax-only', '-x', 'c', '-'],
                       input=source, text=True, check=True)

    def test_member_ipv4_ipv6_golden_bytes(self):
        for ip in ['10.0.0.2', '2001:db8::2']:
            with self.subTest(ip=ip):
                c = self.context(); c.vtep = address(ip)
                self.assertEqual(self.encode(c), (48, netlink(149, member_wire(ip=ip))))

    def test_member_unsigned_boundary_ids(self):
        for identifier in [0x40000000, 0x40000001, 0x4fffffff]:
            with self.subTest(identifier=identifier):
                c = self.context(); c.id = identifier
                self.assertEqual(self.encode(c)[1], netlink(149, member_wire(identifier)))

    def test_member_delete_needs_only_identity(self):
        c = self.context(); c.deleting = True; c.nulls = 3; c.type = 0
        self.assertEqual(self.encode(c)[1], netlink(150, struct.pack('=I28x', c.id)))

    def test_member_rejects_other_producer_ids(self):
        for identifier in [0, 1, 0x10000001, 0x20000001, 0x80000001, 0xffffffff]:
            for deleting in [False, True]:
                with self.subTest(id=identifier, deleting=deleting):
                    c = self.context(); c.id = identifier; c.deleting = deleting
                    self.assertEqual(self.encode(c)[0], -1)

    def test_member_rejects_non_l3_type(self):
        for kind in [0, 2, 255]:
            c = self.context(); c.type = kind
            self.assertEqual(self.encode(c)[0], -1)

    def test_member_rejects_unknown_family(self):
        c = self.context(); c.vtep.type = socket.AF_UNSPEC
        self.assertEqual(self.encode(c)[0], -1)

    def test_member_rejects_absent_address_or_mac(self):
        for nulls in [1, 2, 3]:
            c = self.context(); c.nulls = nulls
            self.assertEqual(self.encode(c)[0], -1)

    def test_member_rejects_zero_multicast_and_broadcast_mac(self):
        for mac in [bytes(6), bytes([1,0,0,0,0,1]), b'\xff'*6]:
            c = self.context(); c.mac[:] = mac
            self.assertEqual(self.encode(c)[0], -1)

    def test_member_exact_and_short_capacity(self):
        c = self.context()
        self.assertEqual(self.encode(c, 48)[0], 48)
        for capacity in [0, 16, 47]:
            self.assertEqual(self.encode(c, capacity)[0], -1)

    def test_group_complete_records_esi_and_numeric_order(self):
        c = self.context(2)
        members = (Member*3)(member(0x40000003), member(0x40000001, '2001:db8::2'), member(0x40000002))
        c.count = 3; c.members = members
        payload = struct.pack('=II10s2x', c.id, 3, bytes(range(10)))
        payload += member_wire(0x40000001, '2001:db8::2') + member_wire(0x40000002) + member_wire(0x40000003)
        self.assertEqual(self.encode(c), (132, netlink(151, payload)))
        self.assertEqual([m.id for m in members], [0x40000003, 0x40000001, 0x40000002])

    def test_empty_group_is_complete_object(self):
        c = self.context(2)
        self.assertEqual(self.encode(c)[1], netlink(151, struct.pack('=II10s2x', c.id, 0, bytes(range(10)))))

    def test_group_maximum_fits_fpm_frame(self):
        c = self.context(2); c.count = 510
        members = (Member*510)(*(member(0x40000000+i) for i in range(510)))
        c.members = members
        size, data = self.encode(c)
        self.assertEqual(size + 4, 16360)
        self.assertEqual(struct.unpack_from('=I', data, 20)[0], 510)

    def test_group_rejects_excessive_or_overflow_count(self):
        for count in [511, 4096, 0xffffffff]:
            c = self.context(2); c.count = count
            self.assertEqual(self.encode(c)[0], -1)

    def test_group_rejects_missing_members_or_esi(self):
        for count, nulls in [(1,0), (0,4), (1,4)]:
            c = self.context(2); c.count = count; c.nulls = nulls
            self.assertEqual(self.encode(c)[0], -1)

    def test_group_rejects_other_producer_ids(self):
        for identifier in [0, 0x10000001, 0x20000001, 0x40000001, 0xffffffff]:
            c = self.context(2); c.id = identifier
            self.assertEqual(self.encode(c)[0], -1)

    def test_group_delete_ignores_retired_member_data(self):
        c = self.context(2); c.deleting = True; c.count = 0xffffffff; c.nulls = 4
        self.assertEqual(self.encode(c)[1], netlink(152, struct.pack('=II12x', c.id, 0)))

    def test_group_rejects_duplicate_member_ids(self):
        c = self.context(2); c.count = 2
        members = (Member*2)(member(), member(ip='2001:db8::2')); c.members = members
        self.assertEqual(self.encode(c)[0], -1)

    def test_group_rejects_invalid_inline_member(self):
        c = self.context(2); c.count = 1
        for field in ['id', 'type', 'family', 'mac']:
            with self.subTest(field=field):
                m = member()
                if field == 'id': m.id = 0x10000001
                elif field == 'type': m.type = 0
                elif field == 'family': m.vtep.type = 0
                else: m.mac[:] = bytes(6)
                members = (Member*1)(m); c.members = members
                self.assertEqual(self.encode(c)[0], -1)

    def test_group_exact_and_short_capacity(self):
        c = self.context(2); c.count = 1
        members = (Member*1)(member()); c.members = members
        self.assertEqual(self.encode(c, 68)[0], 68)
        for capacity in [0, 16, 67]: self.assertEqual(self.encode(c, capacity)[0], -1)

    def test_replay_done_has_no_payload(self):
        self.assertEqual(self.encode(self.context(3)), (16, netlink(153)))

    def test_replay_done_exact_and_short_capacity(self):
        self.assertEqual(self.encode(self.context(3), 16)[0], 16)
        for capacity in [0, 15]: self.assertEqual(self.encode(self.context(3), capacity)[0], -1)

    def binding(self, index=7, group=0x80000001, deleting=False, capacity=24):
        buf = (C.c_uint8 * 32)(*([0xa5]*32))
        size = self.library.binding(index, group, deleting, buf, capacity)
        self.assertEqual(bytes(buf)[capacity:], b'\xa5' * (32-capacity))
        return size, bytes(buf)[:max(0, size)]

    def test_binding_preserves_unsigned_and_legacy_ids(self):
        for identifier in [0x20000001, 0x80000000, 0x80000001, 0x8fffffff]:
            self.assertEqual(self.binding(group=identifier),
                             (24, netlink(147, struct.pack('=iI', 7, identifier))))

    def test_binding_delete_clears_even_stale_nonzero_group(self):
        self.assertEqual(self.binding(deleting=True)[1], netlink(148, struct.pack('=iI', 7, 0)))
        self.assertEqual(self.binding(group=0)[1], netlink(148, struct.pack('=iI', 7, 0)))

    def test_binding_rejects_invalid_index(self):
        for index in [0, 0x80000000, 0xffffffff]:
            self.assertEqual(self.binding(index=index)[0], -1)
        self.assertEqual(self.binding(index=0x7fffffff)[0], 24)

    def test_binding_rejects_short_buffer(self):
        for capacity in [0, 16, 23]:
            self.assertEqual(self.binding(capacity=capacity)[0], -1)

    def test_unknown_operation_is_rejected(self):
        self.assertEqual(self.encode(self.context(99))[0], -1)


if __name__ == '__main__':
    unittest.main()
