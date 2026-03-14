import os
import re

from bgpcfgd.template import TemplateFabric
from .util import load_constants_dir_mappings

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')


def parse_instance_conf(filename):
    activate_re = re.compile(r'^neighbor\s+(\S+)\s+activate$')
    with open(filename) as fp:
        lines = [line.strip() for line in fp if not line.strip().startswith('!') and line.strip() != '']
    # Search all v6 neighbors
    neighbors = {}
    for line in lines:
        if activate_re.match(line):
            neighbor = activate_re.match(line).group(1)
            if TemplateFabric.is_ipv6(neighbor):
                neighbors[neighbor] = {}
    # Extract peer-groups and route-maps
    for neighbor, neighbor_data in neighbors.items():
        route_map_in_re = re.compile(r'^neighbor\s+%s\s+route-map\s+(\S+) in$' % neighbor)
        peer_group_re = re.compile(r'^neighbor\s+%s\s+peer-group\s+(\S+)$' % neighbor)
        for line in lines:
            if route_map_in_re.match(line):
                assert "route-map" not in neighbor_data
                neighbor_data["route-map"] = route_map_in_re.match(line).group(1)
            if peer_group_re.match(line):
                assert "peer-group" not in neighbor_data
                neighbor_data["peer-group"] = peer_group_re.match(line).group(1)
    # Ensure that every ivp6 neighbor has either route-map or peer-group
    for neighbor, neighbor_data in neighbors.items():
        assert "route-map" in neighbor_data or "peer-group" in neighbor_data,\
            "IPv6 neighbor '%s' must have either route-map in or peer-group %s" % (neighbor, neighbor_data)
    return neighbors

def load_results(path, dir_name):
    result_files = []
    for fname in os.listdir(os.path.join(path, dir_name)):
        if not fname.startswith("result_"):
            continue
        full_fname = os.path.join(path, dir_name, fname)
        if not os.path.isfile(full_fname):
            continue
        result_files.append(full_fname)
    return result_files

def process_instances(path):
    result_files = load_results(path, "instance.conf")
    # search for ipv6 neighbors
    neighbors_list = []
    for fname in result_files:
        neighbors = parse_instance_conf(fname)
        if neighbors:
            neighbors_list.append(neighbors)
    return neighbors_list

def parse_peer_group_conf(filename, pg_name):
    route_map_re = re.compile(r'^neighbor\s+%s\s+route-map\s+(\S+)\s+in$' % pg_name)
    with open(filename) as fp:
        lines = [line.strip() for line in fp if not line.strip().startswith('!') and line.strip() != '']
    route_maps = set()
    for line in lines:
        if route_map_re.match(line):
            route_map = route_map_re.match(line).group(1)
            route_maps.add(route_map)
    return route_maps

def extract_rm_from_peer_group(path, peer_group_name):
    result_files = load_results(path, "peer-group.conf")
    rm_set = set()
    for fname in result_files:
        route_maps = parse_peer_group_conf(fname, peer_group_name)
        if route_maps:
            rm_set |= route_maps
    return list(rm_set)

def check_routemap_in_file(filename, route_map_name):
    route_map_re = re.compile(r'^route-map\s+%s\s+permit\s+(\d+)' % route_map_name)
    set_re = re.compile(r'set ipv6 next-hop prefer-global')
    next_re = re.compile(r'on-match next')
    with open(filename) as fp:
        lines = [line.strip() for line in fp if not line.strip().startswith('!') and line.strip() != '']
    found_entry = False
    found_seq_no = None
    route_map_entries = {}
    for line in lines:
        if found_entry:
            if set_re.match(line):
                route_map_entries[found_seq_no][0] = True
            elif next_re.match(line):
                route_map_entries[found_seq_no][1] = True
            else:
                found_entry = False
                found_seq_no = None
        if route_map_re.match(line):
            found_seq_no = None
            seq_n_txt = route_map_re.match(line).group(1)
            assert seq_n_txt.isdigit(), "wrong sequence number for line '%s'" % line
            found_seq_no = int(seq_n_txt)
            assert found_seq_no not in route_map_entries, "Route-map has duplicate entries: %s - %d" % (route_map_name, found_seq_no)
            found_entry = True
            route_map_entries[found_seq_no] = [False, False]
    results = [route_map_entries[seq] for seq in sorted(route_map_entries.keys())]
    err_msg = "route-map %s doesn't have mandatory permit entry for 'set ipv6 next-hop prefer-global" % route_map_name
    assert len(results), err_msg
    assert all(results[0]), "first ipv6 route-map entry doesn't have set ipv6 nexthop"
    return True

def check_routemap(path, route_map_name):
    result_files = load_results(path, "policies.conf")
    checked = False
    for fname in result_files:
        checked = checked or check_routemap_in_file(fname, route_map_name)
    assert checked, "route-map %s wasn't found" % route_map_name

def test_v6_next_hop_global():
    paths = ["tests/data/%s" % value for value in load_constants_dir_mappings().values()]
    for path in paths:
        test_cases = process_instances(path)
        for test_case in test_cases:
            for neighbor_value in test_case.values():
                if 'route-map' in neighbor_value:
                    check_routemap(path, neighbor_value['route-map'])
                elif 'peer-group' in neighbor_value:
                    route_map_in_list = extract_rm_from_peer_group(path, neighbor_value['peer-group'])
                    for route_map_in in route_map_in_list:
                        check_routemap(path, route_map_in)
