#!/usr/bin/env python3

import os
import sys
import yaml


EMPTY_VALUE_ALLOWED_KEYS = [
    'Previous patch',
    'Patch extern name'
]

def check_dict_value(dic):
    rv = 0
    for k, v in dic.items():
        if v is None and k not in EMPTY_VALUE_ALLOWED_KEYS:
            print(k, ": ", "____missing____")
            rv = 1
        elif isinstance(v, dict):
            check_dict_value(v)
        elif isinstance(v, list):
            for item in v:
                if isinstance(item, dict):
                    check_dict_value(item)
        elif k not in EMPTY_VALUE_ALLOWED_KEYS:
            print(k, ": ", v)

    return rv

if __name__ == '__main__':
    yaml_file = sys.argv[1]

    with open(yaml_file, 'r') as fp:
        patch_info = yaml.load(fp, Loader=yaml.SafeLoader)

    sys.exit(check_dict_value(patch_info))
