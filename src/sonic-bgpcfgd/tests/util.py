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
# Optional organization overlay deep-merged on top of the base at image build
# time (see files/build_templates/sonic_debian_extension.j2). Apply the same
# merge here so the tests validate the constants that ship in the real image.
CONSTANTS_OVERLAY_TEMPLATE_PATH = os.path.abspath(
    '../../files/build_templates/constants.yml.overlay.j2')


def _deep_merge(base, overlay):
    """Deep-merge overlay onto base, matching scripts/deep_merge_yaml.py.

    Mappings merge recursively; a null overlay value deletes the key; any other
    value (scalar or list) replaces the base value.
    """
    if not isinstance(base, dict) or not isinstance(overlay, dict):
        return overlay
    result = dict(base)
    for key, value in overlay.items():
        if value is None:
            result.pop(key, None)
        elif isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = _deep_merge(result[key], value)
        else:
            result[key] = value
    return result


def _render_template(template_path):
    with open(template_path) as f:
        return Template(f.read()).render(
            ENABLE_FRR_SNMP_AGENT=os.environ.get('ENABLE_FRR_SNMP_AGENT', 'y'))


def render_constants(template_path=CONSTANTS_TEMPLATE_PATH,
                     overlay_path=CONSTANTS_OVERLAY_TEMPLATE_PATH):
    """Render constants.yml.j2 into a temp file and return its path.

    The template only references ENABLE_FRR_SNMP_AGENT (defaults to 'y', the
    same default as rules/config); everything else is static YAML. When the
    optional organization overlay is present it is deep-merged on top of the
    base, mirroring the image build, so the tests see the shipped constants.
    """
    data = yaml.safe_load(_render_template(template_path))
    if overlay_path and os.path.isfile(overlay_path):
        overlay = yaml.safe_load(_render_template(overlay_path))
        if overlay is not None:
            data = _deep_merge(data, overlay)
    fd, path = tempfile.mkstemp(prefix='constants', suffix='.yml')
    with os.fdopen(fd, 'w') as f:
        yaml.safe_dump(data, f, default_flow_style=False, sort_keys=False)
    return path


CONSTANTS_PATH = render_constants()


def resolve_expected_output(path):
    variant = os.environ.get('SONIC_CFGGEN_OUTPUT_VARIANT')
    if not variant:
        return path

    stem, extension = os.path.splitext(path)
    variant_path = '{}_{}{}'.format(stem, variant, extension)
    return variant_path if os.path.isfile(variant_path) else path


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
