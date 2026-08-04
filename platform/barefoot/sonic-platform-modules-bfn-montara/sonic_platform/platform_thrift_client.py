#!/usr/bin/env python

try:
    import time

    from sonic_platform.pltfm_mgr_rpc.pltfm_mgr_rpc import Client
    from sonic_platform.pltfm_mgr_rpc.pltfm_mgr_rpc import InvalidPltfmMgrOperation

    from thrift.transport import TSocket
    from thrift.transport import TTransport
    from thrift.protocol import TBinaryProtocol
    from thrift.protocol import TMultiplexedProtocol
    from thrift.Thrift import TException
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

THRIFT_SERVER = 'localhost'

class ThriftClient(object):
    def open(self):
        self.transport = TSocket.TSocket(THRIFT_SERVER, 9090)

        self.transport = TTransport.TBufferedTransport(self.transport)
        bprotocol = TBinaryProtocol.TBinaryProtocol(self.transport)

        pltfm_mgr_protocol = TMultiplexedProtocol.TMultiplexedProtocol(bprotocol, "pltfm_mgr_rpc")
        self.pltfm_mgr = Client(pltfm_mgr_protocol)

        self.transport.open()
        return self
    def close(self):
        self.transport.close()
    def __enter__(self):
        return self.open()
    def __exit__(self, exc_type, exc_value, tb):
        self.close()

def pltfm_mgr_ready():
    try:
        with ThriftClient():
            return True
    except Exception:
        return False

def thrift_try(func, attempts=35):
    for attempt in range(attempts):
        try:
            with ThriftClient() as client:
               return func(client)
        except TException as e:
            if attempt + 1 == attempts:
               raise e
        time.sleep(1)

def pltfm_mgr_try(func, default=None, thrift_attempts=35):
    def pm_cb_run(client):
       try:
           return (None, func(client.pltfm_mgr))
       except InvalidPltfmMgrOperation as ouch:
           return (ouch.code, default)

    return thrift_try(pm_cb_run)
