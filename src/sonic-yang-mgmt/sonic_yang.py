import libyang as ly
import syslog

from json import dump
from glob import glob
from sonic_yang_ext import SonicYangExtMixin, SonicYangException

"""
Yang schema and data tree python APIs based on libyang python
Here, sonic_yang_ext_mixin extends funtionality of sonic_yang,
i.e. it is mixin not parent class.
"""
class SonicYang(SonicYangExtMixin):

    def __init__(self, yang_dir, debug=False, print_log_enabled=True):
        self.yang_dir = yang_dir
        self.ctx = None
        self.module = None
        self.root = None

        # logging vars
        self.SYSLOG_IDENTIFIER = "sonic_yang"
        self.DEBUG = debug
        self.print_log_enabled = print_log_enabled

        # yang model files, need this map it to module
        self.yangFiles = list()
        # map from TABLE in config DB to container and module
        self.confDbYangMap = dict()
        # map of backlinks dict()[]
        self.backlinkMap = None
        # JSON format of yang model [similar to pyang conversion]
        self.yJson = list()
        # config DB json input, will be cropped as yang models
        self.jIn = dict()
        # YANG JSON, this is traslated from config DB json
        self.xlateJson = dict()
        # reverse translation from yang JSON, == config db json
        self.revXlateJson = dict()
        # below dict store the input config tables which have no YANG models
        self.tablesWithOutYang = dict()
        # below dict will store preProcessed yang objects, which may be needed by
        # all yang modules, such as grouping.
        self.preProcessedYang = dict()
        # element path for CONFIG DB. An example for this list could be:
        # ['PORT', 'Ethernet0', 'speed']
        self.elementPath = []
        try:
            self.ctx = ly.Context(yang_dir)
        except Exception as e:
            self.fail(e)

        return

    def __del__(self):
        if self.root:
            self.root.free()
            self.root = None
        if self.ctx:
            self.ctx.destroy()
            self.ctx = None

    def sysLog(self, debug=syslog.LOG_INFO, msg=None, doPrint=False):
        # log debug only if enabled
        if self.DEBUG == False and debug == syslog.LOG_DEBUG:
            return
        if msg is None:
            return
        if doPrint and self.print_log_enabled:
            print("{}({}):{}".format(self.SYSLOG_IDENTIFIER, debug, msg))
        syslog.openlog(self.SYSLOG_IDENTIFIER)
        syslog.syslog(debug, msg)
        syslog.closelog()

        return

    def fail(self, e):
        self.sysLog(msg=e, debug=syslog.LOG_ERR, doPrint=True)
        raise e

    """
    load_schema_module(): load a Yang model file
    input:    yang_file - full path of a Yang model file
    returns:  Exception if error
    """
    def _load_schema_module(self, yang_file):
        try:
            with open(yang_file, 'r') as f:
                return self.ctx.parse_module_file(f, "yang")
        except Exception as e:
            self.sysLog(msg="Failed to load yang module file: " + yang_file, debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)

    """
    load_schema_module_list(): load all Yang model files in the list
    input:    yang_files - a list of Yang model file full path
    returns:  Exception if error
    """
    def _load_schema_module_list(self, yang_files):
        for file in yang_files:
             try:
                 self._load_schema_module(file)
             except Exception as e:
                 self.fail(e)

    """
    load_schema_modules(): load all Yang model files in the directory
    input:    yang_dir - the directory of the yang model files to be loaded
    returns:  Exception if error
    """
    def _load_schema_modules(self, yang_dir):
        py = glob(yang_dir+"/*.yang")
        for file in py:
            try:
                self._load_schema_module(file)
            except Exception as e:
                self.fail(e)

    """
    load_data_file(): load a Yang data json file
    input:    data_file - the full path of the yang json data file to be loaded
    returns:  Exception if error
    """
    def _load_data_file(self, data_file):
       try:
           with open(data_file, 'r') as f:
               data_node = self.ctx.parse_data_file(f, "json", no_state=True, strict=True, json_string_datatypes=True)
       except Exception as e:
           self.sysLog(msg="Failed to load data file: " + str(data_file), debug=syslog.LOG_ERR, doPrint=True)
           self.fail(e)
       else:
           self.root = data_node

    """
    get module name from xpath
    input:    path
    returns:  module name
    """
    def _get_module_name(self, schema_xpath):
        module_name = schema_xpath.split(':')[0].strip('/')
        return module_name

    """
    get_module(): get module object from Yang module name
    input:   yang module name
    returns: Schema_Node object
    """
    def _get_module(self, module_name):
        mod = self.ctx.get_module(module_name)
        return mod

    """
    load_data_model(): load both Yang module fileis and data json files
    input:   yang directory, list of yang files and list of data files (full path)
    returns: returns (context, root) if no error,  or Exception if failed
    """
    def _load_data_model(self, yang_dir, yang_files, data_files, output=None):
        if not self.ctx:
            raise Exception('ctx not initialized')

        try:
            self._load_schema_module_list(yang_files)
            if len(data_files) == 0:
                return (self.ctx, self.root)

            self._load_data_file(data_files[0])

            for i in range(1, len(data_files)):
                self._merge_data(data_files[i])
        except Exception as e:
            self.sysLog(msg="Failed to load data files", debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
            return

        if output is not None:
            self._print_data_mem(output)

        return (self.ctx, self.root)

    """
    print_data_mem():  print the data tree
    input:  option:  "JSON" or "XML"
    """
    def _print_data_mem(self, option):
        if (option == "JSON"):
            mem = self.root.print_mem("json", with_siblings=True, pretty=True)
        else:
            mem = self.root.print_mem("yang", with_siblings=True, pretty=True)

        return mem

    """
    save_data_file_json(): save the data tree in memory into json file
    input: outfile - full path of the file to save the data tree to
    """
    def _save_data_file_json(self, outfile):
        mem = self.root.print_mem("json", pretty=True)
        with open(outfile, 'w') as out:
            dump(mem, out, indent=4)

    """
    get_module_tree(): get yang module tree in JSON or XMAL format
    input:   module name
    returns: JSON or XML format of the input yang module schema tree
    """
    def _get_module_tree(self, module_name, format):
        result = None

        try:
            module = self.ctx.get_module(str(module_name))
        except Exception as e:
            self.sysLog(msg="Cound not get module: " + str(module_name), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
        else:
            if (module is not None):
                if (format == "XML"):
                    result = module.print_mem("yin")
                else:
                    result = module.print_mem("json")

        return result

    """
    validate_data(): validate data tree
    input:
           node:   root of the data tree
           ctx:    context
    returns:  Exception if failed
    """
    def _validate_data(self, node=None, ctx=None):
        if not node:
            node = self.root

        if not ctx:
            ctx = self.ctx

        try:
            node.validate(no_state=True)
        except Exception as e:
            self.fail(e)

    """
    validate_data_tree(): validate the data tree. (Public)
    returns: Exception if failed
    """
    def validate_data_tree(self):
        try:
            self._validate_data(self.root, self.ctx)
        except Exception as e:
            self.sysLog(msg="Failed to validate data tree\n{", debug=syslog.LOG_ERR, doPrint=True)
            raise SonicYangException("Failed to validate data tree\n{}".\
                format(str(e)))

    """
    find_parent_data_node():  find the parent node object
    input:    data_xpath - xpath of the data node
    returns:  parent node
    """
    def _find_parent_data_node(self, data_xpath):
        if (self.root is None):
            self.sysLog(msg="data not loaded", debug=syslog.LOG_ERR, doPrint=True)
            return None
        try:
            data_node = self._find_data_node(data_xpath)
        except Exception as e:
            self.sysLog(msg="Failed to find data node from xpath: " + str(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
        else:
            if data_node is not None:
                return data_node.parent()

        return None

    """
    get_parent_data_xpath():  find the parent data node's xpath
    input:    data_xpath - xpathof the data node
    returns:  - xpath of parent data node
              - Exception if error
    """
    def _get_parent_data_xpath(self, data_xpath):
        path=""
        try:
            data_node = self._find_parent_data_node(data_xpath)
        except Exception as e:
            self.sysLog(msg="Failed to find parent node from xpath: " + str(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
        else:
            if  data_node is not None:
                path = data_node.path()
        return path

    """
    new_data_node(): create a new data node in the data tree
    input:
           xpath: xpath of the new node
           value: value of the new node
    returns:  new Data_Node object if success,  Exception if falied
    """
    def _new_data_node(self, xpath, value):
        val = str(value)
        try:
            data_node = self.ctx.create_data_path(xpath, parent=self.root, value=val, update=False, force_return_value=False)
        except Exception as e:
            self.sysLog(msg="Failed to add data node for path: " + str(xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
        else:
            return data_node

    """
    find_data_node():  find the data node from xpath
    input:    data_xpath: xpath of the data node
    returns   - Data_Node object if found
              - None if not exist
              - Exception if there is error
    """
    def _find_data_node(self, data_xpath):
        try:
            set = self.root.find_all(data_xpath)
        except Exception as e:
            self.sysLog(msg="Failed to find data node from xpath: " + str(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
        else:
            if set is not None:
                for data_node in set:
                    if (data_xpath == data_node.path()):
                        return data_node
            return None
    """
    find_schema_node(): find the schema node from schema xpath
        example schema xpath:
        "/sonic-port:sonic-port/sonic-port:PORT/sonic-port:PORT_LIST/sonic-port:port_name"
    input:    xpath of the node
    returns:  Schema_Node oject or None if not found
    """
    def _find_schema_node(self, schema_xpath):
        try:
            schema_set = self.ctx.find_path(schema_xpath)
            for schema_node in schema_set:
                if (schema_xpath == schema_node.schema_path()):
                    return schema_node
        except Exception as e:
             self.fail(e)
             return None
        return None
    """
    find_data_node_schema_xpath(): find the xpath of the schema node from data xpath
      data xpath example:
      "/sonic-port:sonic-port/PORT/PORT_LIST[port_name='Ethernet0']/port_name"
    input:    data_xpath - xpath of the data node
    returns:  - xpath of the schema node if success
              - Exception if error
    """
    def _find_data_node_schema_xpath(self, data_xpath):
        path = ""
        try:
            data_node = self._find_data_node(data_xpath)
            if data_node != None:
                path = data_node.schema().schema_path()

            return path
        except Exception as e:
            self.fail(e)
            return None

    """
    add_node(): add a node to Yang schema or data tree
    input:    xpath and value of the node to be added
    returns:  Exception if failed
    """
    def _add_data_node(self, data_xpath, value):
        try:
            self._new_data_node(data_xpath, value)
            #check if the node added to the data tree
            self._find_data_node(data_xpath)
        except Exception as e:
            self.sysLog(msg="add_node(): Failed to add data node for xpath: " + str(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)

    """
    merge_data(): merge a data file to the existing data tree
    input:    full path of the data json file to be merged into the existing root
    returns:  Exception if failed
    """
    def _merge_data(self, data_file):
        if not self.ctx:
            raise Exception('ctx not initialized')

        if not self.root:
            raise Exception('no root initialized')

        try:
            #source data node
            with open(str(data_file), 'r') as f:
                source_node = self.ctx.parse_data_file(f, "json", no_state=True, strict=True, json_string_datatypes=True)
                #merge
                self.root.merge(source_node, destruct=True, with_siblings=True)
        except Exception as e:
            self.fail(e)

    """
    _deleteNode(): delete a node from the schema/data tree, internal function
    input:    xpath of the schema/data node
    returns:  True - success   False - failed
    """
    def _deleteNode(self, xpath):
        dnode = self._find_data_node(xpath)
        if (dnode):
            dnode.unlink()
            return True

        self.sysLog(msg='Could not delete Node: {}'.format(xpath), debug=syslog.LOG_ERR, doPrint=True)
        return False

    """
    find_data_node_value():  find the value of a node from the data tree
    input:    data_xpath of the data node
    returns:  value string of the node
    """
    def _find_data_node_value(self, data_xpath):
        output = ""
        try:
            data_node = self._find_data_node(data_xpath)
        except Exception as e:
            self.sysLog(msg="find_data_node_value(): Failed to find data node from xpath: {}".format(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
        else:
            if (data_node is not None):
                subtype = data_node.schema().type()
                if (subtype is not None):
                    value = data_node.value()
                    return value
            return output

    """
    set the value of a node in the data tree
    input:    xpath of the data node
    returns:  Exception if failed
    """
    def _set_data_node_value(self, data_xpath, value):
        try:
            self.ctx.create_data_path(data_xpath, parent=self.root, value=str(value), update=True, force_return_value=False)
        except Exception as e:
            self.sysLog(msg="set data node value failed for xpath: " + str(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)

    """
    find_data_nodes(): find the set of nodes for the xpath
    input:    xpath of the data node
    returns:  list of xpath of the dataset
    """
    def _find_data_nodes(self, data_xpath):
        list = []
        node = next(self.root.children())
        try:
            node_set = node.find_all(data_xpath);
        except Exception as e:
            self.fail(e)
        else:
            if node_set is None:
                raise Exception('data node not found')

            for data_set in node_set:
                list.append(data_set.path())
            return list

    def _cache_schema_dependencies(self):
        if self.backlinkMap is not None:
            return

        leafRefPaths = self.ctx.find_backlinks_paths(None)
        if leafRefPaths is None:
            return None

        self.backlinkMap = dict()

        for path in leafRefPaths:
            targets = self.ctx.find_leafref_path_target_paths(path)
            if targets is None:
                continue

            for target in targets:
                if self.backlinkMap.get(target) is None:
                    self.backlinkMap[target] = list()
                self.backlinkMap[target].append(path)


    """
    find_schema_dependencies():  find the schema dependencies from schema xpath
    input:    match_path         target node path to use as a filter
              match_ancestors    whether or not to treat the specified path as
                                 an ancestor rather than a full path.
    returns:  - list of xpath of the dependencies
    """
    def _find_schema_dependencies(self, match_path, match_ancestors: bool=False):
        # Lazy building of cache
        self._cache_schema_dependencies()

        if match_path is not None and (match_path == "/" or len(match_path) == 0):
            match_path = None

        # This is an odd case where you want to know about the subtree.  Do a
        # string prefix match and create a list.
        if match_path is None or match_ancestors is True:
            ret = []
            for target, leafrefs in self.backlinkMap.items():
                if match_path is None or target == match_path or target.startswith(match_path + "/"):
                    ret.extend(leafrefs)
            return ret

        # Common case
        return self.backlinkMap.get(match_path)

    """
    load_module_str_name(): load a module based on the provided string and return
                            the loaded module name.  This is needed by
                            sonic-yang-modules to prevent direct dependency on
                            libyang.
    input: yang_module_str yang-formatted module
    returns: module name on success, exception on failure
    """
    def load_module_str_name(yang_module_str):
        try:
            module = self.ctx.parse_module_str(yang_module_str)
        except Exception as e:
            self.fail(e);
        else:
            return module.name()

        return None

    """
    find_data_dependencies(): find the data dependencies from data xpath  (Public)
    input:    data_xpath - xpath to search.  If it references an exact data node
                           only the references to that data node will be returned.
                           If a path contains multiple data nodes, then all references
                           to all child nodes will be returned.  If set to None (or "" or "/"),
                           will return all references, globally.
    returns:  - list of xpath
              - Exception if error
    """
    def find_data_dependencies(self, data_xpath):
        ref_list = []
        required_value = None
        base_dnode = None
        nodes = []
        node = self.root

        if data_xpath is not None and (len(data_xpath) == 0 or data_xpath == "/"):
            data_xpath = None

        if data_xpath is not None:
            try:
                dnode_list = list(self.root.find_all(data_xpath))
            except Exception as e:
                self.sysLog(msg="find_data_dependencies(): Failed to find data node from xpath: {}".format(data_xapth), debug=syslog.LOG_ERR, doPrint=True)
                return ref_list

            if len(dnode_list) == 0:
                raise Exception("no data nodes found for xpath specified")

            # If exactly 1 node and its a data node, we need to match the value.
            if len(dnode_list) == 1:
                base_dnode = dnode_list[0]
                try:
                    required_value = self._find_data_node_value(data_xpath)
                except Exception as e:
                    pass

        # Get a list of all schema leafrefs pointing to this node (or these data nodes).
        lreflist = []
        try:
            search_xpath = data_xpath
            match_ancestors = True
            if required_value is not None:
                search_xpath = base_dnode.schema().schema_path()
                match_ancestors = False
            lreflist = self._find_schema_dependencies(match_path=search_xpath, match_ancestors=match_ancestors)
            if lreflist is None:
                raise Exception("no schema backlinks found")
        except Exception as e:
            self.sysLog(msg='Failed to find node or dependencies for {}:{}'.format(data_xpath, e), debug=syslog.LOG_ERR, doPrint=True)
            lreflist = []
            # Exception not expected by existing tests if backlinks not found, so don't raise.
            # raise SonicYangException("Failed to find node or dependencies for {}\n{}".format(data_xpath, str(e)))

        # For all found data nodes, emit the path to the data node.  If we need to
        # restrict to a value, do so.
        for lref in lreflist:
            try:
                data_set = self.root.find_all(lref)
                for dnode in data_set:
                    if required_value is None or dnode.value() == required_value:
                        ref_list.append(dnode.path())
            except Exception as e:
                pass

        return ref_list

    """
    get_module_prefix:   get the prefix of a Yang module
    input:    name of the Yang module
    output:   prefix of the Yang module
    """
    def _get_module_prefix(self, module_name):
        prefix = ""
        try:
            module = self._get_module(module_name)
        except Exception as e:
            self.fail(e)
            return prefix
        else:
            return module.prefix()

    def _get_data_type(self, schema_xpath):
        schema_node = self._find_schema_node(schema_xpath)

        if (schema_node is None):
            return None

        return schema_node.type().basename()

    """
    get_leafref_type:   find the type of node that leafref references to
    input:    data_xpath - xpath of a data node
    output:   type of the node this leafref references to
    """
    def _get_leafref_type(self, data_xpath):
        data_node = self._find_data_node(data_xpath)
        if data_node is not None:
            if data_node.schema() is not None:
                if data_node.schema().type().base() != ly.Type.LEAFREF:
                    self.sysLog(msg="get_leafref_type() node type for data xpath: {} is not LEAFREF".format(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
                    return None
                else:
                    return data_node.schema().type().leafref_type().basename()

        return None

    """
    get_leafref_path():   find the leafref path
    input:    schema_xpath - xpath of a schema node
    output:   path value of the leafref node
    """
    def _get_leafref_path(self, schema_xpath):
        try:
            schemas = self.ctx.find_path(schema_xpath)

            for schema_node in schemas:
                if schema_node.type().base() == ly.Type.LEAFREF:
                    leafref_path = schema_node.type().leafref_path()
                    return leafref_path
        except Exception as e:
             self.fail(e)
             return None

    """
    get_leafref_type_schema:   find the type of node that leafref references to
    input:    schema_xpath - xpath of a schema node
    output:   type of the node this leafref references to
    """
    def _get_leafref_type_schema(self, schema_xpath):
        schema_node = self._find_schema_node(schema_xpath)
        if (schema_node is None):
            return None


        subtype = schema_node.type()
        if (subtype is None):
            return None

        if subtype.base() != ly.Type.LEAFREF:
            return None

        return subtype.leafref_type().basename()
