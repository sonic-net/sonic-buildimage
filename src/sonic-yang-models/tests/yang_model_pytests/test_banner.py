# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
"""init_cfg.json.j2 seeds CONFIG_DB's BANNER_MESSAGE|global, and sonic-banner.yang's
defaults document it, so the two must give the same login message."""

import json
import os

import jinja2

CUR_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(CUR_DIR, '..', '..', '..', '..'))  # sonic-buildimage
INIT_CFG_TEMPLATE = os.path.join('files', 'build_templates', 'init_cfg.json.j2')
LOGIN_LEAF = '/sonic-banner:sonic-banner/BANNER_MESSAGE/global/login'


def init_cfg_banner():
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(REPO_ROOT), extensions=['jinja2.ext.do'])
    init_cfg = json.loads(env.get_template(INIT_CFG_TEMPLATE).render(installer_services=''))
    return init_cfg['BANNER_MESSAGE']['global']


def test_yang_login_default_matches_init_cfg(yang_model):
    leaf = next(yang_model.ctx.find_path(LOGIN_LEAF), None)
    assert leaf is not None, f'{LOGIN_LEAF} not found in the YANG models'
    assert leaf.default() == init_cfg_banner()['login']
