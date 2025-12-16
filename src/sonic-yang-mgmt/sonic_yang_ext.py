# This script is used as extension of sonic_yang class. It has methods of
# class sonic_yang. A separate file is used to avoid a single large file.

from __future__ import print_function
import yang as ly
import syslog
from json import dump, dumps, loads
from xmltodict import parse
from glob import glob
import copy
from sonic_yang_path import SonicYangPathMixin

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
    ('VNET_ROUTE_TUNNEL', 'endpoint'): ',',
    ('VNET_ROUTE_TUNNEL', 'mac_address'): ',',
    ('VNET_ROUTE_TUNNEL', 'vni'): ',',
}

"""
This is the Exception thrown out of all public function of this class.
"""
class SonicYangException(Exception):
    pass

# class sonic_yang methods, use mixin to extend sonic_yang
class SonicYangExtMixin(SonicYangPathMixin):

    """
    load all YANG models, create JSON of yang models. (Public function)
    """
    def loadYangModel(self):

        try:
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

            # load json for each yang model
            self._loadJsonYangModel()
            # create a map from config DB table to yang container
            self._createDBTableToModuleMap()
            # compile uses clause (embed into schema)
            self._compileUsesClause()
        except Exception as e:
            self.sysLog(msg="Yang Models Load failed:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise SonicYangException("Yang Models Load failed\n{}".format(str(e)))

        return True

    """
    load JSON schema format from yang models
    """
    def _loadJsonYangModel(self):

        try:
            for f in self.yangFiles:
                m = self.ctx.get_module(f)
                if m is not None:
                    xml = m.print_mem(ly.LYD_JSON, ly.LYP_FORMAT)
                    self.yJson.append(parse(xml))
                    self.sysLog(msg="Parsed Json for {}".format(m.name()))
        except Exception as e:
            self.sysLog(msg="JSON schema Load failed:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise e

        return

    def _preProcessYangGrouping(self, moduleName, module):
        '''
            PreProcess Grouping Section of YANG models, and store it in
            self.preProcessedYang['grouping'] as
            {'<moduleName>':
                {'<groupingName>':
                    [<List of Leafs>]
                }
            }

            Parameters:
                moduleName (str): name of yang module.
                module (dict): json format of yang module.

            Returns:
                void
        '''
        try:
            # create grouping dict
            if self.preProcessedYang.get('grouping') is None:
                self.preProcessedYang['grouping'] = dict()
            self.preProcessedYang['grouping'][moduleName] = dict()

            # get groupings from yang module
            groupings = module['grouping']

            # if grouping is a dict, make it a list for common processing
            if isinstance(groupings, dict):
                groupings = [groupings]

            for grouping in groupings:
                gName = grouping["@name"]
                self.preProcessedYang['grouping'][moduleName][gName] = dict()
                self.preProcessedYang['grouping'][moduleName][gName]["leaf"] = grouping.get('leaf')
                self.preProcessedYang['grouping'][moduleName][gName]["leaf-list"] = grouping.get('leaf-list')
                self.preProcessedYang['grouping'][moduleName][gName]["choice"] = grouping.get('choice')

        except Exception as e:
            self.sysLog(msg="_preProcessYangGrouping failed:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise e
        return

    # preProcesss Generic Yang Objects
    def _preProcessYang(self, moduleName, module):
        '''
            PreProcess Generic Section of YANG models by calling
            _preProcessYang<SectionName> methods.

            Parameters:
                moduleName (str): name of yang module.
                module (dict): json format of yang module.

            Returns:
                void
        '''
        try:
            # preProcesss Grouping
            if module.get('grouping') is not None:
                self._preProcessYangGrouping(moduleName, module)
        except Exception as e:
            self.sysLog(msg="_preProcessYang failed:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise e
        return

    def _compileUsesClauseRefineApply(self, refine, model):
        for item in model:
            if item["@name"] == refine["@target-node"]:
                # NOTE: We only handle 'description' and 'mandatory'
                if refine.get('description') is not None:
                    item["description"] = refine["description"]
                if refine.get('mandatory') is not None:
                    item["mandatory"] = refine["mandatory"]
                return

    def _compileUsesClauseRefine(self, refine, model):
        # Nothing to refine
        if refine is None:
            return

        # Convert to list if not already
        if not isinstance(refine, list):
            refine = [ refine ]

        # Iterate across each refine
        for r in refine:
            self._compileUsesClauseRefineApply(r, model)


    def _compileUsesClauseList(self, model, group, name, refine):
        # If group doesn't have this entry, nothing to do.
        groupobj = group.get(name)
        if groupobj is None:
            return

        # model doesn't have this entry type, create it as a list
        if model.get(name) is None:
            model[name] = []

        # model has this entry type, but its not a list, convert
        if not isinstance(model[name], list):
            model[name] = [ model[name] ]

        if isinstance(groupobj, list):
            model[name].extend(groupobj)
        else:
            model[name].append(groupobj)

        self._compileUsesClauseRefine(refine, model[name])

    def _compileUsesClauseModel(self, module, model):
        """
        Recursively process the yang schema looking for the "uses" clause under
        "container", "list", and "choice"/"case" nodes.  Merge in the "uses"
        dictionaries for leaf and leaf-list so callers don't need to try to do
        their own "uses" processing.  Remove the "uses" member when processed so
        anyone expecting it won't try to re-process.  It will just look like a
        yang model that doesn't use "uses" so shouldn't cause compatibility issues.
        """
        if isinstance(model, list):
            for item in model:
                self._compileUsesClauseModel(module, item)
            return

        for model_name in [ "container", "list", "choice", "case" ]:
            node = model.get(model_name)
            if node:
                self._compileUsesClauseModel(module, node)

        uses_s = model.get("uses")
        if not uses_s:
            return

        # Always make as a list
        if isinstance(uses_s, dict):
            uses_s = [uses_s]

        # uses Example: "@name": "bgpcmn:sonic-bgp-cmn"
        for uses in uses_s:
            # Assume ':'  means reference to another module
            if ':' in uses['@name']:
                prefix = uses['@name'].split(':')[0].strip()
                uses_module_name = self._findYangModuleFromPrefix(prefix, module)
            else:
                uses_module_name = module['@name']
            grouping = uses['@name'].split(':')[-1].strip()
            groupdata = self.preProcessedYang['grouping'][uses_module_name][grouping]

            # Merge leaf from uses
            refine = uses.get("refine")
            self._compileUsesClauseList(model, groupdata, 'leaf', refine)
            self._compileUsesClauseList(model, groupdata, 'leaf-list', refine)
            self._compileUsesClauseList(model, groupdata, 'choice', refine)

        # Delete the uses node so callers don't use it.
        del model["uses"]

    def _compileUsesClause(self):
        try:
            for module in self.yJson:
                self._compileUsesClauseModel(module["module"], module["module"])
        except Exception as e:
            traceback.print_exc()
            raise e

    def _createDBTableToModuleMap(self):
        """
        Create a map from config DB tables to container in yang model
        This module name and topLevelContainer are fetched considering YANG models are
        written using below Guidelines:
        https://github.com/Azure/SONiC/blob/master/doc/mgmt/SONiC_YANG_Model_Guidelines.md.
        """
        for j in self.yJson:
            # get module name
            moduleName = j['module']['@name']
            # preProcesss Generic Yang Objects
            self._preProcessYang(moduleName, j['module'])
            # get top level container
            topLevelContainer = j['module'].get('container')
            # if top level container is none, this is common yang files, which may
            # have definitions. Store module.
            if topLevelContainer is None:
                self.confDbYangMap[moduleName] = j['module']
                continue

            # top level container must exist for rest of the yang files and it should
            # have same name as module name.
            if topLevelContainer['@name'] != moduleName:
                raise(SonicYangException("topLevelContainer mismatch {}:{}".\
                    format(topLevelContainer['@name'], moduleName)))

            # Each container inside topLevelContainer maps to a sonic config table.
            container = topLevelContainer['container']
            # container is a list
            if isinstance(container, list):
                for c in container:
                    self.confDbYangMap[c['@name']] = {
                        "module" : moduleName,
                        "topLevelContainer": topLevelContainer['@name'],
                        "container": c,
                        "yangModule": j['module']
                        }
            # container is a dict
            else:
                self.confDbYangMap[container['@name']] = {
                    "module" : moduleName,
                    "topLevelContainer": topLevelContainer['@name'],
                    "container": container,
                    "yangModule": j['module']
                    }
        return

    """
    Get module, topLevelContainer(TLC) and json container for a config DB table
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
    KeyDict = {"vlan_name": "Vlan111", "ip-prefix": "2a04:5555:45:6709::1/64"}
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

    """
    Fill the dict based on leaf as a list or dict @model yang model object
    """
    def _fillLeafDict(self, leafs, leafDict, isleafList=False):

        if leafs is None:
            return

        # fill default values
        def _fillSteps(leaf):
            leaf['__isleafList'] = isleafList
            leafDict[leaf['@name']] = leaf
            return

        if isinstance(leafs, list):
            for leaf in leafs:
                #print("{}:{}".format(leaf['@name'], leaf))
                _fillSteps(leaf)
        else:
            #print("{}:{}".format(leaf['@name'], leaf))
            _fillSteps(leafs)

        return

    def _findYangModuleFromPrefix(self, prefix, module):
        '''
            Find yang module name from prefix used in given yang module.

            Parameters:
                prefix (str): prefix used in given yang module.
                module (dict): json format of yang module.

            Returns:
                 (str): module name or None
        '''
        try:
            # get imports
            yangImports = module.get("import");
            if yangImports is None:
                return None
            # make a list
            if isinstance(yangImports, dict):
                yangImports = [yangImports]
            # find module for given prefix
            for yImport in yangImports:
                if yImport['prefix']['@value'] == prefix:
                    return yImport['@module']
        except Exception as e:
            self.sysLog(msg="_findYangModuleFromPrefix failed:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise e
        return None

    def _createLeafDict(self, model, table):
        '''
            create a dict to map each key under primary key with a leaf in yang model.
            This is done to improve performance of mapping from values of TABLEs in
            config DB to leaf in YANG LIST.

            Parameters:
                module (dict): json format of yang module.
                table (str): config DB table, this table is being translated.

            Returns:
                 leafDict (dict): dict with leaf(s) information for List/Container
                    corresponding to config DB table.
        '''
        leafDict = dict()
        #Iterate over leaf, choices and leaf-list.
        self._fillLeafDict(model.get('leaf'), leafDict)

        #choices, this is tricky, since leafs are under cases in tree.
        choices = model.get('choice')
        if choices:
            # If single choice exists in container/list
            if isinstance(choices, dict):
                cases = choices['case']
                for case in cases:
                    self._fillLeafDict(case.get('leaf'), leafDict)
            # If multiple choices exist in container/list
            else:
                for choice in choices:
                    cases = choice['case']
                    for case in cases:
                        self._fillLeafDict(case.get('leaf'), leafDict)

        # leaf-lists
        self._fillLeafDict(model.get('leaf-list'), leafDict, True)

        return leafDict

    """
    Convert a string from Config DB value to Yang Value based on type of the
    key in Yang model.
    @model : A List of Leafs in Yang model list
    """
    def _findYangTypedValue(self, key, value, leafDict):

        # convert config DB string to yang Type
        def _yangConvert(val):
            # Convert everything to string
            val = str(val)
            # find type of this key from yang leaf
            type = leafDict[key]['type']['@name']

            if 'uint' in type:
                vValue = int(val, 10)
            # TODO: find type of leafref from schema node
            elif 'leafref' in type:
                vValue = val
            #TODO: find type in sonic-head, as of now, all are enumeration
            elif 'stypes:' in type:
                vValue = val
            else:
                vValue = val
            return vValue

        # if it is a leaf-list do it for each element
        if leafDict[key]['__isleafList']:
            vValue = list()
            if isinstance(value, str) and (self.elementPath[0], self.elementPath[-1]) in LEAF_LIST_WITH_STRING_VALUE_DICT:
                # For field defined as leaf-list but has string value in CONFIG DB, need do special handling here. For exampe:
                # port.adv_speeds in CONFIG DB has value "100,1000,10000", it shall be transferred to [100,1000,10000] as YANG value here to
                # make it align with its YANG definition.
                value = (x.strip() for x in value.split(LEAF_LIST_WITH_STRING_VALUE_DICT[(self.elementPath[0], self.elementPath[-1])]))
            for v in value:
                vValue.append(_yangConvert(v))
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

    Type 1 Lists have inner list, which is diffrent from config DB.
    Each field value in config db should be converted to inner list with
    key and value.
    Example:

    Config DB:
    "DSCP_TO_TC_MAP": {
       "Dscp_to_tc_map1": {
          "1": "1",
          "2": "2"
       }
    }

    YANG Model:
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
    """
    def _xlateType1MapList(self, model, yang, config, table, exceptionList):

        #create a dict to map each key under primary key with a dict yang model.
        #This is done to improve performance of mapping from values of TABLEs in
        #config DB to leaf in YANG LIST.
        inner_clist = model.get('list')
        if inner_clist:
            inner_listKey = inner_clist['key']['@value']
            inner_leafDict = self._createLeafDict(inner_clist, table)
            for lkey in inner_leafDict:
                if inner_listKey != lkey:
                    inner_listVal = lkey

        # get keys from YANG model list itself
        listKeys = model['key']['@value']
        self.sysLog(msg="xlateList keyList:{}".format(listKeys))
        primaryKeys = list(config.keys())
        for pkey in primaryKeys:
            try:
                vKey = None
                self.sysLog(syslog.LOG_DEBUG, "xlateList Extract pkey:{}".\
                    format(pkey))
                # Find and extracts key from each dict in config
                keyDict = self._extractKey(pkey, listKeys)

                if inner_clist:
                   inner_yang_list = list()
                   for vKey in config[pkey]:
                      inner_keyDict = dict()
                      self.sysLog(syslog.LOG_DEBUG, "xlateList Key {} vkey {} Val {} vval {}".\
                          format(inner_listKey, str(vKey), inner_listVal, str(config[pkey][vKey])))
                      inner_keyDict[inner_listKey] = str(vKey)
                      inner_keyDict[inner_listVal] = str(config[pkey][vKey])
                      inner_yang_list.append(inner_keyDict)

                keyDict[inner_clist['@name']] = inner_yang_list
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
    def _xlateContainerInList(self, model, yang, configC, table):
        ccontainer = model
        ccName = ccontainer.get('@name')
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
    def _xlateList(self, model, yang, config, table, exceptionList):

        # Type 1 lists need special handling because of inner yang list and
        # config db format.
        if model['@name'] in Type_1_list_maps_model:
            self.sysLog(msg="_xlateType1MapList: {}".format(model['@name']))
            self._xlateType1MapList(model, yang, config, table, exceptionList)
            return

        # For handling of container(s) in list
        ccontainer = model.get('container')

        #create a dict to map each key under primary key with a dict yang model.
        #This is done to improve performance of mapping from values of TABLEs in
        #config DB to leaf in YANG LIST.

        leafDict = self._createLeafDict(model, table)
        # get keys from YANG model list itself
        listKeys = model['key']['@value']
        self.sysLog(msg="xlateList keyList:{}".format(listKeys))
        primaryKeys = list(config.keys())
        for pkey in primaryKeys:
            try:
                self.elementPath.append(pkey)
                vKey = None
                self.sysLog(syslog.LOG_DEBUG, "xlateList Extract pkey:{}".\
                    format(pkey))
                # Find and extracts key from each dict in config
                keyDict = self._extractKey(pkey, listKeys)
                # fill rest of the values in keyDict
                for vKey in config[pkey]:
                    if ccontainer and vKey == ccontainer.get('@name'):
                        self.sysLog(syslog.LOG_DEBUG, "xlateList Handle container {} in list {}".\
                            format(vKey, table))
                        yangContainer = dict()
                        if isinstance(ccontainer, dict) and bool(config):
                            self._xlateContainerInList(ccontainer, yangContainer, config[pkey], table)
                        # If multi-list exists in container,
                        elif ccontainer and isinstance(ccontainer, list) and bool(config):
                            for modelContainer in ccontainer:
                                self._xlateContainerInList(modelContainer, yangContainer, config[pkey], table)
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
    def _xlateListInContainer(self, model, yang, configC, table, exceptionList):
        clist = model
        yang[clist['@name']] = list()
        self.sysLog(msg="xlateProcessListOfContainer: {}".format(clist['@name']))
        self._xlateList(clist, yang[clist['@name']], configC, table, exceptionList)
        # clean empty lists
        if len(yang[clist['@name']]) == 0:
            del yang[clist['@name']]

        return

    """
    Process container inside a Container.
    This function will call xlateContainer based on Container(s) present
    in outer Container.
    """
    def _xlateContainerInContainer(self, model, yang, configC, table):
        ccontainer = model
        ccName = ccontainer.get('@name')
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
    def _xlateContainer(self, model, yang, config, table):

        # To Handle multiple Lists, Make a copy of config, because we delete keys
        # from config after each match. This is done to match one pkey with one list.
        configC = config.copy()
        exceptionList = list()
        clist = model.get('list')
        # If single list exists in container,
        if clist and isinstance(clist, dict) and \
           clist['@name'] == model['@name']+"_LIST" and bool(configC):
                self._xlateListInContainer(clist, yang, configC, table, \
                    exceptionList)
        # If multi-list exists in container,
        elif clist and isinstance(clist, list) and bool(configC):
            for modelList in clist:
                self._xlateListInContainer(modelList, yang, configC, table, \
                    exceptionList)

        # Handle container(s) in container
        ccontainer = model.get('container')
        # If single list exists in container,
        if ccontainer and isinstance(ccontainer, dict) and bool(configC):
            self._xlateContainerInContainer(ccontainer, yang, configC, table)
        # If multi-list exists in container,
        elif ccontainer and isinstance(ccontainer, list) and bool(configC):
            for modelContainer in ccontainer:
                self._xlateContainerInContainer(modelContainer, yang, configC, table)

        ## Handle other leaves in container,
        leafDict = self._createLeafDict(model, table)
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
            subkey = cmap['topLevelContainer']+":"+cmap['container']['@name']
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
                #print("pair: {} {}".format(key, val))
                keyDict[key] = sval = str(val)
                keyV += sval + "|"
                #print("VAL: {} {}".format(regex, keyV))
            else:
                raise Exception("key {} not found in entry".format(key))
        #print("kDict {}".format(keyDict))
        keyV = keyV.rstrip("|")

        return keyV, keyDict

    """
    Convert a string from Config DB value to Yang Value based on type of the
    key in Yang model.
    @model : A List of Leafs in Yang model list
    """
    def _revFindYangTypedValue(self, key, value, leafDict):

        # convert yang Type to config DB string
        def _revYangConvert(val):
            # config DB has only strings, thank god for that :), wait not yet!!!
            return str(val)

        # if it is a leaf-list do it for each element
        if leafDict[key]['__isleafList']:
            if isinstance(value, list) and (self.elementPath[0], self.elementPath[-1]) in LEAF_LIST_WITH_STRING_VALUE_DICT:
                # For field defined as leaf-list but has string value in CONFIG DB, we need do special handling here:
                # e.g. port.adv_speeds is [10,100,1000] in YANG, need to convert it into a string for CONFIG DB: "10,100,1000"
                vValue = LEAF_LIST_WITH_STRING_VALUE_DICT[(self.elementPath[0], self.elementPath[-1])].join((_revYangConvert(x) for x in value))
            else:
                vValue = list()
                for v in value:
                    vValue.append(_revYangConvert(v))
        elif leafDict[key]['type']['@name'] == 'boolean':
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

    def _revXlateType1MapList(self, model, yang, config, table):
        # get keys from YANG model list itself
        listKeys = model['key']['@value']
        # create a dict to map each key under primary key with a dict yang model.
        # This is done to improve performance of mapping from values of TABLEs in
        # config DB to leaf in YANG LIST.

        # Gather inner list key and value from model
        inner_clist = model.get('list')
        if inner_clist:
            inner_listKey = inner_clist['key']['@value']
            inner_leafDict = self._createLeafDict(inner_clist, table)
            for lkey in inner_leafDict:
                if inner_listKey != lkey:
                    inner_listVal = lkey

        # list with name <NAME>_LIST should be removed,
        if "_LIST" in model['@name']:
            for entry in yang:
                # create key of config DB table
                pkey, pkeydict = self._createKey(entry, listKeys)
                self.sysLog(syslog.LOG_DEBUG, "revXlateList pkey:{}".format(pkey))
                config[pkey]= dict()
                # fill rest of the entries
                inner_list = entry[inner_clist['@name']]
                for index in range(len(inner_list)):
                    self.sysLog(syslog.LOG_DEBUG, "revXlateList fkey:{} fval {}".\
                         format(str(inner_list[index][inner_listKey]),\
                             str(inner_list[index][inner_listVal])))
                    config[pkey][str(inner_list[index][inner_listKey])] = str(inner_list[index][inner_listVal])
        return

    """
    Rev xlate from <TABLE>_LIST to table in config DB
    """
    def _revXlateList(self, model, yang, config, table):

        # special processing for Type 1 Map tables.
        if model['@name'] in Type_1_list_maps_model:
           self._revXlateType1MapList(model, yang, config, table)
           return

        # For handling of container(s) in list
        ccontainer = model.get('container')

        # get keys from YANG model list itself
        listKeys = model['key']['@value']
        # create a dict to map each key under primary key with a dict yang model.
        # This is done to improve performance of mapping from values of TABLEs in
        # config DB to leaf in YANG LIST.
        leafDict = self._createLeafDict(model, table)

        # list with name <NAME>_LIST should be removed,
        if "_LIST" in model['@name']:
            for entry in yang:
                # create key of config DB table
                pkey, pkeydict = self._createKey(entry, listKeys)
                self.sysLog(syslog.LOG_DEBUG, "revXlateList pkey:{}".format(pkey))
                self.elementPath.append(pkey)
                config[pkey]= dict()
                # fill rest of the entries
                for key in entry:
                    if key not in pkeydict:
                        if ccontainer and key == ccontainer['@name']:
                            self.sysLog(syslog.LOG_DEBUG, "revXlateList handle container {} in list {}".format(pkey, table))
                            # IF container has only one inner container
                            if isinstance(ccontainer, dict):
                                self._revXlateContainerInContainer(ccontainer, entry, config[pkey], table)
                            # IF container has many inner container
                            elif isinstance(ccontainer, list):
                                for modelContainer in ccontainer:
                                    self._revXlateContainerInContainer(modelContainer, entry, config[pkey], table)
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
    def _revXlateListInContainer(self, model, yang, config, table):
        modelList = model
        # Pass matching list from Yang Json if exist
        if yang.get(modelList['@name']):
            self.sysLog(msg="revXlateListInContainer {}".format(modelList['@name']))
            self._revXlateList(modelList, yang[modelList['@name']], config, table)
        return

    """
    Rev xlate a container inside a yang container
    """
    def _revXlateContainerInContainer(self, model, yang, config, table):
        modelContainer = model
        # Pass matching list from Yang Json if exist
        if yang.get(modelContainer['@name']):
            config[modelContainer['@name']] = dict()
            self.sysLog(msg="revXlateContainerInContainer {}".format(modelContainer['@name']))
            self.elementPath.append(modelContainer['@name'])
            self._revXlateContainer(modelContainer, yang[modelContainer['@name']], \
                config[modelContainer['@name']], table)
            self.elementPath.pop()
        return

    """
    Rev xlate from yang container to table in config DB
    """
    def _revXlateContainer(self, model, yang, config, table):

        # IF container has only one list
        clist = model.get('list')
        if isinstance(clist, dict):
            self._revXlateListInContainer(clist, yang, config, table)
        # IF container has lists
        elif isinstance(clist, list):
            for modelList in clist:
                self._revXlateListInContainer(modelList, yang, config, table)

        ccontainer = model.get('container')
        # IF container has only one inner container
        if isinstance(ccontainer, dict):
            self._revXlateContainerInContainer(ccontainer, yang, config, table)
        # IF container has only many inner container
        elif isinstance(ccontainer, list):
            for modelContainer in ccontainer:
                self._revXlateContainerInContainer(modelContainer, yang, config, table)

        ## Handle other leaves in container,
        leafDict = self._createLeafDict(model, table)
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
                #print("revXlate " + table)
                cmap = self.confDbYangMap[table]
                cDbJson[table] = dict()
                #print(key + "--" + subkey)
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
    Find a list in YANG Container
    c = container
    l = list name
    return: list if found else None
    """
    def _findYangList(self, container, listName):

        if isinstance(container['list'], dict):
            clist = container['list']
            if clist['@name'] == listName:
                return clist

        elif isinstance(container['list'], list):
            clist = [l for l in container['list'] if l['@name'] == listName]
            return clist[0]

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
            list = self._findYangList(container, table+"_LIST")
            xpath = xpath + "/" + list['key']['@value'].split()[0]
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

            list = self._findYangList(container, table+"_LIST")
            xpath = self._findXpathList(xpath, list, [portName])
        except Exception as e:
            self.sysLog(msg="find xpath of port failed", \
                debug = syslog.LOG_ERR, doPrint=True)
            raise SonicYangException("find xpath of port failed\n{}".format(str(e)))

        return xpath

    """
    Find xpath of a YANG LIST from keys,
    xpath: xpath till list
    list: YANG List
    keys: list of keys in YANG LIST
    """
    def _findXpathList(self, xpath, list, keys):

        try:
            # add list name in xpath
            xpath = xpath + "/" + list['@name']
            listKeys = list['key']['@value'].split()
            i = 0;
            for listKey in listKeys:
                xpath = xpath + '['+listKey+'=\''+keys[i]+'\']'
                i = i + 1
        except Exception as e:
            self.sysLog(msg="_findXpathList:{}".format(str(e)), \
                debug=syslog.LOG_ERR, doPrint=True)
            raise e

        return xpath

    """
    load_data: load Config DB, crop, xlate and create data tree from it. (Public)
    input:    configdbJson - will NOT be modified
              debug Flag
    returns:  True - success   False - failed
    """
    def loadData(self, configdbJson, debug=False):

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
          #print(self.xlateJson)
          self.sysLog(msg="Try to load Data in the tree")
          self.root = self.ctx.parse_data_mem(dumps(self.xlateJson), \
                        ly.LYD_JSON, ly.LYD_OPT_CONFIG|ly.LYD_OPT_STRICT)

       except Exception as e:
           self.root = None
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

        # These MACROS used only here, can we get it from Libyang Header ?
        try:
            LYS_LEAF = 4
            node = self._find_data_node(xpath)
            if node is None:
                raise('Node {} not found'.format(xpath))

            snode = node.schema()
            # check for a leaf if it is a key. If yes delete the parent
            if (snode.nodetype() == LYS_LEAF):
                leaf = ly.Schema_Node_Leaf(snode)
                if leaf.is_key():
                    # try to delete parent
                    nodeP = self._find_parent_data_node(xpath)
                    xpathP = nodeP.path()
                    if self._deleteNode(xpath=xpathP, node=nodeP) == False:
                        raise Exception('_deleteNode failed')
                    else:
                        return True

            # delete non key element
            if self._deleteNode(xpath=xpath, node=node) == False:
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
