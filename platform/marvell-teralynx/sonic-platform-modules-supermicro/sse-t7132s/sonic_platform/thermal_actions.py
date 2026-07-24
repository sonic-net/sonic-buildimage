from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common import logger


class SfpIrqAction(ThermalPolicyActionBase):
    """
    Base thermal action class to do SFP actions
    """
    def __init__(self):
        self.logger = logger.Logger('thermalctld.SfpIrqAction')

    def get_sfp_irq_info(self, thermal_info_dict):
        from .thermal_infos import SfpIrqInfo
        if SfpIrqInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[SfpIrqInfo.INFO_NAME], SfpIrqInfo):
            return thermal_info_dict[SfpIrqInfo.INFO_NAME]
        else:
            return None

@thermal_json_object('sfp.irq.log')
class SfpIrqLog(SfpIrqAction):
    """
    Action to do SFP IRQ log
    """
    def execute(self, thermal_info_dict):
        """
        Log IRQ summary
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        irq_default = {
            #'DP1State': 'DataPathActivated',               # handled separately
            #'DP2State': 'DataPathActivated',               # handled separately
            #'DP3State': 'DataPathActivated',               # handled separately
            #'DP4State': 'DataPathActivated',               # handled separately
            #'DP5State': 'DataPathActivated',               # handled separately
            #'DP6State': 'DataPathActivated',               # handled separately
            #'DP7State': 'DataPathActivated',               # handled separately
            #'DP8State': 'DataPathActivated',               # handled separately
            #'Interrupt': 'Interrupt not asserted',         # handled separately
            #'config_state_hostlane1': 'ConfigUndefined',   # ignore
            #'config_state_hostlane2': 'ConfigUndefined',   # ignore
            #'config_state_hostlane3': 'ConfigUndefined',   # ignore
            #'config_state_hostlane4': 'ConfigUndefined',   # ignore
            #'config_state_hostlane5': 'ConfigUndefined',   # ignore
            #'config_state_hostlane6': 'ConfigUndefined',   # ignore
            #'config_state_hostlane7': 'ConfigUndefined',   # ignore
            #'config_state_hostlane8': 'ConfigUndefined',   # ignore
            'datapath_firmware_fault': False,
            'dpinit_pending_hostlane': '00000000',
            #'dpstatechanged_flag': '00000000',             # handled separately
            'lasertemphighalarm_flag': False,
            'lasertemphighwarning_flag': False,
            'lasertemplowalarm_flag': False,
            'lasertemplowwarning_flag': False,
            'module_fault_cause': 'No Fault detected',
            'module_firmware_fault': False,
            #'module_state': 'ModuleReady',                 # handled separately
            #'module_state_changed': False,                 # handled separately
            'rxcdrlol': '00000000',
            'rxlos': '00000000',
            #'rxoutput_status_hostlane': '00000000',        # not a interrupt flag
            'rxpowerhighalarm_flag': '00000000',
            'rxpowerhighwarning_flag': '00000000',
            'rxpowerlowalarm_flag': '00000000',
            'rxpowerlowwarning_flag': '00000000',
            'temphighalarm_flag': False,
            'temphighwarning_flag': False,
            'templowalarm_flag': False,
            'templowwarning_flag': False,
            'txadaptiveinputeqfault_flag': '00000000',
            'txbiashighalarm_flag': '00000000',
            'txbiashighwarning_flag': '00000000',
            'txbiaslowalarm_flag': '00000000',
            'txbiaslowwarning_flag': '00000000',
            'txcdrlol_hostlane': '00000000',
            'txfault': '00000000',
            'txlos_hostlane': '00000000',
            #'txoutput_status': '00000000',                 # not a interrupt flag
            'txpowerhighalarm_flag': '00000000',
            'txpowerhighwarning_flag': '00000000',
            'txpowerlowalarm_flag': '00000000',
            'txpowerlowwarning_flag': '00000000',
            'vcchighalarm_flag': False,
            'vcchighwarning_flag': False,
            'vcclowalarm_flag': False,
            'vcclowwarning_flag': False
        }

        sfp_irq_info_obj = self.get_sfp_irq_info(thermal_info_dict)
        now_and_last = list(zip(sfp_irq_info_obj.irq_info, sfp_irq_info_obj.irq_info_last))
        for (sfp, irq), (sfp_last, irq_last) in now_and_last:
            if not irq:
                if (sfp == sfp_last) and irq_last:
                    self.logger.log_warning("{} IRQ de-asserted"
                                            .format(", ".join(sfp.get_names())))
                continue
            flags = ""
            for k_def, v_def in irq_default.items():
                v_irq = irq.get(k_def)
                if v_irq != v_def:
                    flags += "[{}]:{} ".format(k_def, v_irq)
            # module state change
            k_mod = 'module_state_changed'
            v_mod = irq.get(k_mod)
            if v_mod != False:
                flags += "[{}]:{} ".format(k_mod, v_mod)
                flags += "[{}]:{} ".format('module_state', irq.get('module_state'))
            # data path state change
            k_data = 'dpstatechanged_flag'
            v_data = irq.get(k_data)
            if v_data != '00000000':
                flags += "[{}]:{} ".format(k_data, v_data)
                # check each lane
                for i, v in zip('87654321', v_data):
                    if v != '0':
                        k_lane = 'DP{}State'.format(i)
                        flags += "[{}]:{} ".format(k_lane, irq.get(k_lane))
            # no flags?
            if flags == "":
                flags = "None"
            self.logger.log_warning("{} IRQ asserted with flags: {}"
                                    .format(", ".join(sfp.get_names()), flags))

@thermal_json_object('sfp.tx_disable')
class SfpTxDisable(SfpIrqAction):
    """
    Action to do SFP tx disable
    """
    def execute(self, thermal_info_dict):
        """
        Tx disable
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        sfp_irq_info_obj = self.get_sfp_irq_info(thermal_info_dict)
        for sfp in sfp_irq_info_obj.get_temp_high_alarm():
            sfp.tx_disable(True)
            sfp_irq_info_obj.append_tx_disabled(sfp)
            self.logger.log_warning("{} is Tx disabled for temp high alarm".format(sfp.get_name()))

@thermal_json_object('sfp.tx_enable')
class SfpTxEnable(SfpIrqAction):
    """
    Action to do SFP tx enable
    """
    def execute(self, thermal_info_dict):
        """
        Tx enable
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        sfp_irq_info_obj = self.get_sfp_irq_info(thermal_info_dict)
        no_warning = sfp_irq_info_obj.get_no_temp_high_warning()
        to_enable = [s for s in no_warning if sfp_irq_info_obj.is_tx_disabled(s)]
        for sfp in to_enable:
            sfp.tx_disable(False)
            sfp_irq_info_obj.remove_tx_disabled(sfp)
            self.logger.log_warning("Tx enabled for temp high warning de-asserted: {}".format(sfp.get_name()))

