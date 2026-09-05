import argparse
import os
from hwsku_init_impl import HwSKUInit

DEFAULT_HWSKU_DIR = '/usr/share/sonic/hwsku' 

def __main__():
    parser = argparse.ArgumentParser(description='Initialize for a given hwsku')
    parser.add_argument('-s', '--hwsku_dir', help='path of hwsku directory')
    parser.add_argument('-n', '--asic_id', help='asic id')
    parser.add_argument('-c', '--config_db_file', help='config db file')

    args = parser.parse_args()

    hwsku_dir = args.hwsku_dir if args.hwsku_dir else DEFAULT_HWSKU_DIR
    asic_id = int(args.asic_id) if args.asic_id else None
    config_db_file = args.config_db_file if args.config_db_file else None

    if asic_id is not None:
        asic_dir = os.path.join(hwsku_dir, str(asic_id))
    else:
        asic_dir = hwsku_dir

    HwSKUInit(asic_dir, config_db_file).do_init()    
    
if __name__ == '__main__':
    __main__()
