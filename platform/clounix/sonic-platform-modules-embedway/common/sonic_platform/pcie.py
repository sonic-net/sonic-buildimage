import os
import yaml
from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil

id_monitor_list = ['8600']

class Pcie(PcieUtil):
    def __init__(self, platform_path):
        self.__pcieutil = PcieUtil(platform_path)
        self.__monitor_list = id_monitor_list
        self.config_path = platform_path
        self._conf_rev = None
        self._generate_yaml = False
        self.total_monitor_obj = 2

    def get_pcie_device(self):
        return self.__pcieutil.get_pcie_device()

    def get_pcie_check(self):
        if self._generate_yaml == True:
            self.dump_conf_yaml()

        return self.__pcieutil.get_pcie_check()

    def dump_conf_yaml(self):
        curInfo = self.get_pcie_device()
        monitor_info = []
        for item in curInfo:
            if item["id"] in self.__monitor_list:
                monitor_info.append(item)

        conf_rev = "_{}".format(self._conf_rev) if self._conf_rev else ""
        config_file = "{}/pcie{}.yaml".format(self.config_path, conf_rev)
        with open(config_file, "w") as conf_file:
            yaml.dump(monitor_info, conf_file, default_flow_style=False)

        fd = os.popen("cat " + config_file + " | grep \"\- bus: \" | wc -l")
        num = fd.readline()
        fd.close()

        num = int(num)
        if num != self.total_monitor_obj:
            os.system("sudo rm " + config_file)

        return
