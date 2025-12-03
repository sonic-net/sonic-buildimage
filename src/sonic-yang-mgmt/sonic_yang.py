import yang as ly
import syslog

from json import dump
from glob import glob
from sonic_yang_ext import SonicYangExtMixin, SonicYangException
from sonic_yang_path import SonicYangPathMixin

"""
Yang schema and data tree python APIs based on libyang python
Here, sonic_yang_ext_mixin extends funtionality of sonic_yang,
i.e. it is mixin not parent class.
"""
class SonicYang(SonicYangExtMixin, SonicYangPathMixin):

    def __init__(self, yang_dir, debug=False, print_log_enabled=True, sonic_yang_options=0):
        self.yang_dir = yang_dir
        self.ctx = None
        self.module = None
        self.root = None
        # logging vars
        self.SYSLOG_IDENTIFIER = "sonic_yang"
        self.DEBUG = debug
        self.print_log_enabled = print_log_enabled
        if not print_log_enabled:
            # The default libyang log options are ly.LY_LOLOG|ly.LY_LOSTORE_LAST.
            # Removing ly.LY_LOLOG will stop libyang from printing the logs.
            ly.set_log_options(ly.LY_LOSTORE_LAST)

        # yang model files, need this map it to module
        self.yangFiles = list()
        # map from TABLE in config DB to container and module
        self.confDbYangMap = dict()
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
        # Lazy caching for backlinks lookups
        self.backlinkCache = dict()
        # Lazy caching for must counts
        self.mustCache = dict()
        # Lazy caching for configdb to xpath
        self.configPathCache = dict()
        # element path for CONFIG DB. An example for this list could be:
        # ['PORT', 'Ethernet0', 'speed']
        self.elementPath = []
        try:
            self.ctx = ly.Context(yang_dir, sonic_yang_options)
        except Exception as e:
            self.fail(e)

        return

    def __del__(self):
        pass

    def sysLog(self, debug=syslog.LOG_INFO, msg=None, doPrint=False):
        # log debug only if enabled
        if self.DEBUG == False and debug == syslog.LOG_DEBUG:
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
            return self.ctx.parse_module_path(yang_file, ly.LYS_IN_YANG)
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
    load_schema_modules_ctx(): load all Yang model files in the directory to context: ctx
    input:    yang_dir,  context
    returns:  Exception if error, returrns context object if no error
    """
    def _load_schema_modules_ctx(self, yang_dir=None):
        if not yang_dir:
            yang_dir = self.yang_dir

        ctx = ly.Context(yang_dir)

        py = glob(yang_dir+"/*.yang")
        for file in py:
            try:
                ctx.parse_module_path(str(file), ly.LYS_IN_YANG)
            except Exception as e:
                self.sysLog(msg="Failed to parse yang module file: " + file, debug=syslog.LOG_ERR, doPrint=True)
                self.fail(e)

        return ctx

    """
    load_data_file(): load a Yang data json file
    input:    data_file - the full path of the yang json data file to be loaded
    returns:  Exception if error
    """
    def _load_data_file(self, data_file):
       try:
           data_node = self.ctx.parse_data_path(data_file, ly.LYD_JSON, ly.LYD_OPT_CONFIG | ly.LYD_OPT_STRICT)
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
        if (self.ctx is None):
            self.ctx = ly.Context(yang_dir)

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
    load_module_str_name(): load a module based on the provided string and return
                            the loaded module name.  This is needed by
                            sonic-utilities to prevent direct dependency on
                            libyang.
    input: yang_module_str yang-formatted module
    returns: module name on success, exception on failure
    """
    def load_module_str_name(self, yang_module_str):
        try:
            module = self.ctx.parse_module_mem(yang_module_str, ly.LYS_IN_YANG)
        except Exception as e:
            self.fail(e)
        
        return module.name()

    """
    print_data_mem():  print the data tree
    input:  option:  "JSON" or "XML"
    """
    def _print_data_mem(self, option):
        if (option == "JSON"):
            mem = self.root.print_mem(ly.LYD_JSON, ly.LYP_WITHSIBLINGS | ly.LYP_FORMAT)
        else:
            mem = self.root.print_mem(ly.LYD_XML, ly.LYP_WITHSIBLINGS | ly.LYP_FORMAT)

        return mem

    """
    save_data_file_json(): save the data tree in memory into json file
    input: outfile - full path of the file to save the data tree to
    """
    def _save_data_file_json(self, outfile):
        mem = self.root.print_mem(ly.LYD_JSON, ly.LYP_FORMAT)
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
            self.sysLog(msg="Could not get module: " + str(module_name), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
        else:
            if (module is not None):
                if (format == "XML"):
                    #libyang bug with format
                    result = module.print_mem(ly.LYD_JSON, ly.LYP_FORMAT)
                else:
                    result = module.print_mem(ly.LYD_XML, ly.LYP_FORMAT)

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
            node.validate(ly.LYD_OPT_CONFIG, ctx)
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
            data_node = self.root.new_path(self.ctx, xpath, val, 0, 0)
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
            set = self.root.find_path(data_xpath)
        except Exception as e:
            self.sysLog(msg="Failed to find data node from xpath: " + str(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
        else:
            if set is not None:
                for data_node in set.data():
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
            for schema_node in schema_set.schema():
                if (schema_xpath == schema_node.path()):
                    return schema_node
        except Exception as e:
             self.fail(e)
             return None
        else:
             for schema_node in schema_set.schema():
                 if schema_xpath == schema_node.path():
                     return schema_node
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
            set = self.root.find_path(data_xpath)
        except Exception as e:
            self.fail(e)
        else:
            for data_node in set.data():
                if data_xpath == data_node.path():
                    return data_node.schema().path()
            return path

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
    input:    yang model directory and full path of the data json file to be merged
    returns:  Exception if failed
    """
    def _merge_data(self, data_file, yang_dir=None):
        #load all yang models to ctx
        if not yang_dir:
            yang_dir = self.yang_dir

        try:
            ctx = self._load_schema_modules_ctx(yang_dir)

            #source data node
            source_node = ctx.parse_data_path(str(data_file), ly.LYD_JSON, ly.LYD_OPT_CONFIG | ly.LYD_OPT_STRICT)

            #merge
            self.root.merge(source_node, 0)
        except Exception as e:
            self.fail(e)

    """
    _deleteNode(): delete a node from the schema/data tree, internal function
    input:    xpath of the schema/data node
    returns:  True - success   False - failed
    """
    def _deleteNode(self, xpath=None, node=None):
        if node is None:
            node = self._find_data_node(xpath)

        if (node):
            node.unlink()
            dnode = self._find_data_node(xpath)
            if (dnode is None):
                #deleted node not found
                return True
            else:
                self.sysLog(msg='Could not delete Node', debug=syslog.LOG_ERR, doPrint=True)
                return False
        else:
            self.sysLog(msg="failed to find node, xpath: " + xpath, debug=syslog.LOG_ERR, doPrint=True)

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
                subtype = data_node.subtype()
                if (subtype is not None):
                    value = subtype.value_str()
                    return value
            return output

    """
    set the value of a node in the data tree
    input:    xpath of the data node
    returns:  Exception if failed
    """
    def _set_data_node_value(self, data_xpath, value):
        try:
            self.root.new_path(self.ctx, data_xpath, str(value), ly.LYD_ANYDATA_STRING, ly.LYD_PATH_OPT_UPDATE)
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
        node = self.root.child()
        try:
            node_set = node.find_path(data_xpath);
        except Exception as e:
            self.fail(e)
        else:
            if node_set is None:
                raise Exception('data node not found')

            for data_set in node_set.data():
                data_set.schema()
                list.append(data_set.path())
            return list

    """
    find_schema_must_count():  find the number of must clauses for the schema path
    input:    schema_xpath     of the schema node
              match_ancestors  whether or not to treat the specified path as
                               an ancestor rather than a full path.  If set to
                               true, will add recursively.
    returns:  - count of must statements encountered
              - Exception if schema node not found
    """
    def find_schema_must_count(self, schema_xpath, match_ancestors: bool=False):
        # See if we have this cached
        key = ( schema_xpath, match_ancestors )
        result = self.mustCache.get(key)
        if result is not None:
            return result

        try:
            schema_node = self._find_schema_node(schema_xpath)
        except Exception as e:
            self.sysLog(msg="Could not find the schema node from xpath: " + str(schema_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
            return 0

        # If not doing recursion, just return the result.  This will internally
        # cache the child so no need to update the cache ourselves
        if not match_ancestors:
            return self.__find_schema_must_count_only(schema_node)

        count = 0
        # Recurse first
        for elem in schema_node.tree_dfs():
            count += self.__find_schema_must_count_only(elem)

        # Pull self
        count += self.__find_schema_must_count_only(schema_node)

        # Save in cache
        self.mustCache[key] = count

        return count

    def __find_schema_must_count_only(self, schema_node):
        # Check non-recursive cache
        key = ( schema_node.path(), False )
        result = self.mustCache.get(key)
        if result is not None:
            return result

        count = 0
        if schema_node.nodetype() == ly.LYS_CONTAINER:
            schema_leaf = ly.Schema_Node_Container(schema_node)
            if schema_leaf.must() is not None:
                count += 1
        elif schema_node.nodetype() == ly.LYS_LEAF:
            schema_leaf = ly.Schema_Node_Leaf(schema_node)
            count += schema_leaf.must_size()
        elif schema_node.nodetype() == ly.LYS_LEAFLIST:
            schema_leaf = ly.Schema_Node_Leaflist(schema_node)
            count += schema_leaf.must_size()
        elif schema_node.nodetype() == ly.LYS_LIST:
            schema_leaf = ly.Schema_Node_List(schema_node)
            count += schema_leaf.must_size()

        # Cache result
        self.mustCache[key] = count
        return count

    """
    find_schema_dependencies():  find the schema dependencies from schema xpath
    input:    schema_xpath     of the schema node
              match_ancestors  whether or not to treat the specified path as
                               an ancestor rather than a full path. If set to
                               true, will add recursively.
    returns:  - list of xpath of the dependencies
              - Exception if schema node not found
    """
    def find_schema_dependencies(self, schema_xpath, match_ancestors: bool=False):
        # See if we have this cached
        key = ( schema_xpath, match_ancestors )
        result = self.backlinkCache.get(key)
        if result is not None:
            return result

        ref_list = []
        if schema_xpath is None or len(schema_xpath) == 0 or schema_xpath == "/":
            if not match_ancestors:
                return ref_list

            # Iterate across all modules, can't use "/"
            for module in self.ctx.get_module_iter():
                if module.data() is None:
                    continue

                module_list = []
                try:
                    module_list = self.find_schema_dependencies(module.data().path(), match_ancestors=match_ancestors)
                except Exception as e:
                    self.sysLog(msg=f"Exception while finding schema dependencies for module {module.name()}: {str(e)}", debug=syslog.LOG_ERR, doPrint=True)

                ref_list.extend(module_list)
            return ref_list

        try:
            schema_node = self._find_schema_node(schema_xpath)
        except Exception as e:
            self.sysLog(msg=f"Could not find the schema node from xpath: {str(schema_xpath)}: {str(e)}", debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
            return ref_list

        # If not doing recursion, just return the result.  This will internally
        # cache the child so no need to update the cache ourselves
        if not match_ancestors:
            return self.__find_schema_dependencies_only(schema_node)

        # Recurse first
        for elem in schema_node.tree_dfs():
            ref_list.extend(self.__find_schema_dependencies_only(elem))

        # Pull self
        ref_list.extend(self.__find_schema_dependencies_only(schema_node))

        # Save in cache
        self.backlinkCache[key] = ref_list

        return ref_list

    def __find_schema_dependencies_only(self, schema_node):
        # Check non-recursive cache
        key = ( schema_node.path(), False )
        result = self.backlinkCache.get(key)
        if result is not None:
            return result

        # New lookup
        ref_list = []
        schema_leaf = None
        if schema_node.nodetype() == ly.LYS_LEAF:
            schema_leaf = ly.Schema_Node_Leaf(schema_node)
        elif schema_node.nodetype() == ly.LYS_LEAFLIST:
            schema_leaf = ly.Schema_Node_Leaflist(schema_node)

        if schema_leaf is not None:
            backlinks = schema_leaf.backlinks()
            if backlinks is not None and backlinks.number() > 0:
                for link in backlinks.schema():
                    ref_list.append(link.path())

        # Cache result
        self.backlinkCache[key] = ref_list
        return ref_list

    """
    find_data_dependencies(): find the data dependencies from data xpath  (Public)
    input:    data_xpath - xpath to search.  If it references an exact data node
                           only the references to that data node will be returned.
                           If a path contains multiple data nodes, then all references
                           to all child nodes will be returned.  If set to None (or "" or "/"),
                           will return all references, globally.
    """
    def find_data_dependencies(self, data_xpath):
        ref_list = []
        required_value = None
        base_dnode = None
        search_xpath = None

        if data_xpath is None or len(data_xpath) == 0 or data_xpath == "/":
            data_xpath = None
            search_xpath = "/"

        if data_xpath is not None:
            dnode_list = []
            try:
                dnode_list = list(self.root.find_path(data_xpath).data())
            except Exception as e:
                # We don't care the reason for the failure, this is caught in 
                # the next statement.
                pass

            if len(dnode_list) == 0:
                self.sysLog(msg="find_data_dependencies(): Failed to find data node from xpath: {}".format(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
                return ref_list

            base_dnode = dnode_list[0]
            if base_dnode.schema() is None:
                return ref_list

            search_xpath = base_dnode.schema().path()

            # If exactly 1 node and it's a data node, we need to match the value.
            if len(dnode_list) == 1:
                try:
                    required_value = self._find_data_node_value(data_xpath)
                except Exception as e:
                    # Might not be a data node, ignore
                    pass

        # Get a list of all schema leafrefs pointing to this node (or these data nodes).
        lreflist = []

        try:
            match_ancestors = True
            if required_value is not None:
                match_ancestors = False

            lreflist = self.find_schema_dependencies(search_xpath, match_ancestors=match_ancestors)
            if lreflist is None:
                raise Exception("no schema backlinks found")
        except Exception as e:
            self.sysLog(msg='Failed to find node or dependencies for {}: {}'.format(data_xpath, str(e)), debug=syslog.LOG_ERR, doPrint=True)
            lreflist = []
            # Exception not expected by existing tests if backlinks not found, so don't raise.
            # raise SonicYangException("Failed to find node or dependencies for {}\n{}".format(data_xpath, str(e)))

        # For all found data nodes, emit the path to the data node.  If we need to
        # restrict to a value, do so.
        for lref in lreflist:
            try:
                data_set = self.root.find_path(lref).data()
                for dnode in data_set:
                    if required_value is None or (
                        dnode.subtype() is not None and dnode.subtype().value_str() == required_value
                    ):
                        ref_list.append(dnode.path())
            except Exception as e:
                # Possible no data paths matched, ignore
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

    """
    str_to_type:   map string to type of node
    input:    string
    output:   type
    """
    def _str_to_type(self, type_str):
           mapped_type = {
                "LY_TYPE_DER":ly.LY_TYPE_DER,
                "LY_TYPE_BINARY":ly.LY_TYPE_BINARY,
                "LY_TYPE_BITS":ly.LY_TYPE_BITS,
                "LY_TYPE_BOOL":ly.LY_TYPE_BOOL,
                "LY_TYPE_DEC64":ly.LY_TYPE_DEC64,
                "LY_TYPE_EMPTY":ly.LY_TYPE_EMPTY,
                "LY_TYPE_ENUM":ly.LY_TYPE_ENUM,
                "LY_TYPE_IDENT":ly.LY_TYPE_IDENT,
                "LY_TYPE_INST":ly.LY_TYPE_INST,
                "LY_TYPE_LEAFREF":ly.LY_TYPE_LEAFREF,
                "LY_TYPE_STRING":ly.LY_TYPE_STRING,
                "LY_TYPE_UNION":ly.LY_TYPE_UNION,
                "LY_TYPE_INT8":ly.LY_TYPE_INT8,
                "LY_TYPE_UINT8":ly.LY_TYPE_UINT8,
                "LY_TYPE_INT16":ly.LY_TYPE_INT16,
                "LY_TYPE_UINT16":ly.LY_TYPE_UINT16,
                "LY_TYPE_INT32":ly.LY_TYPE_INT32,
                "LY_TYPE_UINT32":ly.LY_TYPE_UINT32,
                "LY_TYPE_INT64":ly.LY_TYPE_INT64,
                "LY_TYPE_UINT64":ly.LY_TYPE_UINT64,
                "LY_TYPE_UNKNOWN":ly.LY_TYPE_UNKNOWN
           }

           if type_str not in mapped_type:
               return ly.LY_TYPE_UNKNOWN

           return mapped_type[type_str]

    def _get_data_type(self, schema_xpath):
        try:
            schema_node = self._find_schema_node(schema_xpath)
        except Exception as e:
            self.sysLog(msg="get_data_type(): Failed to find schema node from xpath: {}".format(schema_xpath), debug=syslog.LOG_ERR, doPrint=True)
            self.fail(e)
            return None

        if (schema_node is not None):
           return schema_node.subtype().type().base()

        return ly.LY_TYPE_UNKNOWN

    """
    get_leafref_type:   find the type of node that leafref references to
    input:    data_xpath - xpath of a data node
    output:   type of the node this leafref references to
    """
    def _get_leafref_type(self, data_xpath):
        data_node = self._find_data_node(data_xpath)
        if (data_node is not None):
            subtype = data_node.subtype()
            if (subtype is not None):
                if data_node.schema().subtype().type().base() != ly.LY_TYPE_LEAFREF:
                    self.sysLog(msg="get_leafref_type() node type for data xpath: {} is not LEAFREF".format(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
                    return ly.LY_TYPE_UNKNOWN
                else:
                    return subtype.value_type()

        return ly.LY_TYPE_UNKNOWN

    """
    get_leafref_path():   find the leafref path
    input:    schema_xpath - xpath of a schema node
    output:   path value of the leafref node
    """
    def _get_leafref_path(self, schema_xpath):
        schema_node = self._find_schema_node(schema_xpath)
        if (schema_node is not None):
            subtype = schema_node.subtype()
            if (subtype is not None):
                if subtype.type().base() != ly.LY_TYPE_LEAFREF:
                    return None
                else:
                    return subtype.type().info().lref().path()

        return None

    """
    get_leafref_type_schema:   find the type of node that leafref references to
    input:    schema_xpath - xpath of a schema node
    output:   type of the node this leafref references to
    """
    def _get_leafref_type_schema(self, schema_xpath):
        schema_node = self._find_schema_node(schema_xpath)
        if (schema_node is not None):
            subtype = schema_node.subtype()
            if (subtype is not None):
                if subtype.type().base() != ly.LY_TYPE_LEAFREF:
                    return None
                else:
                    subtype.type().info().lref().path()
                    target = subtype.type().info().lref().target()
                    target_path = target.path()
                    target_type = self._get_data_type(target_path)
                    return target_type

        return None
