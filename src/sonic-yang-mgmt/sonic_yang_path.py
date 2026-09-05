# This script is used as extension of sonic_yang class. It has methods of
# class sonic_yang. A separate file is used to avoid a single large file.

from __future__ import print_function, annotations
from json import dump, dumps, loads
import libyang as ly
import sonic_yang_ext
import re
from jsonpointer import JsonPointer
from typing import Any, Dict, List, Optional, Tuple, TYPE_CHECKING

# class sonic_yang methods related to path handling, use mixin to extend sonic_yang
class SonicYangPathMixin:
    """
    All xpath operations in this class are only relevent to ConfigDb and the conversion to YANG xpath.
    It is not meant to support all the xpath functionalities, just the ones relevent to ConfigDb/YANG.
    """

    # Mixin type stubs — these attributes are defined in SonicYang.__init__
    if TYPE_CHECKING:
        confDbYangMap: Dict[str, Any]
        configPathCache: Dict[Tuple[str, bool], str]
    @staticmethod
    def configdb_path_split(configdb_path: str):
        if configdb_path is None or configdb_path == "" or configdb_path == "/":
            return []
        return JsonPointer(configdb_path).parts

    @staticmethod
    def configdb_path_join(configdb_tokens: List[str]):
        return JsonPointer.from_parts(configdb_tokens).path

    @staticmethod
    def xpath_join(xpath_tokens: List[str], schema_xpath: bool) -> str:
        if not schema_xpath:
            return "/" + "/".join(xpath_tokens)

        # Schema XPath in libyang v3 uses RFC 7951 JSON-style prefixes:
        # nodes inherit their module from their parent, so only the first
        # token (which crosses from root into the module) gets a prefix.
        # Subsequent tokens from the same module have no prefix.
        return "/" + "/".join(xpath_tokens)

    @staticmethod
    def xpath_split(xpath: str) -> List[str]:
        """
        Splits the given xpath into tokens by '/'.

        Example:
          xpath: /sonic-vlan:sonic-vlan/VLAN_MEMBER/VLAN_MEMBER_LIST[name='Vlan1000'][port='Ethernet8']/tagging_mode
          tokens: sonic-vlan:sonic-vlan, VLAN_MEMBER, VLAN_MEMBER_LIST[name='Vlan1000'][port='Ethernet8'], tagging_mode
        """
        if xpath == "":
            raise ValueError("xpath cannot be empty")

        if xpath == "/":
            return []

        idx = 0
        tokens = []
        while idx < len(xpath):
            end = SonicYangPathMixin.__get_xpath_token_end(idx+1, xpath)
            token = xpath[idx+1:end]
            tokens.append(token)
            idx = end

        return tokens

    def configdb_path_to_xpath(self, configdb_path: str, schema_xpath: bool=False, configdb: Optional[dict]=None) -> Optional[str]:
        """
        Converts the given ConfigDB path to a Yang data module xpath.
        Parameters:
          - configdb_path: The JSON path in the form taken by Config DB,
            e.g. /VLAN_MEMBER/Vlan1000|Ethernet8/tagging_mode
          - schema_xpath: Whether or not to output the xpath in schema form or data form.  Schema form will not use
            the data in the path, only table/list names.  Defaults to false, so will emit data xpaths.
          - configdb: If provided, and schema_xpath is false, will also emit the xpath token for a specific leaf-list
            entry based on the value within the configdb itself.  This is provided in the parsed configdb format, such
            as returned from json.loads().

        Example:
          1. configdb_path: /VLAN_MEMBER/Vlan1000|Ethernet8/tagging_mode
             schema_xpath: False
             returns: /sonic-vlan:sonic-vlan/VLAN_MEMBER/VLAN_MEMBER_LIST[name='Vlan1000'][port='Ethernet8']/tagging_mode
          2. configdb_path: /VLAN_MEMBER/Vlan1000|Ethernet8/tagging_mode
             schema_xpath: True
             returns: /sonic-vlan:sonic-vlan/VLAN_MEMBER/VLAN_MEMBER_LIST/tagging_mode
        """

        if configdb_path is None or len(configdb_path) == 0 or configdb_path == "/":
            return "/"

        # Fetch from cache if available
        key = (configdb_path, schema_xpath)
        result = self.configPathCache.get(key)
        if result is not None:
            return result

        # Not available, go through conversion
        tokens = self.configdb_path_split(configdb_path)
        if len(tokens) == 0:
            return None

        xpath_tokens = []
        table = tokens[0]

        cmap = self.confDbYangMap[table]

        # getting the top level element <module>:<topLevelContainer>
        xpath_tokens.append(cmap['module']+":"+cmap['topLevelContainer'])

        xpath_tokens.extend(self.__get_xpath_tokens_from_container(cmap['container'], tokens, 0, schema_xpath, configdb))

        xpath = self.xpath_join(xpath_tokens, schema_xpath)

        # Save to cache
        self.configPathCache[key] = xpath

        return xpath


    def xpath_to_configdb_path(self, xpath: str, configdb: Optional[dict] = None) -> str:
        """
        Converts the given XPATH to ConfigDB Path.
        If the xpath references a list value and the configdb is provided, the
        generated path will reference the index of the list value
        Example:
          xpath: /sonic-vlan:sonic-vlan/VLAN_MEMBER/VLAN_MEMBER_LIST[name='Vlan1000'][port='Ethernet8']/tagging_mode
          path: /VLAN_MEMBER/Vlan1000|Ethernet8/tagging_mode
        """
        tokens = self.xpath_split(xpath)
        if len(tokens) == 0:
            return ""

        if len(tokens) == 1:
            raise ValueError("xpath cannot be just the module-name, there is no mapping to path")

        table = tokens[1]
        cmap = self.confDbYangMap[table]

        configdb_path_tokens = self.__get_configdb_path_tokens_from_container(cmap['container'], tokens, 1, configdb)
        return self.configdb_path_join(configdb_path_tokens)


    def __get_xpath_tokens_from_container(self, model_snode, configdb_path_tokens: List[str], token_index: int, schema_xpath: bool, configdb: Optional[dict]) -> List[str]:
        token = configdb_path_tokens[token_index]
        xpath_tokens = [token]

        if len(configdb_path_tokens)-1 == token_index:
            return xpath_tokens

        # check if the configdb token is referring to a list
        list_snode = self.__get_list_model(model_snode, configdb_path_tokens, token_index)
        if list_snode is not None:
            new_xpath_tokens = self.__get_xpath_tokens_from_list(list_snode, configdb_path_tokens, token_index+1, schema_xpath, configdb)
            xpath_tokens.extend(new_xpath_tokens)
            return xpath_tokens

        # check if it is targetting a child container
        child_container = self.__find_child_by_name(model_snode, configdb_path_tokens[token_index+1], (ly.SNode.CONTAINER,))
        if child_container is not None:
            new_xpath_tokens = self.__get_xpath_tokens_from_container(child_container, configdb_path_tokens, token_index+1, schema_xpath, configdb)
            xpath_tokens.extend(new_xpath_tokens)
            return xpath_tokens

        leaf_token = self.__get_xpath_token_from_leaf(model_snode, configdb_path_tokens, token_index+1, schema_xpath, configdb)
        xpath_tokens.append(leaf_token)

        return xpath_tokens


    # Locate a child of model_snode with the given name and (optional) node-type
    # filter. SNode.children(types=...) flattens choice/case automatically.
    def __find_child_by_name(self, model_snode, name: str, types=None):
        for child in model_snode.children(types=types):
            if child.name() == name:
                return child
        return None


    # A configdb list specifies the container name, plus the keys separated by |.  We are
    # scanning the model for a list with a matching *number* of keys and returning the
    # reference to the model with the definition.  It is not valid to have 2 lists in
    # the same container with the same number of keys since we have no way to match.
    def __get_list_model(self, model_snode, configdb_path_tokens: List[str], token_index: int):
        parent_container_name = configdb_path_tokens[token_index]
        lists = list(model_snode.children(types=(ly.SNode.LIST,)))

        if len(lists) == 0:
            return None

        # Container contains a single list, just return it
        # TODO: check if matching also by name is necessary
        if len(lists) == 1:
            return lists[0]

        configdb_values_str = configdb_path_tokens[token_index+1]
        # Format: "value1|value2|value|..."
        configdb_values = configdb_values_str.split("|")
        for list_snode in lists:
            yang_keys = [k.name() for k in list_snode.keys()]
            # if same number of values and keys, this is the intended list-model
            # TODO: Match also on types and not only the length of the keys/values
            if len(yang_keys) == len(configdb_values):
                return list_snode
        raise ValueError(f"Container {parent_container_name} has multiple lists, "
                         f"but none of them match the config_db value {configdb_values_str}")


    def __get_xpath_tokens_from_list(self, list_snode, configdb_path_tokens: List[str], token_index: int, schema_xpath: bool, configdb: Optional[dict]):
        item_token=""
        list_keys_str = " ".join(k.name() for k in list_snode.keys())

        if schema_xpath:
            item_token = list_snode.name()
        else:
            keyDict = self.__parse_configdb_key_to_dict(list_keys_str, configdb_path_tokens[token_index])
            keyTokens = [f"[{key}='{keyDict[key]}']" for key in keyDict]
            item_token = f"{list_snode.name()}{''.join(keyTokens)}"

        xpath_tokens = [item_token]

        # If we're pointing to the top level list item, and not a child leaf
        # then we can just return.
        if len(configdb_path_tokens)-1 == token_index:
            return xpath_tokens

        type_1_inner_list = self.__type1_get_model(list_snode)
        if type_1_inner_list is not None:
            token = self.__type1_get_xpath_token(type_1_inner_list, configdb_path_tokens, token_index+1, schema_xpath)
            xpath_tokens.append(token)
            return xpath_tokens

        leaf_token = self.__get_xpath_token_from_leaf(list_snode, configdb_path_tokens, token_index+1, schema_xpath, configdb)
        xpath_tokens.append(leaf_token)
        return xpath_tokens


    # Parse configdb key like Vlan1000|Ethernet8 (such as a key might be under /VLAN_MEMBER/)
    # into its key/value dictionary form: { "name": "VLAN1000", "port": "Ethernet8" }
    def __parse_configdb_key_to_dict(self, listKeys: str, configDbKey: str) -> dict:
        xpath_list_keys = listKeys.split()
        configdb_values = configDbKey.split("|")
        # match lens
        if len(xpath_list_keys) != len(configdb_values):
            raise ValueError("Value not found for {} in {}".format(listKeys, configDbKey))
        # create the keyDict
        rv = dict()
        for i in range(len(xpath_list_keys)):
            rv[xpath_list_keys[i]] = configdb_values[i].strip()
        return rv


    # Type1 lists are lists contained within another list.  They always have exactly 1 key, and due to
    # this they are special cased with a static lookup table.  Check to see if the
    # specified model is a type1 list and if so, return the inner list SNode.
    def __type1_get_model(self, list_snode):
        if list_snode.name() not in sonic_yang_ext.Type_1_list_maps_model:
            return None
        # Type 1 list is expected to have a single inner list child.
        return next(iter(list_snode.children(types=(ly.SNode.LIST,))), None)


    # Type1 lists are lists contained within another list.  They always have exactly 1 key, and due to
    # this they are special cased with a static lookup table.  This is just a helper to do a quick
    # transformation from configdb to the xpath key.
    def __type1_get_xpath_token(self, inner_list_snode, configdb_path_tokens: List[str], token_index: int, schema_xpath: bool) -> str:
        if schema_xpath:
            return inner_list_snode.name()
        # Type 1 inner lists always have exactly one key.
        key_name = next(iter(inner_list_snode.keys())).name()
        return f"{inner_list_snode.name()}[{key_name}='{configdb_path_tokens[token_index]}']"


    # This function outputs the xpath token for leaf, choice, and leaf-list entries.
    # iter_children with default options descends into choice/case automatically,
    # so leaves under choice/case are flattened into the children() iteration.
    def __get_xpath_token_from_leaf(self, model_snode, configdb_path_tokens: List[str], token_index: int, schema_xpath: bool, configdb: Optional[dict]) -> str:
        token = configdb_path_tokens[token_index]

        # checking all leaves (including those under choice/case)
        if self.__find_child_by_name(model_snode, token, (ly.SNode.LEAF,)) is not None:
            return token

        # checking leaf-list (i.e. arrays of string, number or bool)
        if self.__find_child_by_name(model_snode, token, (ly.SNode.LEAFLIST,)) is not None:
            # If there are no more tokens, just return the current token.
            if len(configdb_path_tokens)-1 == token_index:
                return token

            value = self.__get_configdb_value(configdb_path_tokens, configdb)
            if value is None or schema_xpath:
                return token

            # Reference an explicit leaf list value
            return f"{token}[.='{value}']"

        raise ValueError(f"Path token not found.\n  model: {model_snode.schema_path()}\n  token_index: {token_index}\n  " + \
                         f"path_tokens: {configdb_path_tokens}\n  config: {configdb}")


    def __get_configdb_value(self, configdb_path_tokens: List[str], configdb: Optional[dict]) -> Optional[str]:
        if configdb is None:
            return None

        ptr: Any = configdb
        for i in range(len(configdb_path_tokens)):
            if isinstance(ptr, dict):
                ptr = ptr[configdb_path_tokens[i]]
            elif isinstance(ptr, list):
                ptr = ptr[int(configdb_path_tokens[i])]
            else:
                return None
        return ptr

    @staticmethod
    def __get_xpath_token_end(start: int, xpath: str) -> int:
        idx = start
        while idx < len(xpath):
            if xpath[idx] == "/":
                break
            elif xpath[idx] == "[":
                idx = SonicYangPathMixin.__get_xpath_predicate_end(idx, xpath)
            idx = idx+1

        return idx

    @staticmethod
    def __get_xpath_predicate_end(start: int, xpath: str) -> int:
        idx = start
        while idx < len(xpath):
            if xpath[idx] == "]":
                break
            elif xpath[idx] == "'" or xpath[idx] == '"':
                idx = SonicYangPathMixin.__get_xpath_quote_str_end(xpath[idx], idx, xpath)

            idx = idx+1

        return idx

    @staticmethod
    def __get_xpath_quote_str_end(ch: str, start: int, xpath: str) -> int:
        idx = start+1 # skip first single quote
        while idx < len(xpath):
            if xpath[idx] == ch:
                break
            # libyang implements XPATH 1.0 which does not escape single or double quotes
            # libyang src: https://netopeer.liberouter.org/doc/libyang/master/html/howtoxpath.html
            # XPATH 1.0 src: https://www.w3.org/TR/1999/REC-xpath-19991116/#NT-Literal
            idx = idx+1

        return idx


    def __get_configdb_path_tokens_from_container(self, model_snode, xpath_tokens: List[str], token_index: int, configdb: Optional[dict]) -> List[str]:
        token = xpath_tokens[token_index]
        configdb_path_tokens = [token]

        if len(xpath_tokens)-1 == token_index:
            return configdb_path_tokens

        if configdb is not None:
            configdb = configdb[token]

        # check child list
        list_name = xpath_tokens[token_index+1].split("[")[0]
        list_snode = self.__find_child_by_name(model_snode, list_name, (ly.SNode.LIST,))
        if list_snode is not None:
            new_path_tokens = self.__get_configdb_path_tokens_from_list(list_snode, xpath_tokens, token_index+1, configdb)
            configdb_path_tokens.extend(new_path_tokens)
            return configdb_path_tokens

        container_name = xpath_tokens[token_index+1]
        container_snode = self.__find_child_by_name(model_snode, container_name, (ly.SNode.CONTAINER,))
        if container_snode is not None:
            new_path_tokens = self.__get_configdb_path_tokens_from_container(container_snode, xpath_tokens, token_index+1, configdb)
            configdb_path_tokens.extend(new_path_tokens)
            return configdb_path_tokens

        new_path_tokens = self.__get_configdb_path_tokens_from_leaf(model_snode, xpath_tokens, token_index+1, configdb)
        configdb_path_tokens.extend(new_path_tokens)

        return configdb_path_tokens


    def __xpath_keys_to_dict(self, token: str) -> dict:
        # Token passed in is something like:
        #   VLAN_MEMBER_LIST[name='Vlan1000'][port='Ethernet8']
        # Strip off the Table name, and return a dictionary of key/value pairs.

        # See if we have keys
        idx = token.find("[")
        if idx == -1:
            return dict()

        # Strip off table name
        token = token[idx:]

        # Use regex to extract our keys and values
        key_value_pattern = r"\[([^=]+)='([^']*)'\]"
        matches = re.findall(key_value_pattern, token)
        kv = dict()
        for item in matches:
            kv[item[0]] = item[1]

        return kv

    def __get_configdb_path_tokens_from_list(self, list_snode, xpath_tokens: List[str], token_index: int, configdb: Optional[dict]):
        token = xpath_tokens[token_index]
        key_dict = self.__xpath_keys_to_dict(token)

        # If no keys specified return empty tokens, as we are already inside the correct table.
        # Also note that the list name in SonicYang has no correspondence in ConfigDb and is ignored.
        # Example where VLAN_MEMBER_LIST has no specific key/value:
        #   xpath: /sonic-vlan:sonic-vlan/VLAN_MEMBER/VLAN_MEMBER_LIST
        #   path: /VLAN_MEMBER
        if not(key_dict):
            return []

        key_list = [k.name() for k in list_snode.keys()]

        if len(key_list) != len(key_dict):
            raise ValueError(f"Keys in configDb not matching keys in SonicYang. ConfigDb keys: {key_dict.keys()}. SonicYang keys: {key_list}")

        values = [key_dict[k] for k in key_list]
        configdb_path_token = '|'.join(values)
        configdb_path_tokens = [ configdb_path_token ]

        # Set pointer to pass for recursion
        if configdb is not None:
            # use .get() here as if configdb doesn't have the key it could return failure, but it can actually still
            # generate a mostly relevant path.
            configdb = configdb.get(configdb_path_token)

        # At end, just return
        if len(xpath_tokens)-1 == token_index:
            return configdb_path_tokens

        next_token = xpath_tokens[token_index+1]
        # if the target node is a key, then it does not have a correspondene to path.
        # Just return the current 'key1|key2|..' token as it already refers to the keys
        # Example where the target node is 'name' which is a key in VLAN_MEMBER_LIST:
        #   xpath: /sonic-vlan:sonic-vlan/VLAN_MEMBER/VLAN_MEMBER_LIST[name='Vlan1000'][port='Ethernet8']/name
        #   path: /VLAN_MEMBER/Vlan1000|Ethernet8
        if next_token in key_dict:
            return configdb_path_tokens

        type_1_inner_list = self.__type1_get_model(list_snode)
        if type_1_inner_list is not None:
            new_path_tokens = self.__get_configdb_path_tokens_from_type_1_list(type_1_inner_list, xpath_tokens, token_index+1, configdb)
            configdb_path_tokens.extend(new_path_tokens)
            return configdb_path_tokens

        new_path_tokens = self.__get_configdb_path_tokens_from_leaf(list_snode, xpath_tokens, token_index+1, configdb)
        configdb_path_tokens.extend(new_path_tokens)
        return configdb_path_tokens


    def __get_configdb_path_tokens_from_leaf(self, model_snode, xpath_tokens: List[str], token_index: int, configdb: Optional[dict]) -> List[Any]:
        token = xpath_tokens[token_index]

        # checking all leaves (including those under choice/case — flattened by iter_children)
        if self.__find_child_by_name(model_snode, token, (ly.SNode.LEAF,)) is not None:
            return [token]

        # checking leaf-list
        leaf_list_tokens = token.split("[", 1) # split once on the first '[', a regex is used later to fetch keys/values
        leaf_list_name = leaf_list_tokens[0]
        if self.__find_child_by_name(model_snode, leaf_list_name, (ly.SNode.LEAFLIST,)) is not None:
            # if whole-list is to be returned, such as if there is no key, or if configdb is not provided,
            # Just return the list-name without checking the list items
            # Example:
            #   xpath: /sonic-vlan:sonic-vlan/VLAN/VLAN_LIST[name='Vlan1000']/dhcp_servers
            #   path: /VLAN/Vlan1000/dhcp_servers
            if configdb is None or len(leaf_list_tokens) == 1:
                return [leaf_list_name]
            leaf_list_pattern = r"^[^\[]+(?:\[\.='([^']*)'\])?$"
            leaf_list_regex = re.compile(leaf_list_pattern)
            match = leaf_list_regex.match(token)
            if match is None:
                raise ValueError(f"leaf-list token did not match expected pattern: {token}")
            leaf_list_value = match.group(1)
            list_config = configdb[leaf_list_name]
            # Workaround for those fields who is defined as leaf-list in YANG model but have string value in config DB
            # No need to lookup the item index in ConfigDb since the list is represented as a string, return path to string immediately
            # Example:
            #   xpath: /sonic-buffer-port-egress-profile-list:sonic-buffer-port-egress-profile-list/BUFFER_PORT_EGRESS_PROFILE_LIST/BUFFER_PORT_EGRESS_PROFILE_LIST_LIST[port='Ethernet9']/profile_list[.='egress_lossy_profile']
            #   path: /BUFFER_PORT_EGRESS_PROFILE_LIST/Ethernet9/profile_list
            if isinstance(list_config, str):
                return [leaf_list_name]

            if not isinstance(list_config, list):
                raise ValueError(f"list_config is expected to be of type list or string. Found {type(list_config)}.\n  " + \
                                 f"model: {model_snode.schema_path()}\n  token_index: {token_index}\n  " + \
                                 f"xpath_tokens: {xpath_tokens}\n  config: {configdb}")

            list_idx = list_config.index(leaf_list_value)
            return [leaf_list_name, list_idx]

        raise ValueError(f"Xpath token not found.\n  model: {model_snode.schema_path()}\n  token_index: {token_index}\n  " + \
                         f"xpath_tokens: {xpath_tokens}\n  config: {configdb}")


    def __get_configdb_path_tokens_from_type_1_list(self, inner_list_snode, xpath_tokens: List[str], token_index: int, configdb: Optional[dict]):
        type_1_inner_list_name = inner_list_snode.name()

        token = xpath_tokens[token_index]
        list_tokens = token.split("[", 1) # split once on the first '[', first element will be the inner list name
        inner_list_name = list_tokens[0]

        if type_1_inner_list_name != inner_list_name:
            raise ValueError(f"Type 1 inner list name '{type_1_inner_list_name}' does match xpath inner list name '{inner_list_name}'.")

        key_dict = self.__xpath_keys_to_dict(token)

        # If no keys specified return empty tokens, as we are already inside the correct table.
        # Also note that the type 1 inner list name in SonicYang has no correspondence in ConfigDb and is ignored.
        # Example where DOT1P_TO_TC_MAP_LIST has no specific key/value:
        #   xpath: /sonic-dot1p-tc-map:sonic-dot1p-tc-map/DOT1P_TO_TC_MAP/DOT1P_TO_TC_MAP_LIST[name='Dot1p_to_tc_map1']/DOT1P_TO_TC_MAP
        #   path: /DOT1P_TO_TC_MAP/Dot1p_to_tc_map1
        if not(key_dict):
            return []

        if len(key_dict) > 1:
            raise ValueError(f"Type 1 inner list should have only 1 key in xpath, {len(key_dict)} specified. Key dictionary: {key_dict}")

        keyName = next(iter(key_dict.keys()))
        value = key_dict[keyName]

        path_tokens = [value]

        # If this is the last xpath token, return the path tokens we have built so far, no need for futher checks
        # Example:
        #   xpath: /sonic-dot1p-tc-map:sonic-dot1p-tc-map/DOT1P_TO_TC_MAP/DOT1P_TO_TC_MAP_LIST[name='Dot1p_to_tc_map1']/DOT1P_TO_TC_MAP[dot1p='2']
        #   path: /DOT1P_TO_TC_MAP/Dot1p_to_tc_map1/2
        if token_index+1 >= len(xpath_tokens):
            return path_tokens

        # Checking if the next_token is actually a child leaf of the inner type 1 list, for which case
        # just ignore the token, and return the already created ConfigDb path pointing to the whole object
        # Example where the leaf specified is the key:
        #   xpath: /sonic-dot1p-tc-map:sonic-dot1p-tc-map/DOT1P_TO_TC_MAP/DOT1P_TO_TC_MAP_LIST[name='Dot1p_to_tc_map1']/DOT1P_TO_TC_MAP[dot1p='2']/dot1p
        #   path: /DOT1P_TO_TC_MAP/Dot1p_to_tc_map1/2
        # Example where the leaf specified is not the key:
        #   xpath: /sonic-dot1p-tc-map:sonic-dot1p-tc-map/DOT1P_TO_TC_MAP/DOT1P_TO_TC_MAP_LIST[name='Dot1p_to_tc_map1']/DOT1P_TO_TC_MAP[dot1p='2']/tc
        #   path: /DOT1P_TO_TC_MAP/Dot1p_to_tc_map1/2
        next_token = xpath_tokens[token_index+1]
        if self.__find_child_by_name(inner_list_snode, next_token, (ly.SNode.LEAF,)) is not None:
            return path_tokens

        raise ValueError(f"Type 1 inner list '{type_1_inner_list_name}' does not have a child leaf named '{next_token}'")
