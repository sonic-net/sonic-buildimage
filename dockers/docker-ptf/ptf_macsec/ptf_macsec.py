"""Native MACsec codec for PTF DataPlane.

Registers per-(device,port) tx/rx transforms on ptf.dataplane.DataPlane so
that plain send_packet()/verify_*() transparently encrypt on inject and
decrypt on capture for MACsec-enabled ports. Replaces the sonic-mgmt
ptftests/macsec.py monkeypatch: because the hook lives on the DataPlane
class (resolved at call time), the "import macsec before from-import
send_packet" ordering trap cannot occur.

Contract (unchanged from the monkeypatch era):
  - reads MACSEC_INFO_FILE (default /root/macsec_info.pickle), the pickle
    written per-run by sonic-mgmt create_macsec_info().
  - entry per DUT-facing port_number:
      (encrypt, send_sci, xpn_en, sci, an, sak, ssci, salt,
       peer_sci, peer_an, peer_ssci, pn)
  - PTF injects on peer AN (the +2 SA scheme lives on the DUT side and is
    reflected in the pickle). The TX PN is anchored at pickle_pn+1000 per port
    (the peer keeps advancing the SA, so the DUT's replay floor is already ahead
    of the pickled snapshot) and steps by PN_INCR per frame.

Activation: import this module and call install(dataplane) once the
DataPlane exists (see the DataPlane.__init__ patch, which auto-installs
when MACSEC_INFO_FILE is present).
"""
import os
import struct

MACSEC_INFO_FILE = os.environ.get("PTF_MACSEC_INFO", "/root/macsec_info.pickle")
# Opt-in switch: the codec stays dormant unless this marker exists, so a
# codec-carrying docker-ptf image changes nothing for existing runs (the
# sonic-mgmt ptftests/macsec.py monkeypatch keeps handling MACsec). sonic-mgmt
# writes the marker before launching PTF when native mode is requested
# (--macsec_ptf_native); the monkeypatch skips its install when it is present.
MACSEC_NATIVE_MODE_FLAG = "/root/macsec_native_codec"
# Rewriting the outer src MAC to the peer's MAC is required on Broadcom, where
# the ASIC derives the ingress SCI from {outer_src_MAC, port_id} rather than
# from the SecTAG's SCI field — the codec must send from the peer's identity,
# not PTF's own MAC, or the DUT computes the wrong SCI and drops the frame.
# On platforms that take the SCI from the SecTAG the rewrite can break SA
# lookup, so create_macsec_info() drops this opt-out marker on non-Broadcom
# DUTs. Default stays rewrite-on so pre-existing pickles keep working
# (matches ptftests/macsec.py's MACSEC_SMAC_NO_REWRITE_FLAG contract).
MACSEC_SMAC_NO_REWRITE_FLAG = "/root/macsec_no_rewrite_outer_smac"
_REWRITE_OUTER_SMAC = not os.path.exists(MACSEC_SMAC_NO_REWRITE_FLAG)
# Per-frame packet-number step. Replay protection only requires the PN to be
# monotonic, so 1 is sufficient and keeps the 32-bit PN space from being burned
# 100x faster than necessary under high-volume tests (CoPP sends unpaced for
# 30 s). The old ptftests/macsec.py monkeypatch used MACSEC_GLOBAL_PN_INCR = 100;
# override PTF_MACSEC_PN_INCR back to 100 on a testbed that still enforces replay
# windows and needs a larger step to stay ahead of the peer's own PN. Validated
# stable at both 1 and 100 via the CoPP suite (24/24 traffic tests pass).
PN_INCR = int(os.environ.get("PTF_MACSEC_PN_INCR", "1"))
ETH_P_MACSEC = 0x88E5
SCI_LEN = 8
ICV_LEN = 16

try:
    from cryptography.hazmat.primitives.ciphers.aead import AESGCM
    _CRYPTO = True
except ImportError:
    _CRYPTO = False

MACSEC_INFOS = {}
_TX_SA = {}          # port -> encrypt-side SA bundle (peer AN/SCI, tx PN)
_RX_SA = {}          # port -> decrypt-side SA bundle (own SCI/AN)
_TX_NEXT_PN = {}


# ---- SA construction (ported verbatim from ptftests/macsec.py fast path) ----
def _build_tx_sa(port):
    (encrypt, send_sci, xpn_en, _sci, _an, sak, _ssci, _salt,
     peer_sci, peer_an, peer_ssci, _pn) = MACSEC_INFOS[port]
    sak_b = sak if isinstance(sak, bytes) else bytes(sak)
    peer_sci_b = struct.pack('!Q', peer_sci) if isinstance(peer_sci, int) else peer_sci
    tci_base = ((1 << 5) if send_sci else 0) | ((1 << 3) if encrypt else 0) | ((1 << 2) if encrypt else 0)
    sa = {
        "encrypt": bool(encrypt), "send_sci": bool(send_sci), "xpn_en": bool(xpn_en),
        "peer_an": peer_an & 0x3, "peer_sci_bytes": peer_sci_b, "peer_mac_bytes": peer_sci_b[:6],
        "aesgcm": AESGCM(sak_b), "tci_base": tci_base,
    }
    if xpn_en:
        sa["ssci_bytes"] = struct.pack('!L', peer_ssci) if isinstance(peer_ssci, int) else peer_ssci
        sa["salt_bytes"] = _salt
    _TX_SA[port] = sa
    return sa


def _build_rx_sa(port):
    (encrypt, send_sci, xpn_en, sci, an, sak, ssci, salt,
     _peer_sci, _peer_an, _peer_ssci, _pn) = MACSEC_INFOS[port]
    sak_b = sak if isinstance(sak, bytes) else bytes(sak)
    sci_b = struct.pack('!Q', sci) if isinstance(sci, int) else sci
    sa = {"encrypt": bool(encrypt), "send_sci": bool(send_sci), "xpn_en": bool(xpn_en),
          "sci_bytes": sci_b, "aesgcm": AESGCM(sak_b)}
    if xpn_en:
        sa["ssci_bytes"] = struct.pack('!L', ssci) if isinstance(ssci, int) else ssci
        sa["salt_bytes"] = salt
    _RX_SA[port] = sa
    return sa


def _encap(sa, eth_bytes, send_pn):
    if _REWRITE_OUTER_SMAC:
        eth_bytes = eth_bytes[:6] + sa["peer_mac_bytes"] + eth_bytes[12:]
    outer_macs = eth_bytes[:12]
    inner = eth_bytes[12:]
    inner_type, inner_payload = inner[:2], inner[2:]
    tci_an = sa["tci_base"] | sa["peer_an"]
    data_after = 2 + len(inner_payload)
    sl = data_after if data_after < 48 else 0
    pn32 = send_pn & 0xFFFFFFFF
    sectag = struct.pack('!BBI', tci_an, sl, pn32)
    if sa["send_sci"]:
        sectag += sa["peer_sci_bytes"]
    if sa["xpn_en"]:
        tmp = sa["ssci_bytes"] + struct.pack('!Q', send_pn & 0xFFFFFFFFFFFFFFFF)
        iv = bytes(a ^ b for a, b in zip(tmp, sa["salt_bytes"]))
    else:
        iv = sa["peer_sci_bytes"] + struct.pack('!I', pn32)
    aad = outer_macs + struct.pack('!H', ETH_P_MACSEC) + sectag
    if sa["encrypt"]:
        return aad + sa["aesgcm"].encrypt(iv, inner_type + inner_payload, aad)
    ct = inner_type + inner_payload
    return aad + ct + sa["aesgcm"].encrypt(iv, b'', aad + ct)


def _decap(sa, wire):
    # wire: outer dst(6) src(6) 0x88e5(2) sectag(6[+8]) ...
    # Minimum MACsec frame: 14 (eth) + 6 (sectag no-SCI) = 20 bytes before body.
    if len(wire) < 20 or wire[12:14] != struct.pack('!H', ETH_P_MACSEC):
        return wire, False
    tci = wire[14]
    has_sci = bool(tci & (1 << 5))
    encrypt = bool(tci & (1 << 3))
    pn32 = struct.unpack('!I', wire[16:20])[0]
    off = 20 + (SCI_LEN if has_sci else 0)
    if len(wire) < off:
        return wire, False
    aad = wire[:off]
    body = wire[off:]
    if sa["xpn_en"]:
        tmp = sa["ssci_bytes"] + struct.pack('!Q', pn32)
        iv = bytes(a ^ b for a, b in zip(tmp, sa["salt_bytes"]))
    else:
        iv = sa["sci_bytes"] + struct.pack('!I', pn32)
    try:
        if encrypt:
            pt = sa["aesgcm"].decrypt(iv, body, aad)
            return wire[:12] + pt, True
        # integrity-only: last 16 bytes tag
        ct, tag = body[:-ICV_LEN], body[-ICV_LEN:]
        sa["aesgcm"].decrypt(iv, tag, aad + ct)
        return wire[:12] + ct, True
    except Exception:
        return wire, False


# ---- DataPlane transform callbacks ----
def _tx_transform(device, port, packet):
    if port not in MACSEC_INFOS or not MACSEC_INFOS[port]:
        return packet
    sa = _TX_SA.get(port) or _build_tx_sa(port)
    # The peer keeps sending on the same SA, so the DUT's replay floor is already
    # ahead of the pickled PN snapshot; start well clear of it.
    if port not in _TX_NEXT_PN:
        _TX_NEXT_PN[port] = MACSEC_INFOS[port][-1] + 1000
    eth = packet if isinstance(packet, bytes) else bytes(packet)
    pn = _TX_NEXT_PN[port]
    _TX_NEXT_PN[port] += PN_INCR
    return _encap(sa, eth, pn)


def _rx_transform(device, port, packet):
    if device != 0 or port not in MACSEC_INFOS or not MACSEC_INFOS[port]:
        return packet
    # Runt frames must pass through untouched: a per-packet exception here would
    # propagate into the dataplane receive loop (only install() is wrapped).
    if len(packet) < 14 or packet[12:14] != struct.pack('!H', ETH_P_MACSEC):
        return packet
    sa = _RX_SA.get(port) or _build_rx_sa(port)
    pt, ok = _decap(sa, packet if isinstance(packet, bytes) else bytes(packet))
    return pt if ok else packet   # pass undecryptable frames through (verify_no_packet semantics)


def load():
    global MACSEC_INFOS
    # Drop any cached SA/PN state from a previous load. Without this, a new
    # DataPlane constructed in the same process after key/AN rotation (or a
    # rewritten pickle) would keep serving the stale SAK/PN via the lazy
    # `_TX_SA.get(port) or _build_tx_sa(port)` lookup.
    _TX_SA.clear()
    _RX_SA.clear()
    _TX_NEXT_PN.clear()
    MACSEC_INFOS = {}
    if not os.path.exists(MACSEC_NATIVE_MODE_FLAG):
        return False
    if not _CRYPTO or not os.path.exists(MACSEC_INFO_FILE):
        return False
    import pickle
    with open(MACSEC_INFO_FILE, "rb") as f:
        # Trusted, locally-written test-infra data (ptf_runner writes this
        # file on the same host), not attacker-controlled input.
        MACSEC_INFOS = pickle.load(f, encoding="bytes")  # nosemgrep: python.lang.security.deserialization.pickle.avoid-pickle
    return bool(MACSEC_INFOS)


def install(dataplane):
    """Register tx/rx transforms on a DataPlane instance."""
    if not load():
        return False
    for port in MACSEC_INFOS:
        dataplane.register_port_transform(0, port, tx=_tx_transform, rx=_rx_transform)
    return True
