#!/usr/bin/env python3
import os
import sys
import ptpmgmt

CFG = "/etc/ptp4l/ptp4l.conf"
class Disp(ptpmgmt.MessageDispatcher):
    def USER_DESCRIPTION_h(self, msg, tlv, tlv_id):
        print("user description:", tlv.userDescription.textField)

def main():
    cfg = ptpmgmt.ConfigFile()
    if not cfg.read_cfg(CFG):
        raise SystemExit("failed to read config")

    sock = ptpmgmt.SockUnix()
    buf = ptpmgmt.Buf(1000)
    msg = ptpmgmt.Message()

    disp = Disp()
    if not sock.setDefSelfAddress() or not sock.init():
        raise SystemExit("socket init failed")
    if not sock.setPeerAddress(cfg):
        raise SystemExit("failed to set peer address")

    prms = msg.getParams()
    prms.self_id.portNumber = os.getpid() & 0xffff
    prms.domainNumber = cfg.domainNumber()
    msg.updateParams(prms)
    msg.useConfig(cfg)
    # GET USER_DESCRIPTION
    tlv_id = ptpmgmt.USER_DESCRIPTION
    msg.setAction(ptpmgmt.GET, tlv_id)
    seq = 1
    if msg.build(buf, seq) != ptpmgmt.MNG_PARSE_ERROR_OK:
        raise SystemExit("build failed")
    if not sock.send(buf, msg.getMsgLen()):
        raise SystemExit("send failed")
    if not sock.poll(500):
        raise SystemExit("timeout")
    cnt = sock.rcv(buf)
    if cnt <= 0:
        raise SystemExit("receive failed")
    err = msg.parse(buf, cnt)
    if err != ptpmgmt.MNG_PARSE_ERROR_OK:
        raise SystemExit(ptpmgmt.Message.err2str_c(err))
    disp.callHadler(msg, msg.getTlvId(), msg.getData())
    sock.close()

if __name__ == "__main__":
    main()