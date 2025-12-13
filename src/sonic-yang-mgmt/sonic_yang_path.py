# This script is used as extension of sonic_yang class. It has methods of
# class sonic_yang. A separate file is used to avoid a single large file.

from __future__ import print_function
from json import dump, dumps, loads
import sonic_yang_ext
import re
from jsonpointer import JsonPointer
from typing import List

# class sonic_yang methods related to path handling, use mixin to extend sonic_yang
class SonicYangPathMixin:
    """
    All xpath operations in this class are only relevent to ConfigDb and the conversion to YANG xpath.
    It is not meant to support all the xpath functionalities, just the ones relevent to ConfigDb/YANG.
    """
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

        # Schema XPath in libyang v1 wants each token prefixed with the module name.
        # The first token should have this, use that to prefix the rest.
        module_name = xpath_tokens[0].split(":")[0]

        return "/" + ("/" + module_name + ":").join(xpath_tokens)

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

    def configdb_path_to_xpath(self, configdb_path: str, schema_xpath: bool=False, configdb: dict=None) -> str:
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


    def xpath_to_configdb_path(self, xpath: str, configdb: dict = None) -> str:
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


    def __get_xpath_tokens_from_container(self, model: dict, configdb_path_tokens: List[str], token_index: int, schema_xpath: bool, configdb: dict) -> List[str]:
        token = configdb_path_tokens[token_index]
        xpath_tokens = [token]

        if len(configdb_path_tokens)-1 == token_index:
            return xpath_tokens

        # check if the configdb token is referring to a list
        list_model = self.__get_list_model(model, configdb_path_tokens, token_index)
        if list_model:
            new_xpath_tokens = self.__get_xpath_tokens_from_list(list_model, configdb_path_tokens, token_index+1, schema_xpath, configdb)
            xpath_tokens.extend(new_xpath_tokens)
            return xpath_tokens

        # check if it is targetting a child container
        child_container_model = self.__get_model(model.get('container'), configdb_path_tokens[token_index+1])
        if child_container_model:
            new_xpath_tokens = self.__get_xpath_tokens_from_container(child_container_model, configdb_path_tokens, token_index+1, schema_xpath, configdb)
            xpath_tokens.extend(new_xpath_tokens)
            return xpath_tokens

        leaf_token = self.__get_xpath_token_from_leaf(model, configdb_path_tokens, token_index+1, schema_xpath, configdb)
        xpath_tokens.append(leaf_token)

        return xpath_tokens


    # Locate a model matching the given name.  If the model provided is a dict,
    # it simply ensures the name matches and returns self.  If the model
    # provided is a list it scans the list for a matching name and returns that
    # model.
    def __get_model(self, model, name: str) -> dict:
        if isinstance(model, dict) and model['@name'] == name:
            return model
        if isinstance(model, list):
            for submodel in model:
                if submodel['@name'] == name:
                    return submodel

        return None


    # A configdb list specifies the container name, plus the keys separated by |.  We are 
    # scanning the model for a list with a matching *number* of keys and returning the
    # reference to the model with the definition.  It is not valid to have 2 lists in
    # the same container with the same number of keys since we have no way to match.
    def __get_list_model(self, model: dict, configdb_path_tokens: List[str], token_index: int) -> dict:
        parent_container_name = configdb_path_tokens[token_index]
        clist = model.get('list')
        # Container contains a single list, just return it
        # TODO: check if matching also by name is necessary
        if isinstance(clist, dict):
            return clist

        if isinstance(clist, list):
            configdb_values_str = configdb_path_tokens[token_index+1]
            # Format: "value1|value2|value|..."
            configdb_values = configdb_values_str.split("|")
            for list_model in clist:
                yang_keys_str = list_model['key']['@value']
                # Format: "key1 key2 key3 ..."
                yang_keys = yang_keys_str.split()
                # if same number of values and keys, this is the intended list-model
                # TODO: Match also on types and not only the length of the keys/values
                if len(yang_keys) == len(configdb_values):
                    return list_model
            raise ValueError(f"Container {parent_container_name} has multiple lists, "
                             f"but none of them match the config_db value {configdb_values_str}")


    def __get_xpath_tokens_from_list(self, model: dict, configdb_path_tokens: List[str], token_index: int, schema_xpath: bool, configdb: dict):
        item_token=""

        if schema_xpath:
            item_token = model['@name']
        else:
            keyDict = self.__parse_configdb_key_to_dict(model['key']['@value'], configdb_path_tokens[token_index])
            keyTokens = [f"[{key}='{keyDict[key]}']" for key in keyDict]
            item_token = f"{model['@name']}{''.join(keyTokens)}"

        xpath_tokens = [item_token]

        # If we're pointing to the top level list item, and not a child leaf
        # then we can just return.
        if len(configdb_path_tokens)-1 == token_index:
            return xpath_tokens

        type_1_list_model = self.__type1_get_model(model)
        if type_1_list_model:
            token = self.__type1_get_xpath_token(type_1_list_model, configdb_path_tokens, token_index+1, schema_xpath)
            xpath_tokens.append(token)
            return xpath_tokens

        leaf_token = self.__get_xpath_token_from_leaf(model, configdb_path_tokens, token_index+1, schema_xpath, configdb)
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
    # specified model is a type1 list and if so, return the model.
    def __type1_get_model(self, model: dict) -> dict:
        list_name = model['@name']
        if list_name not in sonic_yang_ext.Type_1_list_maps_model:
            return None

        # Type 1 list is expected to have a single inner list model.
        # No need to check if it is a dictionary of list models.
        return model.get('list')


    # Type1 lists are lists contained within another list.  They always have exactly 1 key, and due to
    # this they are special cased with a static lookup table.  This is just a helper to do a quick
    # transformation from configdb to the xpath key.
    def __type1_get_xpath_token(self, model: dict, configdb_path_tokens: List[str], token_index: int, schema_xpath: bool) -> str:
        if schema_xpath:
            return model['@name']
        return f"{model['@name']}[{model['key']['@value']}='{configdb_path_tokens[token_index]}']"


    # This function outputs the xpath token for leaf, choice, and leaf-list entries.
    def __get_xpath_token_from_leaf(self, model: dict, configdb_path_tokens: List[str], token_index: int, schema_xpath: bool, configdb: dict) -> str:
        token = configdb_path_tokens[token_index]

        # checking all leaves
        leaf_model = self.__get_model(model.get('leaf'), token)
        if leaf_model:
            return token

        # checking choice
        choices = model.get('choice')
        if choices:
            for choice in choices:
                cases = choice['case']
                for case in cases:
                    leaf_model = self.__get_model(case.get('leaf'), token)
                    if leaf_model:
                        return token

        # checking leaf-list (i.e. arrays of string, number or bool)
        leaf_list_model = self.__get_model(model.get('leaf-list'), token)
        if leaf_list_model:
            # If there are no more tokens, just return the current token.
            if len(configdb_path_tokens)-1 == token_index:
                return token

            value = self.__get_configdb_value(configdb_path_tokens, configdb)
            if value is None or schema_xpath:
                return token

            # Reference an explicit leaf list value
            return f"{token}[.='{value}']"

        raise ValueError(f"Path token not found.\n  model: {model}\n  token_index: {token_index}\n  " + \
                         f"path_tokens: {configdb_path_tokens}\n  config: {configdb}")


    def __get_configdb_value(self, configdb_path_tokens: List[str], configdb: dict) -> str:
        if configdb is None:
            return None

        ptr = configdb
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


    def __get_configdb_path_tokens_from_container(self, model: dict, xpath_tokens: List[str], token_index: int, configdb: dict) -> List[str]:
        token = xpath_tokens[token_index]
        configdb_path_tokens = [token]

        if len(xpath_tokens)-1 == token_index:
            return configdb_path_tokens

        if configdb is not None:
            configdb = configdb[token]

        # check child list
        list_name = xpath_tokens[token_index+1].split("[")[0]
        list_model = self.__get_model(model.get('list'), list_name)
        if list_model:
            new_path_tokens = self.__get_configdb_path_tokens_from_list(list_model, xpath_tokens, token_index+1, configdb)
            configdb_path_tokens.extend(new_path_tokens)
            return configdb_path_tokens

        container_name = xpath_tokens[token_index+1]
        container_model = self.__get_model(model.get('container'), container_name)
        if container_model:
            new_path_tokens = self.__get_configdb_path_tokens_from_container(container_model, xpath_tokens, token_index+1, configdb)
            configdb_path_tokens.extend(new_path_tokens)
            return configdb_path_tokens

        new_path_tokens = self.__get_configdb_path_tokens_from_leaf(model, xpath_tokens, token_index+1, configdb)
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
        key_value_pattern = "\[([^=]+)='([^']*)'\]"
        matches = re.findall(key_value_pattern, token)
        kv = dict()
        for item in matches:
            kv[item[0]] = item[1]

        return kv

    def __get_configdb_path_tokens_from_list(self, model: dict, xpath_tokens: List[str], token_index: int, configdb: dict):
        token = xpath_tokens[token_index]
        key_dict = self.__xpath_keys_to_dict(token)

        # If no keys specified return empty tokens, as we are already inside the correct table.
        # Also note that the list name in SonicYang has no correspondence in ConfigDb and is ignored.
        # Example where VLAN_MEMBER_LIST has no specific key/value:
        #   xpath: /sonic-vlan:sonic-vlan/VLAN_MEMBER/VLAN_MEMBER_LIST
        #   path: /VLAN_MEMBER
        if not(key_dict):
            return []

        listKeys = model['key']['@value']
        key_list = listKeys.split()

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

        type_1_list_model = self.__type1_get_model(model)
        if type_1_list_model:
            new_path_tokens = self.__get_configdb_path_tokens_from_type_1_list(type_1_list_model, xpath_tokens, token_index+1, configdb)
            configdb_path_tokens.extend(new_path_tokens)
            return configdb_path_tokens

        new_path_tokens = self.__get_configdb_path_tokens_from_leaf(model, xpath_tokens, token_index+1, configdb)
        configdb_path_tokens.extend(new_path_tokens)
        return configdb_path_tokens


    def __get_configdb_path_tokens_from_leaf(self, model: dict, xpath_tokens: List[str], token_index: int, configdb: dict) -> List[str]:
        token = xpath_tokens[token_index]

        # checking all leaves
        leaf_model = self.__get_model(model.get('leaf'), token)
        if leaf_model:
            return [token]

        # checking choices
        choices = model.get('choice')
        if choices:
            for choice in choices:
                cases = choice['case']
                for case in cases:
                    leaf_model = self.__get_model(case.get('leaf'), token)
                    if leaf_model:
                        return [token]

        # checking leaf-list
        leaf_list_tokens = token.split("[", 1) # split once on the first '[', a regex is used later to fetch keys/values
        leaf_list_name = leaf_list_tokens[0]
        leaf_list_model = self.__get_model(model.get('leaf-list'), leaf_list_name)
        if leaf_list_model:
            # if whole-list is to be returned, such as if there is no key, or if configdb is not provided,
            # Just return the list-name without checking the list items
            # Example:
            #   xpath: /sonic-vlan:sonic-vlan/VLAN/VLAN_LIST[name='Vlan1000']/dhcp_servers
            #   path: /VLAN/Vlan1000/dhcp_servers
            if configdb is None or len(leaf_list_tokens) == 1:
                return [leaf_list_name]
            leaf_list_pattern = "^[^\[]+(?:\[\.='([^']*)'\])?$"
            leaf_list_regex = re.compile(leaf_list_pattern)
            match = leaf_list_regex.match(token)
            # leaf_list_name = match.group(1)
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
                                 f"model: {model}\n  token_index: {token_index}\n  " + \
                                 f"xpath_tokens: {xpath_tokens}\n  config: {configdb}")

            list_idx = list_config.index(leaf_list_value)
            return [leaf_list_name, list_idx]

        raise ValueError(f"Xpath token not found.\n  model: {model}\n  token_index: {token_index}\n  " + \
                         f"xpath_tokens: {xpath_tokens}\n  config: {configdb}")


    def __get_configdb_path_tokens_from_type_1_list(self, model: dict, xpath_tokens: List[str], token_index: int, configdb: dict):
        type_1_inner_list_name = model['@name']

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
        leaf_model = self.__get_model(model.get('leaf'), next_token)
        if leaf_model:
            return path_tokens

        raise ValueError(f"Type 1 inner list '{type_1_inner_list_name}' does not have a child leaf named '{next_token}'")
