# Description: Module contains the definitions of SONiC platform APIs
# which provide the chassis specific details
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    import os
    import time
    import json
    import sys

    import grpc

    from xcvr_emu.proto import emulator_pb2 as pb2
    # see https://github.com/grpc/grpc/issues/29459#issuecomment-1641587881
    proto_dir = os.path.dirname(pb2.__file__)
    sys.path.append(proto_dir)

    from xcvr_emu.proto import emulator_pb2_grpc

    from sonic_platform_base.chassis_base import ChassisBase
    from .sfp import Sfp

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Chassis(ChassisBase):
    """
    VS Platform-specific Chassis class
    """
    def __init__(self):
        ChassisBase.__init__(self)
        self.metadata_file = '/etc/sonic/vs_chassis_metadata.json'
        self.metadata = self._read_metadata()
        channel = grpc.insecure_channel("localhost:50051")
        self.xcvr_emu = emulator_pb2_grpc.SfpEmulatorServiceStub(channel)
        self.sfps = {}

    def _read_metadata(self):
        metadata = {}
        if os.path.exists(self.metadata_file):
            with open(self.metadata_file, 'r') as f:
                metadata = json.load(f)
        return metadata

    def get_supervisor_slot(self):
        if 'sup_slot_num' not in self.metadata:
            raise KeyError("sup_slot_num not found in Metadata file {}".format(self.metadata_file))
        return self.metadata['sup_slot_num']

    def get_linecard_slot(self):
        if 'lc_slot_num' not in self.metadata:
            raise KeyError("lc_slot_num not found in Metadata file {}".format(self.metadata_file))
        return self.metadata['lc_slot_num']

    def get_my_slot(self):
        if 'is_supervisor' not in self.metadata or 'is_linecard' not in self.metadata:
            raise KeyError("is_supervisor or is_linecard not found in metadata file {}".format(self.metadata_file))

        if self.metadata['is_supervisor']:
            return self.get_supervisor_slot()
        elif self.metadata['is_linecard']:
            return self.get_linecard_slot()
        else:
            raise ValueError("Invalid configuration: Neither supervisor nor line card")

    def get_sfp(self, index):
        if index not in self.sfps:
            sfp = Sfp(index, self.xcvr_emu)
            self.sfps[index] = (sfp, sfp.get_presence())

        return self.sfps[index][0]

    def get_change_event(self, timeout=0):

        port_dict = {}
        change_dict = {"sfp": port_dict}

        start = time.time()

        while True:
            for (sfp, present) in self.sfps.values():
                current = sfp.get_presence()
                if current != present:
                    port_dict[sfp.index] = '1' if current else '0'
                    self.sfps[sfp.index] = (sfp, current)

            if len(port_dict):
                return True, change_dict

            if timeout > 0 and (time.time() - start > (timeout / 1000)):
                return True, change_dict

            time.sleep(0.5)

    def get_reboot_cause(self):
        return ("REBOOT_CAUSE_NON_HARDWARE", "Unknown")
