# -*- coding: utf-8 -*-

import os

import jinja2


TEMPLATE_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..', 'files', 'image_config',
    'rsyslog'))


def render_host_template(server_config):
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(TEMPLATE_DIR))
    return env.get_template('rsyslog.conf.j2').render({
        'SYSLOG_CONFIG': {'GLOBAL': {}},
        'SYSLOG_SERVER': {'192.0.2.1': server_config},
        'udp_server_ip': '127.0.0.1',
        'hostname': 'sonic',
    })


def test_rate_limits_render_valid_values():
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(TEMPLATE_DIR))
    contexts = {
        'rsyslog.conf.j2': {
            'SYSLOG_CONFIG': {'GLOBAL': {
                'rate_limit_interval': '600',
                'rate_limit_burst': '20000',
            }},
            'udp_server_ip': '127.0.0.1',
            'hostname': 'sonic',
        },
        'rsyslog-container.conf.j2': {
            'SYSLOG_CONFIG_FEATURE': {'swss': {
                'rate_limit_interval': '600',
                'rate_limit_burst': '20000',
            }},
            'container_name': 'swss',
        },
    }

    for template_name, context in contexts.items():
        rendered = env.get_template(template_name).render(context)
        assert 'SysSock.RateLimit.Interval="600"' in rendered
        assert 'SysSock.RateLimit.Burst="20000"' in rendered


def test_rate_limits_reject_invalid_values():
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(TEMPLATE_DIR))
    for value in ('not-a-number', '0.9', '-1', u'²', u'١',
                  '2147483648', '9' * 5000):
        contexts = {
            'rsyslog.conf.j2': {
                'SYSLOG_CONFIG': {'GLOBAL': {
                    'rate_limit_interval': value,
                    'rate_limit_burst': value,
                }},
                'udp_server_ip': '127.0.0.1',
                'hostname': 'sonic',
            },
            'rsyslog-container.conf.j2': {
                'SYSLOG_CONFIG_FEATURE': {'swss': {
                    'rate_limit_interval': value,
                    'rate_limit_burst': value,
                }},
                'container_name': 'swss',
            },
        }

        for template_name, context in contexts.items():
            rendered = env.get_template(template_name).render(context)
            assert 'SysSock.RateLimit.Interval="300"' in rendered
            assert 'SysSock.RateLimit.Burst="20000"' in rendered


def test_rate_limits_preserve_explicit_zero():
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(TEMPLATE_DIR))
    contexts = {
        'rsyslog.conf.j2': {
            'SYSLOG_CONFIG': {'GLOBAL': {
                'rate_limit_interval': '0',
                'rate_limit_burst': '0',
            }},
            'udp_server_ip': '127.0.0.1',
            'hostname': 'sonic',
        },
        'rsyslog-container.conf.j2': {
            'SYSLOG_CONFIG_FEATURE': {'swss': {
                'rate_limit_interval': '0',
                'rate_limit_burst': '0',
            }},
            'container_name': 'swss',
        },
    }

    for template_name, context in contexts.items():
        rendered = env.get_template(template_name).render(context)
        assert 'SysSock.RateLimit.Interval="0"' in rendered
        assert 'SysSock.RateLimit.Burst="0"' in rendered


def test_remote_syslog_fields_accept_valid_values():
    rendered = render_host_template({
        'severity': 'warning',
        'protocol': 'tcp',
        'port': '6514',
        'source': 'Ethernet0',
        'vrf': 'Vrf1',
    })

    assert '*.warning' in rendered
    assert 'Target="192.0.2.1" Port="6514" Protocol="tcp"' in rendered
    assert 'Address="Ethernet0"' in rendered
    assert 'Device="Vrf1"' in rendered


def test_remote_syslog_fields_use_defaults_for_invalid_values():
    rendered = render_host_template({
        'severity': 'unsupported',
        'protocol': 'unsupported',
    })

    assert 'unsupported' not in rendered
    assert '*.*' in rendered
    assert 'Target="192.0.2.1" Port="514" Protocol="udp"' in rendered


def test_remote_syslog_preserves_supported_severities():
    for severity in ('none', 'warn'):
        rendered = render_host_template({'severity': severity})
        assert '*.{}'.format(severity) in rendered
        assert '*.*' not in rendered
