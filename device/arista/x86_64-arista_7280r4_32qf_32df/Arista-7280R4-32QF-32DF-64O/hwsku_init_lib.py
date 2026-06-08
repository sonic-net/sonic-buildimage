from dataclasses import dataclass
from jinja2 import Environment, FileSystemLoader
import json
import logging
import os
import re
from swsscommon.swsscommon import ConfigDBConnector
import sys

ASIC_TEMPLATE_DIR = 'template'
HWSKU_CONFIG_FILE = 'hwsku_config.json'

@dataclass
class GearboxPortConfig():
    name : str
    index : int
    phy_id : int
    system_lanes : list
    line_lanes : list

@dataclass
class PhyPortConfig():
    index : int
    system_speed : int
    line_speed : int

class PortSpeed():
    def __init__(self, speed):   # unit: mbps
        if type(speed) == str:
            speed = int(speed)
        self.speed = speed

    def value(self):
        return self.speed

    def shortName(self):
        return f'{self.speed//1000}G'

class HwSKUInitBase():
    def __init__(self, asic_dir, config_db_file):
        self.create_logger()
         
        self.asic_dir = asic_dir
        self.logger.info(f'asic path: {self.asic_dir}')
        self.config_db_file = config_db_file
        self.hwsku_config = self.load_sku_json_file(HWSKU_CONFIG_FILE)
        self.logger.info(f'hwsku config: {str(self.hwsku_config)}')

    def create_logger(self):
        self.logger = logging.getLogger('hwsku-init')
        self.logger.setLevel(logging.DEBUG)
        handler = logging.StreamHandler(sys.stdout)
        handler.setLevel(logging.DEBUG)
        handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s  %(name)s - %(message)s'))
        self.logger.addHandler(handler)
        
    def get_sku_file_path(self, file_name):
        return os.path.join(self.asic_dir, file_name)

    def load_sku_json_file(self, file_name):
        data = None
        try:
            with open(self.get_sku_file_path(file_name)) as f:
                data = json.load(f)
        except FileNotFoundError:
            pass
        return data

    def read_ext_port_config(self):
        if self.config_db_file:
            with open(self.config_db_file) as f:
                data = json.load(f)
            port_config = data['PORT']
        else:
            config_db = ConfigDBConnector()
            config_db.connect()
            port_config = config_db.get_table('PORT')
        return {port : config for port, config in port_config.items() if config['role'] == 'Ext'}

    def render_template(self, template_file, data_name, data, output_file=None):
        # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
        env = Environment(loader=FileSystemLoader(self.get_sku_file_path(ASIC_TEMPLATE_DIR)))

        template = env.get_template(template_file)

        # nosemgrep: python.lang.security.audit.eval-detected.eval-detected
        output = eval(f'template.render({data_name}=data)')
        if output_file:
            with open(self.get_sku_file_path(output_file), 'w') as f:
                f.write(output)
        return output

    def do_init(self):
        pass
