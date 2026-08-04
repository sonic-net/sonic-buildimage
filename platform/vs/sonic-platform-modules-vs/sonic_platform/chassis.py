# Description: Module contains the definitions of SONiC platform APIs
# which provide the chassis specific details
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform_base.module_base import ModuleBase
    import os
    import json
    import threading

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

    def _read_metadata(self):
        metadata = {}
        if os.path.exists(self.metadata_file):
            with open(self.metadata_file, 'r') as f:
                metadata = json.load(f)
        else:
            raise FileNotFoundError("Metadata file {} not found".format(self.metadata_file))
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

