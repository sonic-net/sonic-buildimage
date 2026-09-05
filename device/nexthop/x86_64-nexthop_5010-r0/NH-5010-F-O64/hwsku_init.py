from dataclasses import dataclass
from typing import List, Dict, Optional, Set
import os
import sys
from hwsku_init_common import *

MAX_NH5010_PORTS = 64
TEMPLATE_DIR = "templates"
OUTPUT_DIR = "."
DEFAULTS_DIR = "defaults"
ASIC_BCM_TEMPLATE = "nh5010.bcm.j2"
GB_TEMPLATE = "gearbox_config.json.j2"

q3d_speed_prefix_map = {
    100: "CGE",
    400: "CDGE",
    800: "D3CGE"
}

LANE_SPEED_25G  = 25
LANE_SPEED_50G  = 50
LANE_SPEED_100G = 100

@dataclass
class SKUAsicPortConfig:
    logical_port: str
    asic_port_type: str
    local_tmport: str
    is_pam4: bool

@dataclass
class SKUGbPortConfig:
    name: str

class SkuPipeline(SkuBasePipeline):

    def __init__(
        self,
        speed_map_file: str,
        asic_lane_file: str,
        config_db_file: Optional[str]=None
    ):
        super().__init__(speed_map_file, asic_lane_file, config_db_file)

    def _register_lane_speed(
        self,
        pm_lane_speeds: Dict[int, Set[int]],
        pm: int,
        lane_speed: int,
        port_name: str,
        port_index: int,
        port_speed: int,
    ) -> None:
        speeds = pm_lane_speeds.setdefault(pm, set())
        speeds.add(lane_speed)
        if (LANE_SPEED_25G in speeds and
            (LANE_SPEED_50G in speeds or LANE_SPEED_100G in speeds)):
            raise Exception(
                f"Lane speed conflict in PM {pm} for {port_name} "
                f"(index {port_index}) with speed {port_speed}"
            )

    def generate_plat_asic_config(self) -> List[SKUAsicPortConfig]:
        pm_lane_speeds = {} 
        pm_lane_usage = {}
        asic_port_cfgs: List[SKUAsicPortConfig] = []

        for port_name, port_cfg in self.config_db.items():
            # Find the speed profile for this port
            port_index = port_cfg.index
            if port_index > MAX_NH5010_PORTS:
                continue
            port_speed = port_cfg.speed
            port_lanes = port_cfg.lanes
            port_speed_profiles = self.port_speed_map[port_index]
            for speed_profile in port_speed_profiles:
                if speed_profile.speed == port_speed and speed_profile.asic.lanes == port_lanes:
                    break
            else:
                raise Exception(
                    f"Failed to find speed profile for port {port_name} (index {port_index}) with speed {port_speed}"
                )

            # Now search the lanes in the ASIC lane map using the prepending of q3d_speed_prefix_map
            asic_port = speed_profile.asic
            asic_core = asic_port.core
            asic_pm = asic_port.pm
            asic_lanes = asic_port.lanes

            q3d_port_name = None
            for key, lanes in self.asic_lane_map.items():
                if key.core != asic_core or key.pm != asic_pm:
                    continue
                if lanes == asic_lanes and key.port_name.startswith(q3d_speed_prefix_map[port_speed]):
                    q3d_port_name = key.port_name
                    break
            if q3d_port_name is None:
                raise Exception(
                    f"Failed to find lanes {asic_lanes} in Q3D port map for port {port_name} "
                    f"(index {port_index}) with speed {port_speed}"
                )

            # Validate if lane conflicts happen in other ports in the pm
            for lane in asic_lanes:
                if lane in pm_lane_usage.setdefault(asic_pm, set()):
                    raise Exception(f"Lane conflict in PM {asic_pm} for port {port_name} "
                                    f"(index {port_index}) with speed {port_speed}")
                pm_lane_usage[asic_pm].add(lane)

            # Validate if pm_lane_speeds have 25G conflicting with 50G/100G in the same pm
            lane_speed = port_speed // len(asic_lanes)
            self._register_lane_speed(pm_lane_speeds, asic_pm, lane_speed, port_name, port_index, port_speed)

            # Now we know the q3d port name, lets constuct the entry for SKUAsicPortConfig
            logical_port = 4*(port_index - 1) + 1
            local_tmport_index = (asic_lanes[0] + 1) % 32
            asic_port_type = q3d_port_name
            local_tmport = f"core_{asic_core}.{local_tmport_index}"   # Core has 4 pms and each pm has 8 lanes
            if q3d_port_name.startswith("CGE") and len(asic_lanes) == 4:
                is_pam4 = False
            else:
                is_pam4 = True

            asic_port_cfgs.append(SKUAsicPortConfig(logical_port=str(logical_port),
                                                    asic_port_type=asic_port_type, 
                                                    local_tmport=local_tmport, 
                                                    is_pam4=is_pam4))

        LOG.log_info(f"Generated asic config: {asic_port_cfgs}")
        return asic_port_cfgs

    def generate_plat_gearbox_config(self) -> List[SKUGbPortConfig]:
        raise NotImplementedError("generate_plat_gearbox_config() not implemented")

def __main__():
	"""Entry point.

	If a base directory is provided as the first CLI argument, all file
	paths used by the pipeline are resolved relative to that directory.
	Otherwise, they default to the directory containing this script.
	"""
	if len(sys.argv) > 2:
		print(f"Usage: {sys.argv[0]} [base_dir]", file=sys.stderr)
		sys.exit(1)

	if len(sys.argv) == 2:
		base_dir = sys.argv[1]
	else:
		base_dir = os.path.dirname(os.path.abspath(__file__))

	pipeline = SkuPipeline(
			speed_map_file=os.path.join(base_dir, "port_speed.json"),
			asic_lane_file=os.path.join(base_dir, "Q3D_ports.json"),
	)
	pipeline.run(
			template_dir=os.path.join(base_dir, TEMPLATE_DIR),
			defaults_dir=os.path.join(base_dir, DEFAULTS_DIR),
			output_dir=os.path.join(base_dir, OUTPUT_DIR),
			asic_bcm_template=ASIC_BCM_TEMPLATE,
			gearbox_template=GB_TEMPLATE,
	)

if __name__ == '__main__':
    __main__()
