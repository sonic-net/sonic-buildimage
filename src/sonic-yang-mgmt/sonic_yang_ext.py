# This script is used as extension of sonic_yang class. It has methods of
# class sonic_yang. A separate file is used to avoid a single large file.

from __future__ import print_function, annotations
import libyang as ly
import syslog
from json import dump, dumps, loads
from glob import glob
import copy
import traceback
from typing import TYPE_CHECKING, Any, Dict, List, Optional
from sonic_yang_path import SonicYangPathMixin

if TYPE_CHECKING:
    from sonic_yang import SonicYang

Type_1_list_maps_model = [
    'DSCP_TO_TC_MAP_LIST',
    'DOT1P_TO_TC_MAP_LIST',
    'TC_TO_PRIORITY_GROUP_MAP_LIST',
    'TC_TO_QUEUE_MAP_LIST',
    'MAP_PFC_PRIORITY_TO_QUEUE_LIST',
    'PFC_PRIORITY_TO_PRIORITY_GROUP_MAP_LIST',
    'DSCP_TO_FC_MAP_LIST',
    'EXP_TO_FC_MAP_LIST',
    'CABLE_LENGTH_LIST',
    'MPLS_TC_TO_TC_MAP_LIST',
    'TC_TO_DSCP_MAP_LIST',
]

# Workaround for those fields who is defined as leaf-list in YANG model but have string value in config DB.
# Dictinary structure key = (<table_name>, <field_name>), value = seperator
LEAF_LIST_WITH_STRING_VALUE_DICT = {
    ('MIRROR_SESSION', 'src_ip'): ',',
    ('NTP', 'src_intf'): ';',
    ('BGP_ALLOWED_PREFIXES', 'prefixes_v4'): ',',
    ('BGP_ALLOWED_PREFIXES', 'prefixes_v6'): ',',
    ('BUFFER_PORT_EGRESS_PROFILE_LIST', 'profile_list'): ',',
    ('BUFFER_PORT_INGRESS_PROFILE_LIST', 'profile_list'): ',',
    ('PORT', 'adv_speeds'): ',',
    ('PORT', 'adv_interface_types'): ',',
}

"""
This is the Exception thrown out of all public function of this class.
"""
class SonicYangException(Exception):
    pass

# class sonic_yang methods, use mixin to extend sonic_yang
class SonicYangExtMixin(SonicYangPathMixin):

    # Mixin type stubs — these attributes are defined in SonicYang.__init__
    if TYPE_CHECKING:
        yang_dir: str
        ctx: ly.Context
        module: Optional[ly.Module]
        root: Optional[ly.DNode]
        DEBUG: bool
        print_log_enabled: bool
        yangFiles: List[str]
        confDbYangMap: Dict[str, Any]
        _yJsonCache: Optional[List[Dict[str, Any]]]
        jIn: Dict[str, Any]
        xlateJson: Dict[str, Any]
        revXlateJson: Dict[str, Any]
        tablesWithOutYang: Dict[str, Any]
        elementPath: List[str]

        def sysLog(self, debug: int = ..., msg: Optional[str] = ..., doPrint: bool = ...) -> None: ...
        def fail(self, e: Exception) -> None: ...
        def _load_schema_module(self, yang_file: str) -> Optional[ly.Module]: ...
        def _find_data_node(self, data_xpath: str) -> Optional[ly.DNode]: ...
        def _find_parent_data_node(self, data_xpath: str) -> Optional[ly.DNode]: ...
        def _deleteNode(self, xpath: str) -> bool: ...
        def _print_data_mem(self, option: str) -> str: ...

    """
    load all YANG models, build the table-to-schema map. (Public function)
    """
    def loadYangModel(self):

        try:
            # Invalidate any cached YIN-shape view from a previous load.
            self._yJsonCache = None
            # get all files
            self.yangFiles = glob(self.yang_dir +"/*.yang")
            # load yang modules
            for file in self.yangFiles:
                m = self._load_schema_module(file)
                if m is not None:
                    self.sysLog(msg="module: {} is loaded successfully".format(m.name()))
                else:
                    raise(Exception("Could not load module {}".format(file)))

            # keep only modules name in self.yangFiles
            self.yangFiles = [f.split('/')[-1] for f in self.yangFiles]
            self.yangFiles = [f.split('.')[0] for f in self.yangFiles]
            self.sysLog(syslog.LOG_DEBUG,'Loaded below Yang Models')
            self.sysLog(syslog.LOG_DEBUG,str(self.yangFiles))

            # libyang3 already resolves uses/grouping/augment/deviation when it
            # compiles each schema, so the SNode tree we walk below has all of
            # that pre-resolved — no manual uses/grouping inlining is needed.
            self._createDBTableToModuleMap()
        except Exception as e:
            self.sysLog(msg="Yang Models Load failed:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise SonicYangException("Yang Models Load failed\n{}".format(str(e)))

        return True

    def _createDBTableToModuleMap(self):
        """
        Populate self.confDbYangMap[<table_name>] = {
            'module': <module name>,
            'topLevelContainer': <top-level container name>,
            'container': <SContainer for the table>,
        }

        Each YANG module that maps to ConfigDB is expected to have a single
        top-level container whose name matches the module name; each child
        container of that top-level container is a ConfigDB table. See
        https://github.com/Azure/SONiC/blob/master/doc/mgmt/SONiC_YANG_Model_Guidelines.md.
        Modules with no top-level container are helper-only (typedefs,
        groupings, etc.) and are skipped here — they don't define tables.
        """
        for module_name in self.yangFiles:
            m = self.ctx.get_module(module_name)
            if m is None:
                continue
            top = next(iter(m.children(types=(ly.SNode.CONTAINER,))), None)
            if top is None:
                continue
            if top.name() != m.name():
                raise SonicYangException("topLevelContainer mismatch {}:{}".format(
                    top.name(), m.name()))
            for table in top.children(types=(ly.SNode.CONTAINER,)):
                self.confDbYangMap[table.name()] = {
                    'module': m.name(),
                    'topLevelContainer': top.name(),
                    'container': table,
                }

    """
    Get module, topLevelContainer(TLC) and container SNode for a config DB table
    """
    def _getModuleTLCcontainer(self, table):
        cmap = self.confDbYangMap
        m = cmap[table]['module']
        t = cmap[table]['topLevelContainer']
        c = cmap[table]['container']
        return m, t, c

    """
    Crop config as per yang models,
    This Function crops from config only those TABLEs, for which yang models is
    provided. If there are tables to modify it will perform a deepcopy of the
    original structure in case anyone is holding a reference.
    The Tables without YANG models are stored in self.tablesWithOutYangModels.
    """
    def _cropConfigDB(self, croppedFile=None):
        isCopy = False
        tables = list(self.jIn.keys())
        for table in tables:
            if table not in self.confDbYangMap:
                # Make sure we duplicate if we're modifying so if a caller
                # has a reference we don't clobber it.
                if not isCopy:
                    isCopy = True
                    self.jIn = copy.deepcopy(self.jIn)
                # store in tablesWithOutYang and purge
                self.tablesWithOutYang[table] = self.jIn[table]
                del self.jIn[table]

        if len(self.tablesWithOutYang):
            self.sysLog(msg=f"Note: Below table(s) have no YANG models: {', '.join(self.tablesWithOutYang)}", doPrint=True)

        if croppedFile:
            with open(croppedFile, 'w') as f:
                dump(self.jIn, f, indent=4)

        return

    """
    Extract keys from table entry in Config DB and return in a dict

    Input:
    tableKey: Config DB Primary Key, Example tableKey = "Vlan111|2a04:5555:45:6709::1/64"
    keys: key string from YANG list, i.e. 'vlan_name ip-prefix'.

    Return:
    keyDict = {"vlan_name": "Vlan111", "ip-prefix": "2a04:5555:45:6709::1/64"}
    """
    def _extractKey(self, tableKey, keys):

        keyList = keys.split()
        # get the value groups
        value = tableKey.split("|")
        # match lens
        if len(keyList) != len(value):
            raise Exception("Value not found for {} in {}".format(keys, tableKey))
        # create the keyDict
        keyDict = dict()
        for i in range(len(keyList)):
            keyDict[keyList[i]] = value[i].strip()

        return keyDict

    def _createLeafDict(self, model_snode, table):
        """
        Build a name-keyed dict of leaf and leaf-list SNodes directly under
        model_snode. iter_children with default options transparently descends
        into choice/case, so leaves under choice/case are flattened in.

        Used to drive ConfigDB-key → YANG-leaf mapping during xlate/revXlate.
        """
        leafDict = {}
        for child in model_snode.children(types=(ly.SNode.LEAF, ly.SNode.LEAFLIST)):
            leafDict[child.name()] = child
        return leafDict

    """
    Convert a string from Config DB value to Yang Value based on type of the
    key in Yang model.
    """
    def _findYangTypedValue(self, key, value, leafDict):
        leaf = leafDict[key]
        is_leaf_list = isinstance(leaf, ly.SLeafList)

        # convert config DB string to yang Type
        def _yangConvert(val):
            # Convert everything to string
            val = str(val)
            base = leaf.type().basename()
            # uint{8,16,32,64} → integer; everything else stays a string.
            # leafref/enumeration/identityref/string/etc. all serialise as
            # JSON strings in the yang data tree.
            if 'uint' in base:
                return int(val, 10)
            return val

        # if it is a leaf-list do it for each element
        if is_leaf_list:
            vValue = list()
            if isinstance(value, str) and (self.elementPath[0], self.elementPath[-1]) in LEAF_LIST_WITH_STRING_VALUE_DICT:
                # For field defined as leaf-list but has string value in CONFIG DB, need do special handling here. For exampe:
                # port.adv_speeds in CONFIG DB has value "100,1000,10000", it shall be transferred to [100,1000,10000] as YANG value here to
                # be compliant with yang model.
                value = value.split(LEAF_LIST_WITH_STRING_VALUE_DICT[(self.elementPath[0], self.elementPath[-1])])
            for v in value:
                vValue.append(_yangConvert(v))
            # SONiC YANG models that allow "logically empty" leaf-lists declare
            # `default ""` (e.g. ACL_TABLE.ports) so libyang accepts the empty
            # representation by auto-applying the default. libyang3, unlike
            # libyang1, silently drops an explicit `[]` instead of substituting
            # the default — which lets generic_config_updater's patch sorter
            # accept moves like `remove /ports/0` that strand the configdb
            # field as `[]`. Reject those here so the sorter falls back to
            # removing the whole field (matching the libyang1 behaviour the
            # patch_sorter_test_success fixtures were generated against).
            if len(vValue) == 0 and list(leaf.defaults()):
                raise Exception(
                    "Empty leaf-list at {}: schema declares a default; "
                    "remove the field instead of leaving it empty".format(
                        '/'.join(self.elementPath)))
        else:
            vValue = _yangConvert(value)

        return vValue

    """
    Xlate a Type 1 map list
    This function will xlate from a dict in config DB to a Yang JSON list
    using yang model. Output will be go in self.xlateJson

    Note: Exceptions from this function are collected in exceptionList and
    are displayed only when an entry is not xlated properly from ConfigDB
    to sonic_yang.json.

    Type 1 lists have inner list, each inner list key:val should
    be mapped to field:value in Config DB.
    Example:
    Yang Model:
        list DSCP_TO_TC_MAP_LIST {
            description "DSCP_TO_TC_MAP part of config_db.json";
            key "name";
            leaf name {
                type string;
            }
            list DSCP_TO_TC_MAP { ... }
        }
    Config DB:
    "DSCP_TO_TC_MAP": {
       "Dscp_to_tc_map1": {
          "1": "1",
          "2": "2"
       }
    }
    """
    def _xlateType1MapList(self, list_snode, yang, config, table, exceptionList):

        # Inner list (e.g. DSCP_TO_TC_MAP inside DSCP_TO_TC_MAP_LIST). Type 1
        # maps always have at most one inner list per the model convention.
        inner_clist = next(iter(list_snode.children(types=(ly.SNode.LIST,))), None)
        inner_listKey = ""
        inner_listVal = ""
        if inner_clist is not None:
            inner_keys = [k.name() for k in inner_clist.keys()]
            inner_listKey = inner_keys[0] if inner_keys else ""
            inner_leafDict = self._createLeafDict(inner_clist, table)
            for lkey in inner_leafDict:
                if inner_listKey != lkey:
                    inner_listVal = lkey

        # get keys from YANG model list itself
        listKeys = " ".join(k.name() for k in list_snode.keys())
        self.sysLog(msg="xlateList keyList:{}".format(listKeys))
        primaryKeys = list(config.keys())
        for pkey in primaryKeys:
            try:
                vKey = None
                self.sysLog(syslog.LOG_DEBUG, "xlateList Extract pkey:{}".\
                    format(pkey))
                # Find and extracts key from each dict in config
                keyDict = self._extractKey(pkey, listKeys)

                inner_yang_list = list()
                if inner_clist is not None:
                    for vKey in config[pkey]:
                        inner_keyDict = dict()
                        self.sysLog(syslog.LOG_DEBUG, "xlateList Key {} vkey {} Val {} vval {}".\
                            format(inner_listKey, str(vKey), inner_listVal, str(config[pkey][vKey])))
                        inner_keyDict[inner_listKey] = str(vKey)
                        inner_keyDict[inner_listVal] = str(config[pkey][vKey])
                        inner_yang_list.append(inner_keyDict)

                    keyDict[inner_clist.name()] = inner_yang_list
                yang.append(keyDict)
                # delete pkey from config, done to match one key with one list
                del config[pkey]

            except Exception as e:
                # log debug, because this exception may occur with multilists
                self.sysLog(msg="xlateList Exception:{}".format(str(e)), \
                    debug=syslog.LOG_DEBUG, doPrint=True)
                exceptionList.append(str(e))
                # with multilist, we continue matching other keys.
                continue
        return

    """
    Process container inside a List.
    This function will call xlateContainer based on Container(s) present
    in outer List.
    """
    def _xlateContainerInList(self, ccontainer, yang, configC, table):
        ccName = ccontainer.name()
        if ccName not in configC:
            # Inner container doesn't exist in config
            return

        if bool(configC[ccName]):
            # Empty container - return
            return

        self.sysLog(msg="xlateProcessListOfContainer: {}".format(ccName))
        self.elementPath.append(ccName)
        self._xlateContainer(ccontainer, yang, configC[ccName], table)
        self.elementPath.pop()

        return

    """
    Xlate a list
    This function will xlate from a dict in config DB to a Yang JSON list
    using yang model. Output will be go in self.xlateJson

    Note: Exceptions from this function are collected in exceptionList and
    are displayed only when an entry is not xlated properly from ConfigDB
    to sonic_yang.json.
    """
    def _xlateList(self, list_snode, yang, config, table, exceptionList):

        # Type 1 lists need special handling because of inner yang list and
        # config db format.
        if list_snode.name() in Type_1_list_maps_model:
            self.sysLog(msg="_xlateType1MapList: {}".format(list_snode.name()))
            self._xlateType1MapList(list_snode, yang, config, table, exceptionList)
            return

        # Pre-build a name → SContainer map for fast container-vs-leaf dispatch
        # while iterating the configdb keys for each list entry.
        container_map = {c.name(): c for c in list_snode.children(types=(ly.SNode.CONTAINER,))}

        leafDict = self._createLeafDict(list_snode, table)
        # get keys from YANG model list itself
        listKeys = " ".join(k.name() for k in list_snode.keys())
        self.sysLog(msg="xlateList keyList:{}".format(listKeys))
        primaryKeys = list(config.keys())
        for pkey in primaryKeys:
            try:
                self.elementPath.append(pkey)
                self.sysLog(syslog.LOG_DEBUG, "xlateList Extract pkey:{}".\
                    format(pkey))
                # Find and extracts key from each dict in config
                keyDict = self._extractKey(pkey, listKeys)
                # fill rest of the values in keyDict
                for vKey in config[pkey]:
                    ccontainer = container_map.get(vKey)
                    if ccontainer is not None:
                        self.sysLog(syslog.LOG_DEBUG, "xlateList Handle container {} in list {}".\
                            format(vKey, table))
                        yangContainer = dict()
                        if bool(config):
                            self._xlateContainerInList(ccontainer, yangContainer, config[pkey], table)
                        if len(yangContainer):
                            keyDict[vKey] = yangContainer
                        continue
                    self.elementPath.append(vKey)
                    self.sysLog(syslog.LOG_DEBUG, "xlateList vkey {}".format(vKey))
                    try:
                        keyDict[vKey] = self._findYangTypedValue(vKey, \
                                            config[pkey][vKey], leafDict)
                    finally:
                        self.elementPath.pop()
                yang.append(keyDict)
                # delete pkey from config, done to match one key with one list
                del config[pkey]

            except Exception as e:
                # log debug, because this exception may occur with multilists
                self.sysLog(msg="xlateList Exception:{}".format(str(e)), \
                    debug=syslog.LOG_DEBUG, doPrint=True)
                exceptionList.append(str(e))
                # with multilist, we continue matching other keys.
                continue
            finally:
                self.elementPath.pop()

        return

    """
    Process list inside a Container.
    This function will call xlateList based on list(s) present in Container.
    """
    def _xlateListInContainer(self, modelList, yang, configC, table, exceptionList):
        yang[modelList.name()] = list()
        self.sysLog(msg="xlateProcessListOfContainer: {}".format(modelList.name()))
        self._xlateList(modelList, yang[modelList.name()], configC, table, exceptionList)
        # clean empty lists
        if len(yang[modelList.name()]) == 0:
            del yang[modelList.name()]

        return

    """
    Process container inside a Container.
    This function will call xlateContainer based on Container(s) present
    in outer Container.
    """
    def _xlateContainerInContainer(self, ccontainer, yang, configC, table):
        ccName = ccontainer.name()
        yang[ccName] = dict()
        if ccName not in configC:
            # Inner container doesn't exist in config
            return

        if len(configC[ccName]) == 0:
            # Empty container, clean config and return
            del configC[ccName]
            return
        self.sysLog(msg="xlateProcessListOfContainer: {}".format(ccName))
        self.elementPath.append(ccName)
        self._xlateContainer(ccontainer, yang[ccName], \
        configC[ccName], table)
        self.elementPath.pop()

        # clean empty container
        if len(yang[ccName]) == 0:
            del yang[ccName]
        # remove copy after processing
        del configC[ccName]

        return

    """
    Xlate a container
    This function will xlate from a dict in config DB to a Yang JSON container
    using yang model. Output will be stored in self.xlateJson
    """
    def _xlateContainer(self, model_snode, yang, config, table):

        # To Handle multiple Lists, Make a copy of config, because we delete keys
        # from config after each match. This is done to match one pkey with one list.
        configC = config.copy()
        exceptionList = list()
        lists = list(model_snode.children(types=(ly.SNode.LIST,)))
        # Single-list case: only process if its name matches "<container>_LIST"
        # (preserves original sonic-yang convention). Multi-list case: process
        # all lists regardless of name.
        if len(lists) == 1:
            if lists[0].name() == model_snode.name() + "_LIST" and bool(configC):
                self._xlateListInContainer(lists[0], yang, configC, table,
                                           exceptionList)
        elif len(lists) > 1 and bool(configC):
            for modelList in lists:
                self._xlateListInContainer(modelList, yang, configC, table,
                                           exceptionList)

        # Handle container(s) in container
        if bool(configC):
            for modelContainer in model_snode.children(types=(ly.SNode.CONTAINER,)):
                self._xlateContainerInContainer(modelContainer, yang, configC, table)

        ## Handle other leaves in container,
        leafDict = self._createLeafDict(model_snode, table)
        vKeys = list(configC.keys())
        for vKey in vKeys:
            #vkey must be a leaf\leaf-list\choice in container
            if leafDict.get(vKey):
                self.elementPath.append(vKey)
                self.sysLog(syslog.LOG_DEBUG, "xlateContainer vkey {}".format(vKey))
                yang[vKey] = self._findYangTypedValue(vKey, configC[vKey], leafDict)
                self.elementPath.pop()
                # delete entry from copy of config
                del configC[vKey]

        # All entries in copy of config must have been parsed.
        if len(configC):
            self.sysLog(msg="All Keys are not parsed in {}\n{}".format(table, \
                configC.keys()), debug=syslog.LOG_ERR, doPrint=True)
            self.sysLog(msg="exceptionList:{}".format(exceptionList), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise(Exception("All Keys are not parsed in {}\n{}\nexceptionList:{}".format(table, \
                configC.keys(), exceptionList)))

        return

    """
    xlate ConfigDB json to Yang json
    """
    def _xlateConfigDBtoYang(self, jIn, yangJ):

        # find top level container for each table, and run the xlate_container.
        for table in jIn.keys():
            cmap = self.confDbYangMap[table]
            # create top level containers
            key = cmap['module']+":"+cmap['topLevelContainer']
            subkey = cmap['topLevelContainer']+":"+cmap['container'].name()
            # Add new top level container for first table in this container
            yangJ[key] = dict() if yangJ.get(key) is None else yangJ[key]
            yangJ[key][subkey] = dict()
            self.sysLog(msg="xlateConfigDBtoYang {}:{}".format(key, subkey))
            self.elementPath.append(table)
            self._xlateContainer(cmap['container'], yangJ[key][subkey], \
                                jIn[table], table)
            self.elementPath = []

        return

    """
    Read config file and crop it as per yang models
    """
    def _xlateConfigDB(self, xlateFile=None):

        jIn= self.jIn
        yangJ = self.xlateJson
        # xlation is written in self.xlateJson
        self._xlateConfigDBtoYang(jIn, yangJ)

        if xlateFile:
            with open(xlateFile, 'w') as f:
                dump(self.xlateJson, f, indent=4)

        return

    """
    create config DB table key from entry in yang JSON
    """
    def _createKey(self, entry, keys):

        keyDict = dict()
        keyList = keys.split()
        keyV = ""

        for key in keyList:
            val = entry.get(key)
            if val:
                keyDict[key] = sval = str(val)
                keyV += sval + "|"
            else:
                raise Exception("key {} not found in entry".format(key))
        keyV = keyV.rstrip("|")

        return keyV, keyDict

    """
    Convert a string from Config DB value to Yang Value based on type of the
    key in Yang model.
    """
    def _revFindYangTypedValue(self, key, value, leafDict):
        leaf = leafDict[key]
        is_leaf_list = isinstance(leaf, ly.SLeafList)

        # convert yang Type to config DB string
        def _revYangConvert(val):
            # config DB has only strings, thank god for that :), wait not yet!!!
            return str(val)

        # if it is a leaf-list do it for each element
        if is_leaf_list:
            if isinstance(value, list) and (self.elementPath[0], self.elementPath[-1]) in LEAF_LIST_WITH_STRING_VALUE_DICT:
                # For field defined as leaf-list but has string value in CONFIG DB, we need do special handling here:
                # e.g. port.adv_speeds is [10,100,1000] in YANG, need to convert it into a string for CONFIG DB: "10,100,1000"
                vValue = LEAF_LIST_WITH_STRING_VALUE_DICT[(self.elementPath[0], self.elementPath[-1])].join((_revYangConvert(x) for x in value))
            else:
                vValue = list()
                for v in value:
                    vValue.append(_revYangConvert(v))
        elif leaf.type().basename() == 'boolean':
            vValue = 'true' if value else 'false'
        else:
            vValue = _revYangConvert(value)

        return vValue

    """
    Rev xlate from <TABLE>_LIST to table in config DB
    Type 1 Lists have inner list, each inner list key:val should
    be mapped to field:value in Config DB.
    Example:

    YANG:
    module: sonic-dscp-tc-map
    +--rw sonic-dscp-tc-map
     +--rw DSCP_TO_TC_MAP
        +--rw DSCP_TO_TC_MAP_LIST* [name]
           +--rw name              string
           +--rw DSCP_TO_TC_MAP* [dscp]
              +--rw dscp    string
              +--rw tc?     string

    YANG JSON:
    "sonic-dscp-tc-map:sonic-dscp-tc-map": {
            "sonic-dscp-tc-map:DSCP_TO_TC_MAP": {
                "DSCP_TO_TC_MAP_LIST": [
                    {
                        "name": "map3",
                        "DSCP_TO_TC_MAP": [
                            {
                                "dscp": "64",
                                "tc": "1"
                            },
                            {
                                "dscp":"2",
                                "tc":"2"
                            }
                        ]
                    }
                ]
            }
        }

    Config DB:
    "DSCP_TO_TC_MAP": {
       "Dscp_to_tc_map1": {
          "1": "1",
          "2": "2"
       }
    }
    """

    def _revXlateType1MapList(self, list_snode, yang, config, table):
        # get keys from YANG model list itself
        listKeys = " ".join(k.name() for k in list_snode.keys())

        # Gather inner list key and value from model
        inner_clist = next(iter(list_snode.children(types=(ly.SNode.LIST,))), None)
        inner_listKey = ""
        inner_listVal = ""
        if inner_clist is not None:
            inner_keys = [k.name() for k in inner_clist.keys()]
            inner_listKey = inner_keys[0] if inner_keys else ""
            inner_leafDict = self._createLeafDict(inner_clist, table)
            for lkey in inner_leafDict:
                if inner_listKey != lkey:
                    inner_listVal = lkey

        # list with name <NAME>_LIST should be removed,
        if "_LIST" in list_snode.name():
            for entry in yang:
                # create key of config DB table
                pkey, pkeydict = self._createKey(entry, listKeys)
                self.sysLog(syslog.LOG_DEBUG, "revXlateList pkey:{}".format(pkey))
                config[pkey]= dict()
                # fill rest of the entries
                if inner_clist is None:
                    continue
                inner_list = entry[inner_clist.name()]
                for index in range(len(inner_list)):
                    self.sysLog(syslog.LOG_DEBUG, "revXlateList fkey:{} fval {}".\
                         format(str(inner_list[index][inner_listKey]),\
                             str(inner_list[index][inner_listVal])))
                    config[pkey][str(inner_list[index][inner_listKey])] = str(inner_list[index][inner_listVal])
        return

    """
    Rev xlate from <TABLE>_LIST to table in config DB
    """
    def _revXlateList(self, list_snode, yang, config, table):

        # special processing for Type 1 Map tables.
        if list_snode.name() in Type_1_list_maps_model:
           self._revXlateType1MapList(list_snode, yang, config, table)
           return

        # Pre-build a name → SContainer map for fast container-vs-leaf dispatch
        # while iterating list-entry keys.
        container_map = {c.name(): c for c in list_snode.children(types=(ly.SNode.CONTAINER,))}

        # get keys from YANG model list itself
        listKeys = " ".join(k.name() for k in list_snode.keys())
        leafDict = self._createLeafDict(list_snode, table)

        # list with name <NAME>_LIST should be removed,
        if "_LIST" in list_snode.name():
            for entry in yang:
                # create key of config DB table
                pkey, pkeydict = self._createKey(entry, listKeys)
                self.sysLog(syslog.LOG_DEBUG, "revXlateList pkey:{}".format(pkey))
                self.elementPath.append(pkey)
                config[pkey]= dict()
                # fill rest of the entries
                for key in entry:
                    if key not in pkeydict:
                        ccontainer = container_map.get(key)
                        if ccontainer is not None:
                            self.sysLog(syslog.LOG_DEBUG, "revXlateList handle container {} in list {}".format(pkey, table))
                            self._revXlateContainerInContainer(ccontainer, entry, config[pkey], table)
                            continue

                        self.elementPath.append(key)
                        config[pkey][key] = self._revFindYangTypedValue(key, \
                            entry[key], leafDict)
                        self.elementPath.pop()
                self.elementPath.pop()

        return

    """
    Rev xlate a list inside a yang container
    """
    def _revXlateListInContainer(self, modelList, yang, config, table):
        # Pass matching list from Yang Json if exist
        if yang.get(modelList.name()):
            self.sysLog(msg="revXlateListInContainer {}".format(modelList.name()))
            self._revXlateList(modelList, yang[modelList.name()], config, table)
        return

    """
    Rev xlate a container inside a yang container
    """
    def _revXlateContainerInContainer(self, modelContainer, yang, config, table):
        # Pass matching list from Yang Json if exist
        if yang.get(modelContainer.name()):
            config[modelContainer.name()] = dict()
            self.sysLog(msg="revXlateContainerInContainer {}".format(modelContainer.name()))
            self.elementPath.append(modelContainer.name())
            self._revXlateContainer(modelContainer, yang[modelContainer.name()], \
                config[modelContainer.name()], table)
            self.elementPath.pop()
        return

    """
    Rev xlate from yang container to table in config DB
    """
    def _revXlateContainer(self, model_snode, yang, config, table):

        for modelList in model_snode.children(types=(ly.SNode.LIST,)):
            self._revXlateListInContainer(modelList, yang, config, table)

        for modelContainer in model_snode.children(types=(ly.SNode.CONTAINER,)):
            self._revXlateContainerInContainer(modelContainer, yang, config, table)

        ## Handle other leaves in container,
        leafDict = self._createLeafDict(model_snode, table)
        for vKey in yang:
            #vkey must be a leaf\leaf-list\choice in container
            if leafDict.get(vKey):
                self.sysLog(syslog.LOG_DEBUG, "revXlateContainer vkey {}".format(vKey))
                self.elementPath.append(vKey)
                config[vKey] = self._revFindYangTypedValue(vKey, yang[vKey], leafDict)
                self.elementPath.pop()

        return

    """
    rev xlate ConfigDB json to Yang json
    """
    def _revXlateYangtoConfigDB(self, yangJ, cDbJson):

        yangJ = self.xlateJson
        cDbJson = self.revXlateJson

        # find table in config DB, use name as a KEY
        for module_top in yangJ.keys():
            # module _top will be of from module:top
            for container in yangJ[module_top].keys():
                # the module_top can the format
                # moduleName:TableName or
                # TableName
                names = container.split(':')
                if len(names) > 2:
                    raise SonicYangException("Invalid Yang data file structure")
                table = names[0] if len(names) == 1 else names[1]
                cmap = self.confDbYangMap[table]
                cDbJson[table] = dict()
                self.sysLog(msg="revXlateYangtoConfigDB {}".format(table))
                self.elementPath.append(table)
                self._revXlateContainer(cmap['container'], yangJ[module_top][container], \
                    cDbJson[table], table)
                self.elementPath = []

        return

    """
    Reverse Translate tp config DB
    """
    def _revXlateConfigDB(self, revXlateFile=None):

        yangJ = self.xlateJson
        cDbJson = self.revXlateJson
        # xlation is written in self.xlateJson
        self._revXlateYangtoConfigDB(yangJ, cDbJson)

        if revXlateFile:
            with open(revXlateFile, 'w') as f:
                dump(self.revXlateJson, f, indent=4)

        return

    """
    Find a YANG list child by name within a container SNode.
    Returns the SList SNode or None.
    """
    def _findYangList(self, container_snode, listName):
        for clist in container_snode.children(types=(ly.SNode.LIST,)):
            if clist.name() == listName:
                return clist
        return None

    """
    Find xpath of the PORT Leaf in PORT container/list. Xpath of Leaf is needed,
    because only leaf can have leafrefs depend on them. (Public)
    """
    def findXpathPortLeaf(self, portName):

        try:
            table = "PORT"
            xpath = self.findXpathPort(portName)
            module, topc, container = self._getModuleTLCcontainer(table)
            ylist = self._findYangList(container, table+"_LIST")
            if ylist is None:
                raise SonicYangException("YANG list not found for {}".format(table+"_LIST"))
            keys = list(ylist.keys())
            if not keys:
                raise SonicYangException("YANG list {} has no keys".format(table+"_LIST"))
            xpath = xpath + "/" + keys[0].name()
        except Exception as e:
            self.sysLog(msg="find xpath of port Leaf failed", \
                debug = syslog.LOG_ERR, doPrint=True)
            raise SonicYangException("find xpath of port Leaf failed\n{}".format(str(e)))

        return xpath

    """
    Find xpath of PORT. (Public)
    """
    def findXpathPort(self, portName):

        try:
            table = "PORT"
            module, topc, container = self._getModuleTLCcontainer(table)
            xpath = "/" + module + ":" + topc + "/" + table

            ylist = self._findYangList(container, table+"_LIST")
            xpath = self._findXpathList(xpath, ylist, [portName])
        except Exception as e:
            self.sysLog(msg="find xpath of port failed", \
                debug = syslog.LOG_ERR, doPrint=True)
            raise SonicYangException("find xpath of port failed\n{}".format(str(e)))

        return xpath

    """
    Find xpath of a YANG LIST from keys.
    xpath: xpath up to and including parent
    list_snode: SList SNode
    keys: list of key values, in the same order as list_snode.keys()
    """
    def _findXpathList(self, xpath, list_snode, keys):

        try:
            xpath = xpath + "/" + list_snode.name()
            listKeys = [k.name() for k in list_snode.keys()]
            for i, listKey in enumerate(listKeys):
                xpath = xpath + '['+listKey+'=\''+keys[i]+'\']'
        except Exception as e:
            self.sysLog(msg="_findXpathList:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise e

        return xpath

    """
    load_data: load Config DB, crop, xlate and create data tree from it. (Public)
    input:    configdbJson - will NOT be modified
              debug Flag
              quiet  - when True, suppress the informational "Try to load Data"
                       syslog line and the "Data Loading Failed" syslog LOG_ERR
                       line on failure. The SonicYangException is still raised
                       so callers that want to recover silently (e.g. tools that
                       speculatively validate many candidate configs, like the
                       generic_config_updater patch sorter) can do so without
                       polluting /var/log/syslog with transient errors they are
                       already handling via the exception. Default False keeps
                       existing behavior for all current callers.
    returns:  True on success
    raises:   SonicYangException on failure
    """
    def loadData(self, configdbJson, debug=False, quiet=False):

       try:
          # write Translated config in file if debug enabled
          xlateFile = None
          if debug:
              xlateFile = "xlateConfig.json"
          self.jIn = configdbJson
          # reset xlate and tablesWithOutYang
          self.xlateJson = dict()
          self.tablesWithOutYang = dict()
          # self.jIn will be cropped if needed, however it will duplicate the object
          # so the original is not modified
          self._cropConfigDB()
          # xlated result will be in self.xlateJson
          self._xlateConfigDB(xlateFile=xlateFile)
          if not quiet:
              self.sysLog(msg="Try to load Data in the tree")
          self.root = self.ctx.parse_data_mem(dumps(self.xlateJson), "json", no_state=True, strict=True, json_string_datatypes=True)

       except Exception as e:
           self.root = None
           if not quiet:
               self.sysLog(msg="Data Loading Failed:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
           raise SonicYangException("Data Loading Failed\n{}".format(str(e)))

       return True

    """
    Get data from Data tree, data tree will be assigned in self.xlateJson. (Public)
    """
    def getData(self, debug=False):

        try:
            # write reverse Translated config in file if debug enabled
            revXlateFile = None
            if debug:
                revXlateFile = "revXlateConfig.json"
            self.xlateJson = loads(self._print_data_mem('JSON'))
            # reset reverse xlate
            self.revXlateJson = dict()
            # result will be stored self.revXlateJson
            self._revXlateConfigDB(revXlateFile=revXlateFile)

        except Exception as e:
            self.sysLog(msg="Get Data Tree Failed:{}".format(str(e)), \
             debug=syslog.LOG_ERR, doPrint=True)
            raise SonicYangException("Get Data Tree Failed\n{}".format(str(e)))

        return self.revXlateJson

    """
    Delete a node from data tree, if this is LEAF and KEY Delete the Parent.
    (Public)
    """
    def deleteNode(self, xpath):

        try:
            node = self._find_data_node(xpath)
            if node is None:
                raise Exception('Node {} not found'.format(xpath))

            snode = node.schema()
            # check for a leaf if it is a key. If yes delete the parent
            if isinstance(snode, ly.SLeaf) and snode.is_key():
                # try to delete parent
                nodeP = self._find_parent_data_node(xpath)
                if nodeP is None:
                    raise Exception('Parent node for {} not found'.format(xpath))
                xpathP = nodeP.path()
                if self._deleteNode(xpath=xpathP) == False:
                    raise Exception('_deleteNode failed')
                else:
                    return True

            # delete non key element
            if self._deleteNode(xpath=xpath) == False:
                raise Exception('_deleteNode failed')
        except Exception as e:
            self.sysLog(msg="deleteNode:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise SonicYangException("Failed to delete node {}\n{}".\
                format( xpath, str(e)))

        return True


    def XlateYangToConfigDB(self, yang_data):
        config_db_json = dict()
        self.xlateJson = yang_data
        self.revXlateJson = config_db_json
        self._revXlateYangtoConfigDB(yang_data, config_db_json)
        return config_db_json


    # End of class sonic_yang
