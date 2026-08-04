from swsscommon import swsscommon
import time
import re
import json

def test_PortChannel(dvs, testlog):
    appldb = swsscommon.DBConnector(0, dvs.redis_sock, 0)
    statdb = swsscommon.DBConnector(6, dvs.redis_sock, 0)

    # create the lag
    dvs.runcmd("config portchannel add PortChannel0001")
    time.sleep(1)

    # test lag table in appl db
    tbl = swsscommon.Table(appldb, "LAG_TABLE")
    (status, fvs) = tbl.get("PortChannel0001")
    assert status == True

    # test lag table in state db
    tbl = swsscommon.Table(statdb, "LAG_TABLE")
    (status, fvs) = tbl.get("PortChannel0001")
    assert status == True

    # create the lag member
    dvs.runcmd("config portchannel member add PortChannel0001 Ethernet112")
    time.sleep(1)

    # test lag member table in appl db
    tbl = swsscommon.Table(appldb, "LAG_MEMBER_TABLE")
    (status, fvs) = tbl.get("PortChannel0001:Ethernet112")
    assert status == True

    # remove the lag member
    dvs.runcmd("config portchannel member del PortChannel0001 Ethernet112")

    # remove the lag
    dvs.runcmd("config portchannel del PortChannel0001")
