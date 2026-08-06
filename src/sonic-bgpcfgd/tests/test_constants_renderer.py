import os
import subprocess

import yaml


REPOSITORY_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..')
)
RENDERER = os.path.join(REPOSITORY_ROOT, 'scripts', 'render_constants.py')
BASE_TEMPLATE = os.path.join(
    REPOSITORY_ROOT,
    'files',
    'build_templates',
    'constants.yml.j2',
)


def render_constants(overlay=None, env_overlay=None):
    env = os.environ.copy()
    env['ENABLE_FRR_SNMP_AGENT'] = 'y'
    env.pop('SONIC_CONSTANTS_OVERLAY', None)
    if env_overlay:
        env['SONIC_CONSTANTS_OVERLAY'] = str(env_overlay)
    command = ['python3', RENDERER, BASE_TEMPLATE]
    if overlay:
        command.extend(['--overlay', str(overlay)])
    result = subprocess.run(
        command,
        check=True,
        env=env,
        stdout=subprocess.PIPE,
        text=True,
    )
    return yaml.safe_load(result.stdout)


def test_constants_overlay_merges_nested_values(tmp_path):
    overlay = tmp_path / 'overlay.yml'
    overlay.write_text(
        'constants:\n'
        '  bgp:\n'
        '    loopback_advertisement_route_map:\n'
        '      name: TEST_ROUTE_MAP\n'
        '      community: "12345:8888"\n'
    )

    constants = render_constants(overlay)['constants']

    assert constants['bgp']['traffic_shift_community'] == '12345:12345'
    assert constants['bgp']['loopback_advertisement_route_map'] == {
        'name': 'TEST_ROUTE_MAP',
        'community': '12345:8888',
    }


def test_constants_overlay_can_replace_mapping(tmp_path):
    overlay = tmp_path / 'overlay.yml'
    overlay.write_text(
        'constants:\n'
        '  bgp:\n'
        '    peers:\n'
        '      __replace__: true\n'
        '      custom:\n'
        '        template_dir: custom\n'
    )

    peers = render_constants(overlay)['constants']['bgp']['peers']

    assert peers == {
        'custom': {
            'template_dir': 'custom',
        },
    }


def test_constants_environment_overlay_can_replace_root(tmp_path):
    overlay = tmp_path / 'overlay.yml'
    overlay.write_text(
        'constants:\n'
        '  __replace__: true\n'
        '  custom:\n'
        '    enabled: true\n'
    )

    constants = render_constants(env_overlay=overlay)['constants']

    assert constants == {
        'custom': {
            'enabled': True,
        },
    }
