import ipaddress
import re


class TrafficShift:
    """Isolate unicast advertisements without changing user route-maps."""

    AF_TABLES = ('BGP_PEER_GROUP_AF', 'BGP_NEIGHBOR_AF')
    PREFIX = 'FRRCFGD_TSA_'

    def __init__(self, config_db, run_command, validate_input):
        self.config_db = config_db
        self.run_command = run_command
        self.validate_input = validate_input
        self.enabled = False
        self.route_maps = set()
        self.prefixes = set()
        self.bindings = {}

    def recover(self, running_config):
        """Recover private objects after a daemon restart or partial apply."""
        self.route_maps = set(re.findall(
            r'^route-map (' + self.PREFIX + r'\S+) ', running_config, re.MULTILINE))
        self.prefixes = set(re.findall(
            r'^(ip|ipv6) prefix-list (' + self.PREFIX + r'\S+) (?:seq \d+ )?permit (\S+)',
            running_config, re.MULTILINE))
        self.bindings = {}
        asn = vrf = af = None
        peer_groups = set()
        for line in running_config.splitlines():
            line = line.strip()
            if line.startswith('router '):
                router = re.fullmatch(r'router bgp (\S+)(?: vrf (\S+))?', line)
                asn = router[1] if router else None
                vrf = (router[2] or 'default') if router else None
                af = None
                peer_groups.clear()
            elif line == 'exit':
                asn = vrf = af = None
            elif asn is not None:
                group = re.fullmatch(r'neighbor (\S+) peer-group', line)
                family = re.fullmatch(r'address-family (ipv4|ipv6) unicast', line)
                binding = re.fullmatch(r'neighbor (\S+) route-map (' + self.PREFIX + r'\S+) out', line)
                if group:
                    peer_groups.add(group[1])
                elif line.startswith('address-family '):
                    af = family[1] + '_unicast' if family else None
                elif line == 'exit-address-family':
                    af = None
                elif af and binding:
                    peer, name = binding.groups()
                    table = 'BGP_PEER_GROUP_AF' if peer in peer_groups else 'BGP_NEIGHBOR_AF'
                    self.bindings[(table, (vrf, peer, af))] = (asn, name)
        self.enabled = bool(self.route_maps or self.prefixes or self.bindings)

    def apply(self, enabled):
        if enabled:
            for table in ('ROUTE_MAP', 'PREFIX_SET'):
                for key in self.config_db.get_table(table):
                    name = key[0] if isinstance(key, tuple) else key
                    if name.startswith(self.PREFIX):
                        return False
        commands = ['configure terminal']
        prefixes = set()
        route_maps = set()
        bindings = {}
        if enabled:
            loopbacks = self.config_db.get_table('LOOPBACK_INTERFACE')
            for key in loopbacks:
                if not isinstance(key, tuple) or len(key) != 2:
                    continue
                network = ipaddress.ip_network(key[1], strict=False)
                protocol = 'ip' if network.version == 4 else 'ipv6'
                vrf = loopbacks.get(key[0], {}).get('vrf_name', 'default')
                if self.validate_input('BGP_GLOBALS', vrf, {}) is None:
                    return False
                prefixes.add((protocol, self.PREFIX + vrf + '_' + protocol, str(network)))
            for protocol, name, prefix in sorted(self.prefixes - prefixes):
                commands.append('no {} prefix-list {} permit {}'.format(protocol, name, prefix))
            for protocol, name, prefix in sorted(prefixes):
                commands.append('{} prefix-list {} permit {}'.format(protocol, name, prefix))

        globals_table = self.config_db.get_table('BGP_GLOBALS')
        metadata_asn = self.config_db.get_entry('DEVICE_METADATA', 'localhost').get('bgp_asn')
        neighbors = self.config_db.get_table('BGP_NEIGHBOR')
        peer_af = self.config_db.get_table('BGP_PEER_GROUP_AF')
        af_tables = {'BGP_PEER_GROUP_AF': peer_af,
                     'BGP_NEIGHBOR_AF': self.config_db.get_table('BGP_NEIGHBOR_AF')}
        for table in self.AF_TABLES:
            for key, entry in af_tables[table].items():
                if not isinstance(key, tuple) or len(key) != 3:
                    continue
                vrf, peer, af_type = key
                if af_type.lower() not in ('ipv4_unicast', 'ipv6_unicast'):
                    continue
                asn = globals_table.get(vrf, {}).get('local_asn')
                if asn is None and vrf == 'default':
                    asn = metadata_asn
                if asn is None:
                    continue
                if (self.validate_input(table, '|'.join(key), entry) is None or
                        self.validate_input('BGP_GLOBALS', vrf, {'local_asn': asn}) is None):
                    return False
                original = entry.get('route_map_out', [])
                original = original[-1] if original else None
                # An AF row without an explicit policy inherits its peer-group.
                group = neighbors.get((vrf, peer), {}).get('peer_group_name')
                inherited = (table == 'BGP_NEIGHBOR_AF' and original is None and
                             (vrf, group, af_type) in peer_af)
                af = af_type.lower().split('_')[0]
                protocol = 'ip' if af == 'ipv4' else 'ipv6'
                name = self.PREFIX + vrf + '_' + af + ('_' + original if original else '')
                if enabled and not inherited:
                    route_maps.add(name)
                    bindings[(table, key)] = (asn, name)
                    commands += ['route-map {} permit 10'.format(name),
                                 'match {} address prefix-list {}{}_{}'.format(
                                     protocol, self.PREFIX, vrf, protocol)]
                    commands.append('call {}'.format(original) if original else 'no call')
                    commands.append('exit')
                commands += ['router bgp {} vrf {}'.format(asn, vrf),
                             'address-family {} unicast'.format(af)]
                if enabled and not inherited:
                    commands.append('neighbor {} route-map {} out'.format(peer, name))
                elif original:
                    commands.append('neighbor {} route-map {} out'.format(peer, original))
                else:
                    commands.append('no neighbor {} route-map {} out'.format(peer, name))
                commands += ['exit-address-family', 'exit']

        for (table, key), (asn, name) in self.bindings.items():
            if key in af_tables[table]:
                continue
            vrf, peer, af_type = key
            # Do not recreate a neighbor that was deleted along with its AF row.
            peer_table = 'BGP_PEER_GROUP' if table == 'BGP_PEER_GROUP_AF' else 'BGP_NEIGHBOR'
            if ((vrf, peer) in self.config_db.get_table(peer_table) and
                    vrf in globals_table):
                commands += ['router bgp {} vrf {}'.format(asn, vrf),
                             'address-family {} unicast'.format(af_type.split('_')[0]),
                             'no neighbor {} route-map {} out'.format(peer, name),
                             'exit-address-family', 'exit']

        for name in sorted(self.route_maps - route_maps):
            commands.append('no route-map {}'.format(name))
        if not enabled:
            for protocol, name in sorted({(p, n) for p, n, _ in self.prefixes}):
                commands.append('no {} prefix-list {}'.format(protocol, name))
        if not self.run_command(commands):
            return False
        self.enabled = enabled
        self.prefixes = prefixes
        self.route_maps = route_maps
        self.bindings = bindings
        return True
