from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common import logger
from sonic_py_common import daemon_base
from swsscommon import swsscommon
from datetime import datetime

sonic_logger = logger.Logger('thermal_infos')

@thermal_json_object('sfp_irq_info')
class SfpIrqInfo(ThermalPolicyInfoBase):
    """
    SFP interrupt information needed by thermal policy
    """
    INFO_NAME = 'sfp_irq_info'
    # SFP IRQ information table name in database
    SFP_IRQ_INFO_TABLE_NAME = 'TRANSCEIVER_IRQ_INFO'

    def __init__(self):
        self.irq_info = []
        self.irq_info_last = []
        self.irq_sfp = None
        self.temp_high_alarm = None
        self.no_temp_high_warning = None
        self.tx_disabled = []
        self.count = 0  # debug
        self.state_db = daemon_base.db_connect("STATE_DB")
        self.table = swsscommon.Table(self.state_db, SfpIrqInfo.SFP_IRQ_INFO_TABLE_NAME)

    def collect(self, chassis):
        """
        Collect SFP interrupt and DOM info.
        :param chassis: The chassis object
        :return:
        """
        self.count = self.count + 1  # debug
        if not self.irq_info:
            self.irq_info_last = [(sfp, {}) for sfp in chassis.get_all_sfps()]
        else:
            self.irq_info_last = self.irq_info
        self.irq_info = []
        for sfp in chassis.get_all_sfps():
            if sfp.get_presence() and sfp.get_modirq():
                info = sfp.get_transceiver_interrupt_info()
                self.irq_info.append((sfp, info))
            else:
                self.irq_info.append((sfp, {}))
        self.irq_sfp = None
        self.temp_high_alarm = None
        self.no_temp_high_warning = None

        self.update_database()

    def update_database(self):
        for sfp, irq_info in self.irq_info:
            if self.is_tx_disabled(sfp):
                action = 'Tx disabled'
            else:
                action = 'No action'

            tuple_list = []
            for k, v in irq_info.items():
                tuple_list.append((k, str(v)))
            tuple_list.append(('Port', sfp.port_name))
            tuple_list.append(('Action', action))
            tuple_list.append(('timestamp', datetime.now().strftime('%Y%m%d %H:%M:%S')))

            fvs = swsscommon.FieldValuePairs(tuple_list)
            name = sfp.get_name()
            self.table._del(name)
            self.table.set(name, fvs)

    def get_irq_sfp(self):
        """
        Retrieves SFPs with IRQ.
        :return: a list for IRQ asserted SPFs
        """
        # create the list only once after collect
        if self.irq_sfp is None:
            self.irq_sfp = []
            for sfp, irq in self.irq_info:
                if not irq:
                    continue
                self.irq_sfp.append(sfp)
            self.irq_sfp = [irq for sfp, irq in self.irq_info]
        return self.irq_sfp

    def get_temp_high_alarm(self):
        """
        Retrieves temp high alarm SFPs
        :return: a list for temp high alarm SPFs
        """
        # create the list only once after collect
        if self.temp_high_alarm is None:
            self.temp_high_alarm = []
            for sfp, irq in self.irq_info:
                if not irq:
                    continue
                if irq.get('temphighalarm_flag') != True:
                    continue
                # check temp to make sure current status
                thermals = sfp.get_all_thermals()
                if len(thermals) == 0:
                    continue
                temp = thermals[0].get_temperature()
                temphighalarm = thermals[0].get_high_critical_threshold()
                if temp >= temphighalarm:
                    self.temp_high_alarm.append(sfp)
        return self.temp_high_alarm

    def get_no_temp_high_warning(self):
        """
        Retrieves the list of SFPs that has temp high warning cleared
        :return: a list for no temp high warning SPFs
        """
        # create the list only once after collect
        if self.no_temp_high_warning is None:
            self.no_temp_high_warning = []
            for sfp, irq in self.irq_info:
                if not irq:
                    # no need to check temp
                    self.no_temp_high_warning.append(sfp)
                    continue
                if irq.get('temphighwarning_flag') == False:
                    # no need to check temp
                    self.no_temp_high_warning.append(sfp)
                    continue
                # check temp to make sure current status
                thermals = sfp.get_all_thermals()
                if len(thermals) == 0:
                    continue
                temp = thermals[0].get_temperature()
                temphighwarning = thermals[0].get_high_threshold()
                if temp < temphighwarning:
                    self.no_temp_high_warning.append(sfp)
        return self.no_temp_high_warning

    def append_tx_disabled(self, sfp):
        """
        append sfp to tx disabled list
        """
        if sfp not in self.tx_disabled:
            self.tx_disabled.append(sfp)
            self.update_database()

    def remove_tx_disabled(self, sfp):
        """
        remove sfp from tx disabled list
        """
        if sfp in self.tx_disabled:
            self.tx_disabled.remove(sfp)
            self.update_database()

    def is_tx_disabled(self, sfp):
        """
        check a sfp is tx disabled or not
        """
        return(sfp in self.tx_disabled)

