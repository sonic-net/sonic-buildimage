import ipaddress
import re


class TrafficShift:
    """Withdraw transit advertisements without resetting BGP sessions.

    While enabled, each IPv4/IPv6 unicast row in BGP_PEER_GROUP_AF and
    BGP_NEIGHBOR_AF is bound to a private wrapper instead of its outbound
    route-map:

        route-map FRRCFGD_TSA_<vrf>_<af>[_<policy>] permit 10
         match <ip|ipv6> address prefix-list FRRCFGD_TSA_<vrf>_<ip|ipv6>
         call <policy>

    The prefix-list holds the VRF's LOOPBACK_INTERFACE prefixes, so only
    loopbacks that the original policy also permits are advertised. A
    neighbor without its own policy keeps inheriting its peer-group's
    wrapper. bgpd.conf.db.tsa.j2 renders the same objects at startup.
    """

    AF_TABLES = ('BGP_PEER_GROUP_AF', 'BGP_NEIGHBOR_AF')
    PEER_TABLES = {'BGP_PEER_GROUP_AF': 'BGP_PEER_GROUP', 'BGP_NEIGHBOR_AF': 'BGP_NEIGHBOR'}
    PREFIX = 'FRRCFGD_TSA_'

    def __init__(self, config_db, run_command, validate_input):
        self.config_db = config_db
        self.run_command = run_command
        self.validate_input = validate_input
        self.enabled = False
        # Private objects known to be in FRR, so that stale ones can be removed.
        self.prefixes = set()    # {(protocol, prefix-list, prefix)}
        self.route_maps = set()  # {wrapper}
        self.bindings = {}       # {(table, key): (asn, wrapper)}

    def recover(self, running_config):
        """Load the private objects present in FRR, e.g. after a restart."""
        self.prefixes = set(re.findall(
            r'^(ip|ipv6) prefix-list (' + self.PREFIX + r'\S+) (?:seq \d+ )?permit (\S+)',
            running_config, re.MULTILINE))
        self.route_maps = set(re.findall(
            r'^route-map (' + self.PREFIX + r'\S+) ', running_config, re.MULTILINE))
        self.bindings = {}
        # Split into top-level blocks. Nested blocks, such as "segment-routing
        # srv6", are indented, so their "exit" stays inside the router block.
        for block in re.split(r'^(?=\S)', running_config, flags=re.MULTILINE):
            router = re.match(r'router bgp (\S+)(?: vrf (\S+))?', block)
            if not router:
                continue
            asn, vrf = router[1], router[2] or 'default'
            groups = re.findall(r'^ neighbor (\S+) peer-group$', block, re.MULTILINE)
            sections = re.findall(r'^ address-family (ipv4|ipv6) unicast$(.*?)^ exit-address-family$',
                                  block, re.MULTILINE | re.DOTALL)
            for af, section in sections:
                for peer, name in re.findall(r'^  neighbor (\S+) route-map (' + self.PREFIX + r'\S+) out$',
                                             section, re.MULTILINE):
                    table = 'BGP_PEER_GROUP_AF' if peer in groups else 'BGP_NEIGHBOR_AF'
                    self.bindings[(table, (vrf, peer, af + '_unicast'))] = (asn, name)
        self.enabled = bool(self.prefixes or self.route_maps or self.bindings)

    def apply(self, enabled):
        """Isolate (enabled) or restore advertisements to match CONFIG_DB.

        Returns False, keeping the recorded state, if CONFIG_DB has invalid
        input or FRR rejects a command.
        """
        if enabled and self._reserved_name_in_use():
            return False
        db = {table: self.config_db.get_table(table) for table in
              ('BGP_GLOBALS', 'BGP_NEIGHBOR', 'BGP_PEER_GROUP', 'LOOPBACK_INTERFACE') + self.AF_TABLES}
        prefixes = self._loopback_prefixes(db['LOOPBACK_INTERFACE']) if enabled else set()
        rows = self._unicast_rows(db)
        if prefixes is None or rows is None:
            return False

        commands = ['configure terminal']
        commands += ['no {} prefix-list {} permit {}'.format(*p) for p in sorted(self.prefixes - prefixes)]
        commands += ['{} prefix-list {} permit {}'.format(*p) for p in sorted(prefixes)]
        route_maps, bindings = set(), {}
        for table, key, asn, policy, inherits in rows:
            vrf, peer, af = key[0], key[1], key[2].lower().split('_')[0]
            wrapper = self.PREFIX + vrf + '_' + af + ('_' + policy if policy else '')
            if enabled and not inherits:
                if wrapper not in route_maps:
                    route_maps.add(wrapper)
                    protocol = 'ip' if af == 'ipv4' else 'ipv6'
                    commands += ['route-map {} permit 10'.format(wrapper),
                                 'match {0} address prefix-list {1}{2}_{0}'.format(protocol, self.PREFIX, vrf),
                                 'call {}'.format(policy) if policy else 'no call',
                                 'exit']
                bindings[(table, key)] = (asn, wrapper)
                line = 'neighbor {} route-map {} out'.format(peer, wrapper)
            elif policy:
                line = 'neighbor {} route-map {} out'.format(peer, policy)
            else:
                line = 'no neighbor {} route-map {} out'.format(peer, wrapper)
            commands += self._in_af(asn, vrf, af, line)

        # Detach rows deleted from CONFIG_DB before deleting their wrappers, but
        # do not recreate a peer that was deleted together with its row.
        for (table, key), (asn, wrapper) in self.bindings.items():
            vrf, peer, af_type = key
            if (key not in db[table] and (vrf, peer) in db[self.PEER_TABLES[table]] and
                    vrf in db['BGP_GLOBALS']):
                commands += self._in_af(asn, vrf, af_type.split('_')[0],
                                        'no neighbor {} route-map {} out'.format(peer, wrapper))
        commands += ['no route-map {}'.format(name) for name in sorted(self.route_maps - route_maps)]
        if not enabled:
            commands += ['no {} prefix-list {}'.format(protocol, name)
                         for protocol, name in sorted({(p, n) for p, n, _ in self.prefixes})]
        if not self.run_command(commands):
            return False
        self.enabled, self.prefixes, self.route_maps, self.bindings = enabled, prefixes, route_maps, bindings
        return True

    def _reserved_name_in_use(self):
        return any((key[0] if isinstance(key, tuple) else key).startswith(self.PREFIX)
                   for table in ('ROUTE_MAP', 'PREFIX_SET')
                   for key in self.config_db.get_table(table))

    def _loopback_prefixes(self, loopbacks):
        """Return {(protocol, prefix-list, prefix)}, or None for invalid input."""
        prefixes = set()
        for key in loopbacks:
            if not isinstance(key, tuple) or len(key) != 2:
                continue  # An interface row rather than an address.
            vrf = loopbacks.get(key[0], {}).get('vrf_name', 'default')
            try:
                network = ipaddress.ip_network(key[1], strict=False)
            except ValueError:
                return None
            if self.validate_input('BGP_GLOBALS', vrf, {}) is None:
                return None
            protocol = 'ip' if network.version == 4 else 'ipv6'
            prefixes.add((protocol, self.PREFIX + vrf + '_' + protocol, str(network)))
        return prefixes

    def _unicast_rows(self, db):
        """Return [(table, key, asn, policy, inherits)] for unicast AF rows.

        policy is the row's outbound route-map, if any; inherits is True for a
        neighbor that uses its peer-group's policy. Rows in a VRF without an
        ASN are skipped. Returns None for invalid input.
        """
        default_asn = self.config_db.get_entry('DEVICE_METADATA', 'localhost').get('bgp_asn')
        rows = []
        for table in self.AF_TABLES:
            for key, entry in db[table].items():
                if not isinstance(key, tuple) or len(key) != 3:
                    continue
                vrf, peer, af_type = key
                if af_type.lower() not in ('ipv4_unicast', 'ipv6_unicast'):
                    continue
                asn = db['BGP_GLOBALS'].get(vrf, {}).get('local_asn')
                if asn is None and vrf == 'default':
                    asn = default_asn
                if asn is None:
                    continue
                if (self.validate_input(table, '|'.join(key), entry) is None or
                        self.validate_input('BGP_GLOBALS', vrf, {'local_asn': asn}) is None):
                    return None
                policy = entry['route_map_out'][-1] if entry.get('route_map_out') else None
                group = db['BGP_NEIGHBOR'].get((vrf, peer), {}).get('peer_group_name')
                inherits = (table == 'BGP_NEIGHBOR_AF' and policy is None and
                            (vrf, group, af_type) in db['BGP_PEER_GROUP_AF'])
                rows.append((table, key, asn, policy, inherits))
        return rows

    @staticmethod
    def _in_af(asn, vrf, af, line):
        return ['router bgp {} vrf {}'.format(asn, vrf), 'address-family {} unicast'.format(af),
                line, 'exit-address-family', 'exit']
