#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Tests for `sonic_yang_models` package."""

import pytest

from os import system
from sys import exit

@pytest.fixture
def response():
    """Sample pytest fixture.

    See more at: http://doc.pytest.org/en/latest/fixture.html
    """
    # import requests
    # return requests.get('https://github.com/audreyr/cookiecutter-pypackage')

def test_content(response):

    """Sample pytest test function with the pytest fixture as an argument."""

def test_generate_yang_tree():

    # Generate YANG Tree, see no error in it.
    pyang_tree_cmd = "pyang -Vf tree -p /usr/local/share/yang/modules/ietf ./yang-models/*.yang > ./yang-models/sonic_yang_tree"
    if (system(pyang_tree_cmd)):
        print("Failed: {}".format(pyang_tree_cmd))
        system("pyang --version")
        system("env")
        system("ls -l /usr/local/share/yang/modules/ietf/")
        exit(1)
    else:
        print("Passed: {}".format(pyang_tree_cmd))

    return

    # from bs4 import BeautifulSoup
    # assert 'GitHub' in BeautifulSoup(response.content).title.string
