try:
    import os
    import yaml
    from .helper import APIHelper
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

ETH_ENTRY = {
    "dev": "00",
    "fn": "0",
    "id": "1004",  # dev_id=0x1004
    "name": "Ethernet controller: AMD Pensando Systems DSC Management Controller",
}
SER_ENTRY = {
    "dev": "00",
    "fn": "1",
    "id": "100a",  # dev_id=0x100a
    "name": "Serial controller: AMD Pensando Systems DSC Serial Port Controller",
}

HOST_PLATFORM_PATH = '/usr/share/sonic/device'

class Pcie(PcieUtil):
    def __init__(self, path):
        self._api_helper = APIHelper()
        self.pcie_yaml_path = "/".join([HOST_PLATFORM_PATH, self._api_helper.get_platform(), "pcie.yaml"])
        if self._api_helper.is_host():
            self.create_pcie_yaml(self.pcie_yaml_path)
        super().__init__(path)

    def get_pcie_check(self):
        self.load_config_file()
        for item_conf in self.confInfo:
            item_conf["result"] = "Passed"
        return self.confInfo

    def get_pcie_device(self):
        """
        Parse `pcieutil dev` output and return a list of PCI devices,
        enriched with id and name from ETH_ENTRY / SER_ENTRY.
        """
        if self._api_helper.is_host():
            output = self._api_helper.run_docker_cmd("pcieutil dev")
            if not output:
                return []

            lines = output.strip().splitlines()
            if len(lines) < 2:
                return []

            pci_list = []

            # Skip header
            for line in lines[1:]:
                fields = line.split()
                if len(fields) < 4:
                    continue

                # hdl lif name p:bb:dd.f intx intrs
                pcie_addr = fields[3]  # 0:18:00.0

                try:
                    _, bus_hex, devfn = pcie_addr.split(":")
                    dev, fn = devfn.split(".")
                    bus = f"{int(bus_hex, 16):02x}"
                except ValueError:
                    continue

                # Attach ID + Name from constants
                if fn == ETH_ENTRY["fn"]:
                    entry = ETH_ENTRY
                elif fn == SER_ENTRY["fn"]:
                    entry = SER_ENTRY
                else:
                    # Unknown PCIe function – ignore
                    continue

                pci_list.append({
                    "bus": bus,
                    "dev": dev,
                    "fn": fn,
                    "id": entry["id"],
                    "name": entry["name"]
                })
        else:
            if not os.path.exists(self.pcie_yaml_path):
                return []

            try:
                with open(self.pcie_yaml_path, "r", encoding="utf-8") as f:
                    data = yaml.safe_load(f)
            except Exception:
                return []

            if not isinstance(data, list):
                return []

            pci_list = []
            for item in data:
                # Defensive parsing
                try:
                    pci_list.append({
                        "bus": item["bus"],
                        "dev": item["dev"],
                        "fn": item["fn"],
                        "id": item["id"],
                        "name": item["name"]
                    })
                except KeyError:
                    continue

        return pci_list

    def create_pcie_yaml(self, path):
        """
        Create pcie.yaml using fully-populated PCI device info
        """
        pci_devices = self.get_pcie_device()
        if not pci_devices:
            return

        lines = []

        for dev in pci_devices:
            lines.append(f"- bus: '{dev['bus']}'")
            lines.append(f"  dev: '{dev['dev']}'")
            lines.append(f"  fn: '{dev['fn']}'")
            lines.append(f"  id: {dev['id']}")
            lines.append(f"  name: '{dev['name']}'")

        yaml_text = "\n".join(lines) + "\n"

        with open(path, "w", encoding="utf-8") as f:
            f.write(yaml_text)

