import os
import tempfile
import yaml
from jinja2 import Template

# The production constants are owned by the shared build template
# files/build_templates/constants.yml.j2 (the same source used to generate
# /etc/sonic/constants.yml for real images). Render it here so the tests use
# the real constants without depending on a separate static copy.
CONSTANTS_TEMPLATE_PATH = os.path.abspath(
    '../../files/build_templates/constants.yml.j2')


def render_constants(template_path=CONSTANTS_TEMPLATE_PATH):
    """Render constants.yml.j2 into a temp file and return its path.

    The template only references ENABLE_FRR_SNMP_AGENT (defaults to 'y', the
    same default as rules/config); everything else is static YAML.
    """
    with open(template_path) as f:
        rendered = Template(f.read()).render(
            ENABLE_FRR_SNMP_AGENT=os.environ.get('ENABLE_FRR_SNMP_AGENT', 'y'))
    fd, path = tempfile.mkstemp(prefix='constants', suffix='.yml')
    with os.fdopen(fd, 'w') as f:
        f.write(rendered)
    return path


CONSTANTS_PATH = render_constants()


def load_constants_dir_mappings():
    data = load_constants()
    result = {}
    assert "bgp" in data["constants"], "'bgp' key not found in constants.yml"
    assert "peers" in data["constants"]["bgp"], "'peers' key not found in constants.yml"
    for name, value in data["constants"]["bgp"]["peers"].items():
        assert "template_dir" in value, "'template_dir' key not found for peer '%s'" % name
        result[name] = value["template_dir"]
    return result


def load_constants(constants=CONSTANTS_PATH):
    with open(constants) as f:
        data = yaml.safe_load(f)
    assert "constants" in data, "'constants' key not found in constants.yml"
    return data
