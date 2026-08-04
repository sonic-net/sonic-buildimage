import itertools
import os
import re

from .util import load_constants_dir_mappings, load_constants


TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr/bgpd/templates')


def find_all_files(path):
    paths_to_check = [path]
    res = []
    while paths_to_check:
        path = paths_to_check[0]
        paths_to_check = paths_to_check[1:]
        for name in os.listdir(path):
            full_path = "%s/%s" % (path, name)
            if os.path.isfile(full_path):
                res.append(full_path)
            elif os.path.isdir(full_path):
                paths_to_check.append(full_path)
    return res

def get_files_to_check():
    directories = load_constants_dir_mappings()
    general_path = "%s/%s" % (TEMPLATE_PATH, directories['general'])
    files = find_all_files(general_path)
    return files

def get_peer_groups_with_bbr(filename):
    re_bbr = re.compile(r".+CONFIG_DB__BGP_BBR.+") #\['status'\] == 'enabled'")
    re_endif = re.compile(r'^\s*{% +endif +%}\s*$')
    re_peer = re.compile(r'^\s*neighbor\s+(\S+)\s+allowas-in\s+1\s*$')
    inside_bbr = False
    res = []
    with open(filename) as fp:
        for line in fp:
            s_line = line.strip()
            if s_line == '':
                continue
            elif s_line.startswith('!'):
                continue
            elif re_bbr.match(s_line):
                inside_bbr = True
            elif re_peer.match(s_line) and inside_bbr:
                m = re_peer.match(s_line)
                pg = m.group(1)
                res.append(pg)
            elif re_endif.match(s_line) and inside_bbr:
                inside_bbr = False
    return res

def load_constants_bbr():
    data = load_constants()
    assert "bgp" in data["constants"], "'bgp' key not found in constants.yml"
    assert "peers" in data["constants"]["bgp"], "'peers' key not found in constants.yml"
    assert "general" in data["constants"]["bgp"]['peers'], "'general' key not found in constants.yml"
    return data["constants"]["bgp"]["peers"]['general']

def test_bbr_templates():
    files_to_check = get_files_to_check()
    pg_with_bbr_per_file = [ get_peer_groups_with_bbr(name) for name in files_to_check ]
    pg_with_bbr = set(itertools.chain.from_iterable(pg_with_bbr_per_file))
    general = load_constants_bbr()
    if pg_with_bbr:
        assert 'bbr' in general, "BBR is not defined in 'general', but BBR is enabled for %s" % pg_with_bbr
        for pg in pg_with_bbr:
            assert pg in general['bbr'], "peer-group '%s' has BBR enabled, but it is not configured in constants.yml"

