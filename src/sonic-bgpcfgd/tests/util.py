import os
import yaml

CONSTANTS_PATH = os.path.abspath('../../files/image_config/constants/constants.yml')

def load_constants_dir_mappings():
    data = load_constants()
    result = {}
    assert "bgp" in data["constants"], "'bgp' key not found in constants.yml"
    assert "peers" in data["constants"]["bgp"], "'peers' key not found in constants.yml"
    for name, value in data["constants"]["bgp"]["peers"].items():
        assert "template_dir" in value, "'template_dir' key not found for peer '%s'" % name
        result[name] = value["template_dir"]
    return result

def load_constants(constants = CONSTANTS_PATH):
    with open(constants) as f:
        data = yaml.safe_load(f)
    assert "constants" in data, "'constants' key not found in constants.yml"
    return data
