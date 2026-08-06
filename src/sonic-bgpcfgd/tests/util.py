import os
import subprocess
import tempfile
import yaml

# The production constants are owned by the shared build template
# files/build_templates/constants.yml.j2 (the same source used to generate
# /etc/sonic/constants.yml for real images). Render it here so the tests use
# the real constants without depending on a separate static copy.
CONSTANTS_TEMPLATE_PATH = os.path.abspath(
    '../../files/build_templates/constants.yml.j2')


def render_constants(template_path=CONSTANTS_TEMPLATE_PATH):
    """Render constants.yml.j2 into a temp file and return its path.

    Optional downstream constants are applied by the same renderer used by the
    image build.
    """
    fd, path = tempfile.mkstemp(prefix='constants', suffix='.yml')
    os.close(fd)
    repository_root = os.path.abspath(os.path.join(
        os.path.dirname(__file__), '..', '..', '..'))
    renderer = os.path.join(repository_root, 'scripts', 'render_constants.py')
    env = os.environ.copy()
    env.setdefault('ENABLE_FRR_SNMP_AGENT', 'y')
    with open(path, 'w') as output:
        subprocess.run(
            ['python3', renderer, template_path],
            check=True,
            env=env,
            stdout=output,
        )
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
