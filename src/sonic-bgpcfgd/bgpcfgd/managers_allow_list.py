"""
Implementation of "allow-list" feature
"""
import re
import ipaddress

from .log import log_debug, log_info, log_err, log_warn
from .template import TemplateFabric
from .manager import Manager


class BGPAllowListMgr(Manager):
    """ This class initialize "AllowList" settings """
    ALLOW_ADDRESS_PL_NAME_TMPL = "ALLOW_ADDRESS_%d_%s"  # template for a name for the ALLOW_ADDRESS prefix-list ???
    EMPTY_COMMUNITY = "empty"
    PL_NAME_TMPL = "PL_ALLOW_LIST_DEPLOYMENT_ID_%d_COMMUNITY_%s_V%s"
    PL_NAME_TMPL_WITH_NEIGH = "PL_ALLOW_LIST_DEPLOYMENT_ID_%d_NEIGHBOR_%s_COMMUNITY_%s_V%s"
    COMMUNITY_NAME_TMPL = "COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_%d_COMMUNITY_%s"
    COMMUNITY_NAME_TMPL_WITH_NEIGH = "COMMUNITY_ALLOW_LIST_DEPLOYMENT_ID_%d_NEIGHBOR_%s_COMMUNITY_%s"
    RM_NAME_TMPL = "ALLOW_LIST_DEPLOYMENT_ID_%d_V%s"
    RM_NAME_TMPL_WITH_NEIGH = "ALLOW_LIST_DEPLOYMENT_ID_%d_NEIGHBOR_%s_V%s"
    ROUTE_MAP_ENTRY_WITH_COMMUNITY_START = 10
    ROUTE_MAP_ENTRY_WITH_COMMUNITY_END = 29990
    ROUTE_MAP_ENTRY_WITHOUT_COMMUNITY_START = 30000
    ROUTE_MAP_ENTRY_WITHOUT_COMMUNITY_END = 65530
    PREFIX_LIST_POS = 1 # the position of the ip prefix in the permit string.

    V4 = "v4"  # constant for af enum: V4
    V6 = "v6"  # constant for af enum: V6

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(BGPAllowListMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )
        self.key_re = re.compile(r"^DEPLOYMENT_ID\|\d+\|\S+$|^DEPLOYMENT_ID\|\d+$|^DEPLOYMENT_ID\|\d+\|\S+\|NEIGHBOR_TYPE\|\S+$|^DEPLOYMENT_ID\|\d+\|NEIGHBOR_TYPE\|\S+")
        self.enabled = self.__get_enabled()
        self.prefix_match_tag = self.__get_routemap_tag()
        self.__load_constant_lists()

    def set_handler(self, key, data):
        """
        Manager method which runs on receiving 'SET' message
        :param key: ket of the 'SET' message
        :param data: data of the 'SET' message
        :return: True if the message was executed, False - the message should be postponed.
        """
        if not self.enabled:
            log_warn("BGPAllowListMgr::Received 'SET' command, but this feature is disabled in constants")
            return True
        if not self.__set_handler_validate(key, data):
            return True
        if 'NEIGHBOR_TYPE' in key:
            keys = key.split('|NEIGHBOR_TYPE|', 1)
            deployment_id = keys[0].replace("DEPLOYMENT_ID|", "")
            neighbor_type, community_value = keys[1].split('|', 1) if '|' in keys[1] else (keys[1], BGPAllowListMgr.EMPTY_COMMUNITY)
        else:
            key = key.replace("DEPLOYMENT_ID|", "")
            deployment_id, community_value = key.split('|', 1) if '|' in key else (key, BGPAllowListMgr.EMPTY_COMMUNITY)
            neighbor_type = ''
        deployment_id = int(deployment_id)
        prefixes_v4 = []
        prefixes_v6 = []
        if "prefixes_v4" in data:
            prefixes_v4 = str(data['prefixes_v4']).split(",")
        if "prefixes_v6" in data:
            prefixes_v6 = str(data['prefixes_v6']).split(",")
        default_action_community = self.__get_default_action_community(data)
        self.__update_policy(deployment_id, community_value, prefixes_v4, prefixes_v6, default_action_community, neighbor_type)
        return True

    def __set_handler_validate(self, key, data):
        """
        Validate parameters of a "Set" message
        :param key: ket of the 'SET' message
        :param data: data of the 'SET' message
        :return: True if parameters are valid, False if parameters are invalid
        """
        if data is None:
            log_err("BGPAllowListMgr::Received BGP ALLOWED 'SET' message without data")
            return False
        if not self.key_re.match(key):
            log_err("BGPAllowListMgr::Received BGP ALLOWED 'SET' message with invalid key: '%s'" % key)
            return False
        prefixes_v4 = []
        prefixes_v6 = []
        if "prefixes_v4" in data:
            prefixes_v4 = str(data["prefixes_v4"]).split(",")
            if not all(TemplateFabric.is_ipv4(re.split('ge|le', prefix)[0]) for prefix in prefixes_v4):
                arguments = "prefixes_v4", str(data["prefixes_v4"])
                log_err("BGPAllowListMgr::Received BGP ALLOWED 'SET' message with invalid input[%s]:'%s'" % arguments)
                return False
        if "prefixes_v6" in data:
            prefixes_v6 = str(data["prefixes_v6"]).split(",")
            if not all(TemplateFabric.is_ipv6(re.split('ge|le', prefix)[0]) for prefix in prefixes_v6):
                arguments = "prefixes_v6", str(data["prefixes_v6"])
                log_err("BGPAllowListMgr::Received BGP ALLOWED 'SET' message with invalid input[%s]:'%s'" % arguments)
                return False
        if not prefixes_v4 and not prefixes_v6:
            log_err("BGPAllowListMgr::Received BGP ALLOWED 'SET' message with no prefixes specified: %s" % str(data))
            return False
        if "default_action" in data and data["default_action"] != "permit" and data["default_action"] != "deny":
            log_err("BGPAllowListMgr::Received BGP ALLOWED 'SET' message with invalid 'default_action' field: '%s'" % str(data))
            return False
        return True

    def del_handler(self, key):
        """
        Manager method which runs on "DEL" message
        :param key: a key of "DEL" message
        """
        if not self.enabled:
            log_warn("BGPAllowListMgr::Received 'DEL' command, but this feature is disabled in constants")
            return
        if not self.__del_handler_validate(key):
            return

        if 'NEIGHBOR_TYPE' in key:
            keys = key.split('|NEIGHBOR_TYPE|', 1)
            deployment_id = keys[0].replace("DEPLOYMENT_ID|", "")
            neighbor_type, community_value = keys[1].split('|', 1) if '|' in keys[1] else (keys[1], BGPAllowListMgr.EMPTY_COMMUNITY)
        else:
            key = key.replace("DEPLOYMENT_ID|", "")
            deployment_id, community_value = key.split('|', 1) if '|' in key else (key, BGPAllowListMgr.EMPTY_COMMUNITY)
            neighbor_type = ''

        deployment_id = int(deployment_id)
        self.__remove_policy(deployment_id, community_value, neighbor_type)

    def __del_handler_validate(self, key):
        """
        Validate "DEL" method parameters
        :param key: a key of "DEL" message
        :return: True if parameters are valid, False if parameters are invalid
        """
        if not self.key_re.match(key):
            log_err("BGPAllowListMgr::Received BGP ALLOWED 'DEL' message with invalid key: '$s'" % key)
            return False
        return True

    def __update_policy(self, deployment_id, community_value, prefixes_v4, prefixes_v6, default_action, neighbor_type):
        """
        Update "allow list" policy with parameters
        :param deployment_id: deployment id which policy will be changed
        :param community_value: community value to match for the updated policy
        :param prefixes_v4: a list of v4 prefixes for the updated policy
        :param prefixes_v6: a list of v6 prefixes for the updated policy
        :param default_action: the default action for the policy. should be either 'permit' or 'deny'
        """
        # update all related entries with the information
        info = deployment_id, community_value, str(prefixes_v4), str(prefixes_v6), neighbor_type
        msg = "BGPAllowListMgr::Updating 'Allow list' policy."
        msg += " deployment_id '%s'. community: '%s'"
        msg += " prefix_v4 '%s'. prefix_v6: '%s'"
        msg += " neighbor_type %s"
        log_info(msg % info)
        names = self.__generate_names(deployment_id, community_value, neighbor_type)
        self.cfg_mgr.update()
        cmds = []
        cmds += self.__update_prefix_list(self.V4, names['pl_v4'], prefixes_v4)
        cmds += self.__update_prefix_list(self.V6, names['pl_v6'], prefixes_v6)
        cmds += self.__update_community(names['community'], community_value)
        cmds += self.__update_allow_route_map_entry(self.V4, names['pl_v4'], names['community'], names['rm_v4'])
        cmds += self.__update_allow_route_map_entry(self.V6, names['pl_v6'], names['community'], names['rm_v6'])
        cmds += self.__update_default_route_map_entry(names['rm_v4'], default_action)
        cmds += self.__update_default_route_map_entry(names['rm_v6'], default_action)
        if cmds:
            self.cfg_mgr.push_list(cmds)
            peer_groups = self.__find_peer_group(deployment_id, neighbor_type)
            self.cfg_mgr.restart_peer_groups(peer_groups)
            log_debug("BGPAllowListMgr::__update_policy. The peers configuration scheduled for updates")
        else:
            log_debug("BGPAllowListMgr::__update_policy. Nothing to update")
        log_info("BGPAllowListMgr::Done")

    def __remove_policy(self, deployment_id, community_value, neighbor_type):
        """
        Remove "allow list" policy for given deployment_id and community_value
        :param deployment_id: deployment id which policy will be removed
        :param community_value: community value to match for the removed policy
        """
        # remove all related entries from the configuration
        # put default rule to the route-map
        info = deployment_id, community_value
        msg = "BGPAllowListMgr::Removing 'Allow list' policy."
        msg += " deployment_id '%s'. community: '%s'"
        log_info(msg % info)

        default_action = self.__get_default_action_community()
        names = self.__generate_names(deployment_id, community_value, neighbor_type)
        self.cfg_mgr.update()
        cmds = []
        cmds += self.__remove_allow_route_map_entry(self.V4, names['pl_v4'], names['community'], names['rm_v4'])
        cmds += self.__remove_allow_route_map_entry(self.V6, names['pl_v6'], names['community'], names['rm_v6'])
        cmds += self.__remove_prefix_list(self.V4, names['pl_v4'])
        cmds += self.__remove_prefix_list(self.V6, names['pl_v6'])
        cmds += self.__remove_community(names['community'])
        cmds += self.__update_default_route_map_entry(names['rm_v4'], default_action)
        cmds += self.__update_default_route_map_entry(names['rm_v6'], default_action)
        if cmds:
            self.cfg_mgr.push_list(cmds)
            peer_groups = self.__find_peer_group(deployment_id, neighbor_type)
            self.cfg_mgr.restart_peer_groups(peer_groups)
            log_debug("BGPAllowListMgr::__remove_policy. 'Allow list' policy was scheduled for removal")
        else:
            log_debug("BGPAllowListMgr::__remove_policy. Nothing to remove")
        log_info('BGPAllowListMgr::Done')

    @staticmethod
    def __generate_names(deployment_id, community_value, neighbor_type):
        """
        Generate prefix-list names for a given peer_ip and community value
        :param deployment_id: deployment_id for which we're going to filter prefixes
        :param community_value: community, which we want to use to filter prefixes
        :return: a dictionary with names
        """
        if not neighbor_type:
            if community_value == BGPAllowListMgr.EMPTY_COMMUNITY:
                community_name = BGPAllowListMgr.EMPTY_COMMUNITY
            else:
                community_name = BGPAllowListMgr.COMMUNITY_NAME_TMPL % (deployment_id, community_value)
            names = {
                "pl_v4": BGPAllowListMgr.PL_NAME_TMPL % (deployment_id, community_value, '4'),
                "pl_v6": BGPAllowListMgr.PL_NAME_TMPL % (deployment_id, community_value, '6'),
                "rm_v4": BGPAllowListMgr.RM_NAME_TMPL % (deployment_id, '4'),
                "rm_v6": BGPAllowListMgr.RM_NAME_TMPL % (deployment_id, '6'),
                "community": community_name,
                'neigh_type': neighbor_type,
            }
            arguments = deployment_id, community_value, str(names)
            log_debug("BGPAllowListMgr::__generate_names. deployment_id: %d, community: %s. names: %s" % arguments)
        else:
            if community_value == BGPAllowListMgr.EMPTY_COMMUNITY:
                community_name = BGPAllowListMgr.EMPTY_COMMUNITY
            else:
                community_name = BGPAllowListMgr.COMMUNITY_NAME_TMPL_WITH_NEIGH % (deployment_id, neighbor_type, community_value)
            names = {
                "pl_v4": BGPAllowListMgr.PL_NAME_TMPL_WITH_NEIGH % (deployment_id, neighbor_type, community_value, '4'),
                "pl_v6": BGPAllowListMgr.PL_NAME_TMPL_WITH_NEIGH % (deployment_id, neighbor_type, community_value, '6'),
                "rm_v4": BGPAllowListMgr.RM_NAME_TMPL_WITH_NEIGH % (deployment_id, neighbor_type, '4'),
                "rm_v6": BGPAllowListMgr.RM_NAME_TMPL_WITH_NEIGH % (deployment_id, neighbor_type, '6'),
                "community": community_name,
            }
            arguments = deployment_id, neighbor_type, community_value, str(names)
            log_debug("BGPAllowListMgr::__generate_names. deployment_id: %d, neighbor_type: %s, community: %s. names: %s" % arguments)
        return names

    def __update_prefix_list(self, af, pl_name, allow_list):
        """
        Create or update a prefix-list with name pl_name.
        :param af: "v4" to create ipv4 prefix-list, "v6" to create ipv6 prefix-list
        :param pl_name: prefix-list name
        :param allow_list: prefix-list entries
        :return: True if updating was successful, False otherwise
        """
        assert af == self.V4 or af == self.V6
        constant_list = self.__get_constant_list(af)
        allow_list = self.__to_prefix_list(af, allow_list)
        log_debug("BGPAllowListMgr::__update_prefix_list. af='%s' prefix-list name=%s" % (af, pl_name))
        '''
            Need to check exist and equality of the allowed prefix list.
            A. If exist and equal, no operation needed.
            B. If exist but not equal, first delete then add prefix based on the data from condig db and constants.
            C. If non-exist, directly add prefix based on the data from condig db and constants.
        '''
        exist, correct = self.__is_prefix_list_valid(af, pl_name, allow_list, constant_list)
        if correct:
            log_debug("BGPAllowListMgr::__update_prefix_list. the prefix-list '%s' exists and correct" % pl_name)
            return []
        family = self.__af_to_family(af)
        cmds = []
        seq_no = 10
        if exist:
            cmds.append('no %s prefix-list %s' % (family, pl_name))
        for entry in self.__normalize_ipnetwork(af, constant_list + allow_list):
            cmds.append('%s prefix-list %s seq %d %s' % (family, pl_name, seq_no, entry))
            seq_no += 10
        return cmds

    def __remove_prefix_list(self, af, pl_name):
        """
        Remove prefix-list in the address-family af.
        :param af: "v4" to create ipv4 prefix-list, "v6" to create ipv6 prefix-list
        :param pl_name: list of prefix-list names
        :return: True if operation was successful, False otherwise
        """
        assert af == self.V4 or af == self.V6
        log_debug("BGPAllowListMgr::__remove_prefix_lists. af='%s' pl_names='%s'" % (af, pl_name))
        exist, _ = self.__is_prefix_list_valid(af, pl_name, [], [])
        if not exist:
            log_debug("BGPAllowListMgr::__remove_prefix_lists: prefix_list '%s' not found" % pl_name)
            return []
        family = self.__af_to_family(af)
        return ["no %s prefix-list %s" % (family, pl_name)]

    def __normalize_ipnetwork(self, af, allow_prefix_list):
        '''
            Normalize IPv6 addresses
                for example:
                    2001:cdba:0000:0000:0000:0000:3257:9652
                    2001:cdba:0:0:0:0:3257:9652
                    2001:cdba::3257:9652
                after normalize, all would be normalized to
                    2001:cdba::3257:9652
        '''
        normalize_list = []
        for allow_item in allow_prefix_list:
            tmp_list = allow_item.split(' ')
            if af == self.V6:
                tmp_list[self.PREFIX_LIST_POS] = str(ipaddress.IPv6Network(tmp_list[self.PREFIX_LIST_POS]))
            normalize_list.append(' '.join(tmp_list))
        return normalize_list

    def __is_prefix_list_valid(self, af, pl_name, allow_list, constant_list):
        """
        Check that a prefix list exists and it has valid entries
        :param af: address family of the checked prefix-list
        :param pl_name: prefix-list name
        :param allow_list: a prefix-list which must be a part of the valid prefix list
        :param constant_list: a constant list which must be on top of each "allow" prefix list on the device
        :return: a tuple. The first element of the tuple has True if the prefix-list exists, False otherwise,
                 The second element of the tuple has True if allow prefix list in running configuraiton is
                 equal with ones in config db + constants, False if not
        """
        assert af == self.V4 or af == self.V6
        family = self.__af_to_family(af)
        match_string = '%s prefix-list %s seq ' % (family, pl_name)
        conf = self.cfg_mgr.get_text()
        if not any(line.strip().startswith(match_string) for line in conf):
            return False, False  # if the prefix list is not exists, it is not correct
        expect_set = set(self.__normalize_ipnetwork(af, constant_list))
        expect_set.update(set(self.__normalize_ipnetwork(af, allow_list)))

        config_list = []
        for line in conf:
            if line.startswith(match_string):
                found = line[len(match_string):].strip().split(' ')
                rule = " ".join(found[1:])
                config_list.append(rule)

        # Return double Ture, when running configuraiton is identical with config db + constants.
        return True, expect_set == set(self.__normalize_ipnetwork(af, config_list))

    def __update_community(self, community_name, community_value):
        """
        Update community for a peer
        :param community_name: name of the community to update
        :param community_value: community value for the peer
        :return: True if operation was successful, False otherwise
        """
        log_debug("BGPAllowListMgr::__update_community. community_name='%s' community='%s'" % (community_name, community_value))
        if community_value == self.EMPTY_COMMUNITY:  # we don't need to do anything for EMPTY community
            log_debug("BGPAllowListMgr::__update_community. Empty community. exiting")
            return []
        cmds = []
        exists, found_community_value = self.__is_community_presented(community_name)
        if exists:
            if community_value == found_community_value:
                log_debug("BGPAllowListMgr::__update_community. community '%s' is already presented" % community_name)
                return []
            else:
                msg = "BGPAllowListMgr::__update_community. "
                msg += "community '%s' is already presented, but community value should be updated" % community_name
                log_debug(msg)
                cmds.append("no bgp community-list standard %s" % community_name)
        cmds.append('bgp community-list standard %s permit %s' % (community_name, community_value))
        return cmds

    def __remove_community(self, community_name):
        """
        Remove community for a peer
        :param community_name: community value for the peer
        :return: True if operation was successful, False otherwise
        """
        log_debug("BGPAllowListMgr::__remove_community. community='%s'" % community_name)
        if community_name == self.EMPTY_COMMUNITY:  # we don't need to do anything for EMPTY community
            log_debug("BGPAllowListMgr::__remove_community. There is nothing to remove in empty community")
            return []
        exists, _ = self.__is_community_presented(community_name)
        if not exists:
            log_debug("BGPAllowListMgr::__remove_community. Community is already removed.")
            return []
        return ['no bgp community-list standard %s' % community_name]

    def __is_community_presented(self, community_name):
        """
        Return True if community for the peer_ip exists
        :param community_name: community value for the peer
        :return: A tuple. First element: True if operation was successful, False otherwise
                          Second element: community value if the first element is True no value otherwise
        """
        log_debug("BGPAllowListMgr::__is_community_presented. community='%s'" % community_name)
        match_string = 'bgp community-list standard %s permit ' % community_name
        conf = self.cfg_mgr.get_text()
        found = [line.strip() for line in conf if line.strip().startswith(match_string)]
        if not found:
            return False, None
        community_value = found[0].replace(match_string, '')
        return True, community_value

    def __update_allow_route_map_entry(self, af, allow_address_pl_name, community_name, route_map_name):
        """
        Add or update a "Allow address" route-map entry with the parameters
        :param af: "v4" to create ipv4 prefix-list, "v6" to create ipv6 prefix-list
        :return: True if operation was successful, False otherwise
        """
        assert af == self.V4 or af == self.V6
        info = af, route_map_name, allow_address_pl_name, community_name
        log_debug("BGPAllowListMgr::__update_allow_route_map_entry. af='%s' Allow rm='%s' pl='%s' cl='%s'" % info)
        entries = self.__parse_allow_route_map_entries(af, route_map_name)
        found, _ = self.__find_route_map_entry(entries, allow_address_pl_name, community_name)
        if found:
            log_debug("BGPAllowListMgr::__update_allow_route_map_entry. route-map='%s' is already found" % route_map_name)
            return []
        seq_number = self.__find_next_seq_number(entries.keys(), community_name != self.EMPTY_COMMUNITY, route_map_name)
        info = af, seq_number, allow_address_pl_name, community_name
        out = "af='%s' seqno='%d' Allow pl='%s' cl='%s'" % info
        log_debug("BGPAllowListMgr::__update_allow_route_map_entry. %s" % out)
        ip_version = "" if af == self.V4 else "v6"
        cmds = [
            'route-map %s permit %d' % (route_map_name, seq_number),
            ' match ip%s address prefix-list %s' % (ip_version, allow_address_pl_name)
        ]
        if not community_name.endswith(self.EMPTY_COMMUNITY):
            cmds.append(" match community %s" % community_name)
        elif self.prefix_match_tag:
            cmds.append(" set tag %s" % self.prefix_match_tag)
        return cmds

    def __update_default_route_map_entry(self, route_map_name, default_action_community):
        """
        Add or update default action rule for the route-map.
        Default action rule is hardcoded into route-map permit 65535
        :param route_map_name: name of the target route_map
        :param default_action_community: community value to mark not-matched prefixes
        """
        info = route_map_name, default_action_community
        log_debug("BGPAllowListMgr::__update_default_route_map_entry. rm='%s' set_community='%s'" % info)
        current_default_action_value = self.__parse_default_action_route_map_entry(route_map_name)
        if current_default_action_value != default_action_community:
            return [
                'route-map %s permit 65535' % route_map_name,
                ' set community %s additive' % default_action_community
            ]
        else:
            return []

    def __parse_default_action_route_map_entry(self, route_map_name):
        """
        Parse default-action route-map entry
        :param route_map_name: Name of the route-map to parse
        :return: a community value used for default action
        """
        log_debug("BGPAllowListMgr::__parse_default_action_route_map_entries. rm='%s'" % route_map_name)
        match_string = 'route-map %s permit 65535' % route_map_name
        match_community = re.compile(r'^set community (\S+) additive$')
        inside_route_map = False
        community_value = ""
        conf = self.cfg_mgr.get_text()
        for line in conf + [""]:
            s_line = line.strip()
            if inside_route_map:
                matched = match_community.match(s_line)
                if matched:
                    community_value = matched.group(1)
                    break
                else:
                    log_err("BGPAllowListMgr::Found incomplete route-map '%s' entry. seq_no=65535" % route_map_name)
                inside_route_map = False
            elif s_line == match_string:
                inside_route_map = True
        if community_value == "":
            log_err("BGPAllowListMgr::Default action community value is not found. route-map '%s' entry. seq_no=65535" % route_map_name)
        return community_value

    def __remove_allow_route_map_entry(self, af, allow_address_pl_name, community_name, route_map_name):
        """
        Add or update a "Allow address" route-map entry with the parameters
        :param af: "v4" to create ipv4 prefix-list, "v6" to create ipv6 prefix-list
        :return: True if operation was successful, False otherwise
        """
        assert af == self.V4 or af == self.V6
        info = af, route_map_name, allow_address_pl_name, community_name
        log_debug("BGPAllowListMgr::__update_allow_route_map_entry. af='%s' Allow rm='%s' pl='%s' cl='%s'" % info)
        entries = self.__parse_allow_route_map_entries(af, route_map_name)
        found, seq_number = self.__find_route_map_entry(entries, allow_address_pl_name, community_name)
        if not found:
            log_debug("BGPAllowListMgr::__update_allow_route_map_entry. Not found route-map '%s' entry" % allow_address_pl_name)
            return []
        return ['no route-map %s permit %d' % (route_map_name, seq_number)]

    @staticmethod
    def __find_route_map_entry(entries, allow_address_pl_name, community_name):
        """
        Find route-map entry with given allow_address prefix list name and community name in the parsed route-map.
        :param entries: entries of parsed route-map
        :param allow_address_pl_name: name of the "allow address" prefix-list
        :param community_name: name of the "allow address" community name
        :return: a tuple. The first element of the tuple is True, if the route-map entry was found, False otherwise.
                          The second element of the tuple has a sequence number of the entry.
        """
        for sequence_number, values in entries.items():
            if sequence_number == 65535:
                continue
            allow_list_presented = values['pl_allow_list'] == allow_address_pl_name
            community_presented = values['community'] == community_name
            if allow_list_presented and community_presented:
                log_debug("BGPAllowListMgr::__find_route_map_entry. found route-map '%s' entry" % allow_address_pl_name)
                return True, sequence_number
        return False, None

    def __parse_allow_route_map_entries(self, af, route_map_name):
        """
        Parse "Allow list" route-map entries.
        :param af: "v4" to create ipv4 prefix-list, "v6" to create ipv6 prefix-list
        :return: A tuple, First element: True if operation was successful, False otherwise
                          Second element: list of object with parsed route-map entries
        """
        assert af == self.V4 or af == self.V6
        log_debug("BGPAllowListMgr::__parse_allow_route_map_entries. af='%s', rm='%s'" % (af, route_map_name))
        match_string = 'route-map %s permit ' % route_map_name
        entries = {}
        inside_route_map = False
        route_map_seq_number = None
        pl_allow_list_name = None
        community_name = self.EMPTY_COMMUNITY
        if af == self.V4:
            match_pl_allow_list = 'match ip address prefix-list '
        else:  # self.V6
            match_pl_allow_list = 'match ipv6 address prefix-list '
        match_community = 'match community '
        conf = self.cfg_mgr.get_text()
        for line in conf + [""]:
            if inside_route_map:
                if line.strip().startswith(match_pl_allow_list):
                    pl_allow_list_name = line.strip()[len(match_pl_allow_list):]
                    continue
                elif line.strip().startswith(match_community):
                    community_name = line.strip()[len(match_community):]
                    continue
                else:
                    if pl_allow_list_name is not None:
                        entries[route_map_seq_number] = {
                            'pl_allow_list': pl_allow_list_name,
                            'community': community_name,
                        }
                    else:
                        if route_map_seq_number != 65535:
                            log_warn("BGPAllowListMgr::Found incomplete route-map '%s' entry. seq_no=%d" % (route_map_name, route_map_seq_number))
                    inside_route_map = False
                    pl_allow_list_name = None
                    community_name = self.EMPTY_COMMUNITY
                    route_map_seq_number = None
            if line.startswith(match_string):
                found = line[len(match_string):]
                assert found.isdigit()
                route_map_seq_number = int(found)
                inside_route_map = True
        return entries

    @staticmethod
    def __find_next_seq_number(seq_numbers, has_community, route_map_name):
        """
        Find a next available "Allow list" route-map entry number
        :param seq_numbers: a list of already used sequence numbers
        :param has_community: True, if the route-map entry has community
        :return: next available route-map sequence number
        """
        used_sequence_numbers = set(seq_numbers)
        sequence_number = None
        if has_community:  # put entries without communities after 29999
            start_seq = BGPAllowListMgr.ROUTE_MAP_ENTRY_WITH_COMMUNITY_START
            end_seq = BGPAllowListMgr.ROUTE_MAP_ENTRY_WITH_COMMUNITY_END
        else:
            start_seq = BGPAllowListMgr.ROUTE_MAP_ENTRY_WITHOUT_COMMUNITY_START
            end_seq = BGPAllowListMgr.ROUTE_MAP_ENTRY_WITHOUT_COMMUNITY_END
        for i in range(start_seq, end_seq, 10):
            if i not in used_sequence_numbers:
                sequence_number = i
                break
        if sequence_number is None:
            raise RuntimeError("No free sequence numbers for '%s'" % route_map_name)
        info = sequence_number, "yes" if has_community else "no"
        log_debug("BGPAllowListMgr::__find_next_seq_number '%d' has_community='%s'" % info)
        return sequence_number

    def __extract_peer_group_names(self):
        """
        Extract names of all peer-groups defined in the config
        :return: list of peer-group names
        """
        # Find all peer-groups entries
        re_peer_group = re.compile(r'^\s*neighbor (\S+) peer-group$')
        peer_groups = []
        for line in self.cfg_mgr.get_text():
            result = re_peer_group.match(line)
            if result:
                peer_groups.append(result.group(1))
        return peer_groups

    def __get_peer_group_to_route_map(self, peer_groups):
        """
        Extract names of route-maps which is connected to peer-groups defines as peer_groups
        :peer_groups: a list of peer-group names
        :return: dictionary where key is a peer-group, value is a route-map name which is defined as route-map in
                 for the peer_group.
        """
        pg_2_rm = {}
        for pg in peer_groups:
            re_peer_group_rm = re.compile(r'^\s*neighbor %s route-map (\S+) in$' % pg)
            for line in self.cfg_mgr.get_text():
                result = re_peer_group_rm.match(line)
                if result:
                    pg_2_rm[pg] = result.group(1)
                    break
        return pg_2_rm

    def __get_route_map_calls(self, rms):
        """
        Find mapping between route-maps and route-map call names, defined for the route-maps
        :rms: a set with route-map names
        :return: a dictionary: key - name of a route-map, value - name of a route-map call defined for the route-map
        """
        rm_2_call = {}
        re_rm = re.compile(r'^route-map (\S+) permit \d+$')
        re_call = re.compile(r'^\s*call (\S+)$')
        inside_name = None
        for line in self.cfg_mgr.get_text():
            if inside_name:
                inside_result = re_call.match(line)
                if inside_result:
                    rm_2_call[inside_name] = inside_result.group(1)
                    inside_name = None
                    continue
            result = re_rm.match(line)
            if not result:
                continue
            inside_name = None
            if result.group(1) not in rms:
                continue
            inside_name = result.group(1)
        return rm_2_call

    def __get_routemap_tag(self):
        """
        Find if any user define tag is provided to be used when allow prefifx list is matched
        :return: string: prefix mix tag if define in constants.yml else None
        """
        prefix_match_tag = None
        if 'bgp' in self.constants and \
           'allow_list' in self.constants["bgp"] and \
           'prefix_match_tag' in \
           self.constants["bgp"]["allow_list"]:
           prefix_match_tag = \
              self.constants["bgp"]["allow_list"]["prefix_match_tag"]
        return prefix_match_tag

    @staticmethod
    def __get_peer_group_to_restart(deployment_id, pg_2_rm, rm_2_call, neighbor_type):
        """
        Get peer_groups which are assigned to deployment_id
        :deployment_id: deployment_id number
        :pg_2_rm: a dictionary where key is a peer-group, value is a route-map name which is defined as route-map in
                  for the peer_group.
        :rm_2_call: a dictionary: key - name of a route-map, value - name of a route-map call defined for the route-map
        """
        ret = set()
        if not neighbor_type:
            target_allow_list_prefix = 'ALLOW_LIST_DEPLOYMENT_ID_%d_V' % deployment_id
        else:
            target_allow_list_prefix = 'ALLOW_LIST_DEPLOYMENT_ID_%d_NEIGHBOR_%s_V' % (deployment_id, neighbor_type)
        for peer_group, route_map in pg_2_rm.items():
            if route_map in rm_2_call:
                if rm_2_call[route_map].startswith(target_allow_list_prefix):
                    ret.add(peer_group)
        return list(ret)

    def __find_peer_group(self, deployment_id, neighbor_type):
        """
        Deduce peer-group names which are connected to devices with requested deployment_id
        :param deployment_id: deployment_id number
        :return: a list of peer-groups which a used by devices with requested deployment_id number
        """
        self.cfg_mgr.update()
        peer_groups = self.__extract_peer_group_names()
        pg_2_rm = self.__get_peer_group_to_route_map(peer_groups)
        rm_2_call = self.__get_route_map_calls(set(pg_2_rm.values()))
        ret = self.__get_peer_group_to_restart(deployment_id, pg_2_rm, rm_2_call, neighbor_type)
        return list(ret)

    def __get_enabled(self):
        """
        Load enable/disabled property from constants
        :return: True if enabled, False otherwise
        """
        return 'bgp' in self.constants \
           and 'allow_list' in self.constants["bgp"] \
           and "enabled" in self.constants["bgp"]["allow_list"] \
           and self.constants["bgp"]["allow_list"]["enabled"]

    def __load_constant_lists(self):
        """
        Load default prefix-list entries from constants.yml file
        """
        if 'bgp' in self.constants and 'allow_list' in self.constants["bgp"] \
                and "default_pl_rules" in self.constants["bgp"]["allow_list"]:
            obj = self.constants["bgp"]["allow_list"]["default_pl_rules"]
            if "v4" in obj:
                self.constants_v4 = obj["v4"]
            else:
                self.constants_v4 = []
            if "v6" in obj:
                self.constants_v6 = obj["v6"]
            else:
                self.constants_v6 = []

    def __get_constant_list(self, af):
        """
        Return loaded default prefix-list entries bases on address family
        :param af: address family
        :return: default prefix-list entries
        """
        if af == self.V4:
            return self.constants_v4
        else:
            return self.constants_v6

    def __to_prefix_list(self, af, allow_list):
        """
        Convert "allow list" prefix list, to a prefix-list rules
        :param af: address-family
        :param allow_list: "allow list" prefix list
        :return: prefix-list rules
        """
        res = []
        prefix_mask_default = 32 if af == self.V4 else 128
        for prefix in allow_list:
            if 'le' in prefix or 'ge' in prefix:
                res.append("permit %s" % prefix)
            else:
                prefix_mask = int(prefix.split("/")[1])
                if prefix_mask == prefix_mask_default:
                    res.append("permit %s" % prefix)
                else:
                    res.append("permit %s le %d" % (prefix, prefix_mask_default))
        return res

    def __af_to_family(self, af):
        """
        Convert address family into prefix list family
        :param af: address family
        :return: prefix list ip family
        """
        return 'ip' if af == self.V4 else 'ipv6'

    def __get_default_action_community(self, data=None):
        """
        Determine the default action community based on the request.
        If request doesn't contain "default_action" field - the default_action value
        from the constants is being used
        :param data: SET request data
        :return: returns community value for "default_action"
        """
        drop_community = self.constants["bgp"]["allow_list"]["drop_community"]
        if data and "default_action" in data:
            if data["default_action"] == "deny":
                return "no-export"
            else: # "permit"
                return drop_community
        else:
            if "default_action" in self.constants["bgp"]["allow_list"]:
                if self.constants["bgp"]["allow_list"]["default_action"].strip() == "deny":
                    return "no-export"
                else:
                    return drop_community
            else:
                return drop_community
