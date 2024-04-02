import grpc
import sonic_platform
import multiprocessing
import sys
from concurrent import futures
import subprocess
from swsscommon.swsscommon import SonicV2Connector
import syslog
from pathlib import Path
from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
sys.path.append(str(Path(sonic_platform.__file__).parent.absolute()))

from sonic_platform import health_pb2, health_pb2_grpc

SYSLOG_IDENTIFIER = 'dpu-health-service'

def log_err(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_ERR,msg)
    syslog.closelog()

def log_info(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_INFO,msg)
    syslog.closelog()


try:
    from health_checker.manager import HealthCheckerManager
    from sonic_platform.chassis import Chassis
    from sonic_platform.helper import APIHelper
except Exception as e:
    log_err(f'failed to load modules due to {e}')

IP = "0.0.0.0"
PORT = 12000
EEPROM_INFO_TABLE = 'EEPROM_INFO'

class HealthSvcServicer(health_pb2_grpc.HealthSvcServicer):

    def __init__(self):
        self.chassis = None

    def _get_slot_id(self):
        try:
            if self.chassis == None:
                self.chassis = Chassis()
            slot = self.chassis.get_my_slot()
        except Exception as e:
            log_err("failed to fetch chassis slot id due to {e}")
            slot = -1
        slot_id = str(slot) if slot != -1 else 'UNDEFINED'
        return slot_id

    def _fetch_monitor_list(self):
        try:
            if self.chassis == None:
                self.chassis = Chassis()
            manager = HealthCheckerManager()
            manager.config.load_config()
            stats = manager.check(self.chassis)
            return stats
        except:
            log_err('failed to fetch sonic host health check status')
            return None

    def _populate_system_health_summary(self, stat):
        try:
            led_status = self.chassis.get_status_led()
            services_status = health_pb2.StatusResponse.SummaryInfo.Status.OK
            hardware_status = health_pb2.StatusResponse.SummaryInfo.Status.OK
            not_running = ""
            not_accessible = ""
            hardware_reason = ""
            services_list = []
            fs_list = []
            device_list =[]
            for category, elements in stat.items():
                for element in elements:
                    if elements[element]['status'] != "OK":
                        if category == 'Services':
                            if 'Accessible' in elements[element]['message']:
                                fs_list.append(element)
                            else:
                                services_list.append(element)
                        else:
                            device_list.append(elements[element]['message'])
            if services_list or fs_list:
                services_status = health_pb2.StatusResponse.SummaryInfo.Status.NOT_OK
            if services_list:
                not_running = ', '.join(services_list)
            if fs_list:
                not_accessible = ', '.join(fs_list)
            if device_list:
                hardware_status = health_pb2.StatusResponse.SummaryInfo.Status.NOT_OK
                device_list.reverse()
                hardware_reason = device_list[0]
                if len(device_list) > 1:
                    hardware_reason += '\n'.join(("\t     " + x) for x in device_list[1:])
            return health_pb2.StatusResponse.SummaryInfo(
                led_status = led_status,
                services_status = services_status,
                not_running = not_running,
                not_accessible = not_accessible,
                hardware_status = hardware_status,
                hardware_reason = hardware_reason
            )
        except Exception as e:
            log_err(f'failed to populate system health summary data due to {e}')
            return health_pb2.StatusResponse.SummaryInfo()

    def read_eeprom_from_db(self):
        tlv_dict = {}

        db = SonicV2Connector(host="127.0.0.1")
        db.connect(db.STATE_DB)

        initialized = db.get(db.STATE_DB, '{}|{}'.format(EEPROM_INFO_TABLE, 'State'), 'Initialized')
        if initialized != '1':
            return None

        tlv_header = db.get_all(db.STATE_DB, '{}|{}'.format(EEPROM_INFO_TABLE, 'TlvHeader'))
        tlv_dict['header'] = {}
        tlv_dict['header']['id'] = tlv_header.get('Id String', 'N/A')
        tlv_dict['header']['version'] = tlv_header.get('Version', 'N/A')
        tlv_dict['header']['length'] = tlv_header.get('Total Length', 'N/A')

        tlv_dict['tlv_list'] = []
        concerned_tlvs = []
        concerned_tlvs.extend(range(TlvInfoDecoder._TLV_CODE_PRODUCT_NAME, TlvInfoDecoder._TLV_CODE_SERVICE_TAG + 1))
        concerned_tlvs.append(TlvInfoDecoder._TLV_CODE_VENDOR_EXT)
        concerned_tlvs.append(TlvInfoDecoder._TLV_CODE_CRC_32)
        for tlv_code in concerned_tlvs:
            tlv_code_string = '0x{:02X}'.format(tlv_code)

            tlv_data = db.get_all(db.STATE_DB, '{}|{}'.format(EEPROM_INFO_TABLE, tlv_code_string.lower()))
            if not tlv_data:
                continue

            if tlv_code == TlvInfoDecoder._TLV_CODE_VENDOR_EXT:
                num_vendor_ext = int(tlv_data.get('Num_vendor_ext', '0'))
                for i in range(num_vendor_ext):
                    tlv = {}
                    tlv['code'] = tlv_code_string
                    tlv['name'] = tlv_data.get('Name_{}'.format(i), 'N/A')
                    tlv['length'] = tlv_data.get('Len_{}'.format(i), 'N/A')
                    tlv['value'] = tlv_data.get('Value_{}'.format(i), 'N/A')
                    tlv_dict['tlv_list'].append(tlv)
            else:
                tlv = {}
                tlv['code'] = tlv_code_string
                tlv['name'] = tlv_data.get('Name', 'N/A')
                tlv['length'] = tlv_data.get('Len', 'N/A')
                tlv['value'] = tlv_data.get('Value', 'N/A')
                tlv_dict['tlv_list'].append(tlv)

        checksum_valid = db.get(db.STATE_DB, '{}|{}'.format(EEPROM_INFO_TABLE, 'Checksum'), 'Valid')
        tlv_dict['checksum_valid'] = (checksum_valid == '1')

        return tlv_dict

    def GetEeprom(self, request, context):
        slot_id = self._get_slot_id()
        tlvfields = []
        try:
            tlv_dict = self.read_eeprom_from_db()
            if tlv_dict == None:
                return health_pb2.EepromResponse(slot_id=slot_id, success=False)
            header = health_pb2.EepromResponse.HeaderInfo(
                id = tlv_dict['header']['id'],
                version = tlv_dict['header']['version'],
                length = tlv_dict['header']['length']
            )
            for tlv in tlv_dict['tlv_list']:
                tlv_field_info = health_pb2.EepromResponse.TlvFieldInfo(
                    name = tlv['name'],
                    code = tlv['code'],
                    len = tlv['length'],
                    value = tlv['value']
                )
                tlvfields.append(tlv_field_info)
            return health_pb2.EepromResponse(slot_id=slot_id, header=header, tlvfields=tlvfields, success=True)
        except Exception as e:
            log_err(f'failed to fetch eeprom data due to {e}')
            return health_pb2.EepromResponse(slot_id=slot_id, success=False)


    def GetHealth(self, request, context):

        status_type_map = {
            "OK": health_pb2.StatusResponse.StatusInfo.OK,
            "Not OK": health_pb2.StatusResponse.StatusInfo.NOT_OK,
            "Program": health_pb2.StatusResponse.StatusInfo.PROGRAM,
            "Process": health_pb2.StatusResponse.StatusInfo.PROCESS,
            "System": health_pb2.StatusResponse.StatusInfo.SYSTEM,
            "Filesystem": health_pb2.StatusResponse.StatusInfo.FILESYSTEM,
            "Service": health_pb2.StatusResponse.StatusInfo.SERVICE,
            "UserDefine": health_pb2.StatusResponse.StatusInfo.USERDEFINE
        }

        statuses = []
        slot_id = self._get_slot_id()
        try:
            stat = self._fetch_monitor_list()
            if stat == None:
                return health_pb2.StatusResponse(slot_id=slot_id, statuses=[], success=False)
            summary_info = self._populate_system_health_summary(stat)
            for elements in stat.values():
                for element in sorted(elements.items(), key=lambda x: x[1]['status']):
                    name = element[0]
                    status = element[1]['status']
                    element_type = element[1]['type']
                    status_enum = status_type_map.get(status, health_pb2.StatusResponse.StatusInfo.NOT_OK)
                    type_enum = status_type_map.get(element_type, health_pb2.StatusResponse.StatusInfo.UNKNOWN)
                    status_info = health_pb2.StatusResponse.StatusInfo(
                        name=name,
                        status=status_enum,
                        type=type_enum
                    )
                    statuses.append(status_info)
        except Exception as e:
            log_err('failed to fetch health data')
            return health_pb2.StatusResponse(slot_id=slot_id, statuses=[], success=False)
        return health_pb2.StatusResponse(slot_id=slot_id, summary=summary_info, statuses=statuses, success=True)

    def GetRebootCauses(self, request, context):
        slot_id = self._get_slot_id()
        try:
            REBOOT_CAUSE_TABLE_NAME = "REBOOT_CAUSE"
            TABLE_NAME_SEPARATOR = '|'
            db = SonicV2Connector(host='127.0.0.1')
            db.connect(db.STATE_DB, False)
            prefix = REBOOT_CAUSE_TABLE_NAME + TABLE_NAME_SEPARATOR
            _hash = '{}{}'.format(prefix, '*')
            table_keys = db.keys(db.STATE_DB, _hash)
            reboot_causes = []
            if table_keys is not None:
                table_keys.sort(reverse=True)

                table = []
                for tk in table_keys:
                    entry = db.get_all(db.STATE_DB, tk)
                    name = tk.replace(prefix, "")
                    cause = entry['cause'] if 'cause' in entry else ""
                    time = entry['time'] if 'time' in entry else ""
                    user = entry['user'] if 'user' in entry else ""
                    comment = entry['comment'] if 'comment' in entry else ""
                    reboot_causes.append(
                        health_pb2.RebootCause(
                            name=name, cause=cause, time=time, user=user, comment=comment
                        )
                    )
                return health_pb2.RebootCauseResponse(slot_id=slot_id, reboot_causes=reboot_causes, success=True)
            else:
                log_err("Reboot-cause history is not yet available in StateDB")
                return health_pb2.RebootCauseResponse(slot_id=slot_id, reboot_causes=[], success=False)
        except Exception as e:
            log_err(f"Failed to sent reboot cause response due to {e}")
            return health_pb2.RebootCauseResponse(slot_id=slot_id, reboot_causes=[], success=False)

def serve():
    channel_addr = "{}:{}".format(IP, str(PORT))
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    health_pb2_grpc.add_HealthSvcServicer_to_server(HealthSvcServicer(), server)
    server.add_insecure_port(channel_addr)
    server.start()
    log_info(f"Server started. Listening on port {PORT}")
    server.wait_for_termination()

if __name__ == '__main__':
    process = multiprocessing.Process(target=serve)
    process.start()
    process.join()
