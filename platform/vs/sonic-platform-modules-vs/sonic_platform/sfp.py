import grpc

from xcvr_emu.proto.emulator_pb2 import ReadRequest, WriteRequest, GetInfoRequest

from sonic_platform_base.sfp_base import SfpBase


class Sfp(SfpBase):
    def __init__(self, index, xcvr_emu):
        self.index = index
        self.xcvr_emu = xcvr_emu
        super().__init__()

    def get_model(self):
        api = self.get_xcvr_api()
        return api.get_model() if api is not None else None

    def get_serial(self):
        api = self.get_xcvr_api()
        return api.get_serial() if api is not None else None

    def get_transceiver_info(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_info() if api is not None else None

    def get_transceiver_info_firmware_versions(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_info_firmware_versions() if api is not None else None

    def get_transceiver_bulk_status(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_bulk_status() if api is not None else None

    def get_transceiver_threshold_info(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_threshold_info() if api is not None else None

    def get_transceiver_status(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_status() if api is not None else None

    def get_transceiver_loopback(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_loopback() if api is not None else None

    def is_coherent_module(self):
        api = self.get_xcvr_api()
        return api.is_coherent_module() if api is not None else None

    def get_transceiver_pm(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_pm() if api is not None else None

    def get_presence(self):
        try:
            info = self.xcvr_emu.GetInfo(GetInfoRequest(index=self.index))
        except grpc.RpcError:
            return False
        return info.present

    def is_replaceable(self):
        return True

    def read_eeprom(self, offset, num_bytes):
        if not self.get_presence():
            return None
        # convert optoe offset to SFF page and offset
        # optoe maps the SFF 2D address to a linear address
        page = offset // 128
        if page > 0:
            page = page - 1

        if offset > 128:
            offset = (offset % 128) + 128

        return self.xcvr_emu.Read(
            ReadRequest(index=self.index, offset=offset, page=page, length=num_bytes)
        ).data

    def write_eeprom(self, offset, num_bytes, write_buffer):
        assert len(write_buffer) <= num_bytes
        # convert optoe offset to SFF page and offset
        # optoe maps the SFF 2D address to a linear address
        page = offset // 128
        if page > 0:
            page = page - 1

        if offset > 128:
            offset = (offset % 128) + 128

        return self.xcvr_emu.Write(
            WriteRequest(
                index=self.index, page=page, offset=offset, length=num_bytes, data=bytes(write_buffer)
            )
        )
