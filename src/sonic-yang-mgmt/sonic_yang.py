from __future__ import annotations

import libyang as ly
import syslog

from json import dump
from glob import glob
from typing import Optional, Dict, List, Tuple, Set, Any, cast
from sonic_yang_ext import SonicYangExtMixin, SonicYangException
from sonic_yang_path import SonicYangPathMixin

"""
Yang schema and data tree python APIs based on libyang python
Here, sonic_yang_ext_mixin extends funtionality of sonic_yang,
i.e. it is mixin not parent class.
"""
class SonicYang(SonicYangExtMixin, SonicYangPathMixin):

    def __init__(self, yang_dir: str, debug: bool = False, print_log_enabled: bool = True):
        self.yang_dir: str = yang_dir
        self.ctx: ly.Context = None  # type: ignore[assignment]
        self.module: Optional[ly.Module] = None
        self.root: Optional[ly.DNode] = None
        # logging vars
        self.SYSLOG_IDENTIFIER: str = "sonic_yang"
        self.DEBUG: bool = debug
        self.print_log_enabled: bool = print_log_enabled

        # yang model files, need this map it to module
        self.yangFiles: List[str] = list()
        # map from TABLE in config DB to container and module
        self.confDbYangMap: Dict[str, Any] = dict()
        # map of backlinks dict()[]
        self.backlinkMap: Optional[Dict[str, List[str]]] = None
        # config DB json input, will be cropped as yang models
        self.jIn: Dict[str, Any] = dict()
        # YANG JSON, this is traslated from config DB json
        self.xlateJson: Dict[str, Any] = dict()
        # reverse translation from yang JSON, == config db json
        self.revXlateJson: Dict[str, Any] = dict()
        # below dict store the input config tables which have no YANG models
        self.tablesWithOutYang: Dict[str, Any] = dict()
        # Lazy YIN-shape view of the loaded modules, rebuilt on demand from the
        # compiled SNode tree the first time a backward-compat consumer reads
        # `self.yJson`. None means "not yet built"; the loader resets this back
        # to None whenever the schema set changes. See _build_yjson_compat().
        self._yJsonCache: Optional[List[Dict[str, Any]]] = None
        # Lazy caching for must counts
        self.mustCache: Dict[Tuple[Any, bool], int] = dict()
        # Lazy caching for configdb to xpath
        self.configPathCache: Dict[Tuple[str, bool], str] = dict()
        # element path for CONFIG DB. An example for this list could be:
        # ['PORT', 'Ethernet0', 'speed']
        self.elementPath: List[str] = []
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
            self.ctx = None  # type: ignore[assignment]

    @property
    def yJson(self) -> List[Dict[str, Any]]:
        """Backward-compat YIN-shape dict tree of all loaded modules.

        Returned shape mirrors the xmltodict output that earlier versions of
        sonic-yang-mgmt produced from `module.print_mem("yin")`: a list of
        per-module dicts, each `{'module': {'@name': ..., 'container': ...}}`,
        with single children rendered as a dict and multiple as a list.

        Built on demand from the **compiled** schema tree, so `uses`,
        `grouping`, `augment` and `deviation` are already resolved by libyang3
        — there's no manual inlining and no parsed-tree access. Cached after
        first build; the cache is invalidated when the schema set changes via
        `loadYangModel()`.

        This exists solely for external consumers (e.g. sonic-utilities's
        sonic_cli_gen) that still walk the YIN dict shape. Internal code
        should walk SNode iteration instead.
        """
        if self._yJsonCache is None:
            self._yJsonCache = self._build_yjson_compat()
        return self._yJsonCache

    def _build_yjson_compat(self) -> List[Dict[str, Any]]:
        result: List[Dict[str, Any]] = []
        for module_name in self.yangFiles:
            m = self.ctx.get_module(module_name)
            if m is None:
                continue
            module_dict: Dict[str, Any] = {'@name': m.name()}
            self._yjson_add_subtree(m, module_dict)
            result.append({'module': module_dict})
        return result

    def _yjson_add_subtree(self, parent_snode: Any, parent_dict: Dict[str, Any]) -> None:
        """Add the children of `parent_snode` (a Module or SNode) to
        `parent_dict`, grouping them by YIN keyword. iter_children with
        with_choice=True yields direct leaves and choice nodes separately so
        the choice/case structure is preserved (the consumer expects to find
        leaves both directly and under `choice/case/leaf`).
        """
        groups: Dict[str, List[Dict[str, Any]]] = {}
        for child in parent_snode.children(with_choice=True):
            yin_key, yin_dict = self._yjson_node(child)
            if yin_key is None:
                continue
            groups.setdefault(yin_key, []).append(yin_dict)
        for key, values in groups.items():
            parent_dict[key] = values[0] if len(values) == 1 else values

    def _yjson_node(self, snode: Any) -> Tuple[Optional[str], Dict[str, Any]]:
        d: Dict[str, Any] = {'@name': snode.name()}
        desc = snode.description()
        if desc is not None:
            d['description'] = {'text': desc}
        if snode.mandatory():
            d['mandatory'] = {'@value': 'true'}

        if isinstance(snode, ly.SLeaf):
            d['type'] = self._yjson_type(snode.type())
            default = snode.default()
            if default is not None:
                d['default'] = {'@value': str(default)}
            return 'leaf', d
        if isinstance(snode, ly.SLeafList):
            d['type'] = self._yjson_type(snode.type())
            return 'leaf-list', d
        if isinstance(snode, ly.SList):
            keys = [k.name() for k in snode.keys()]
            if keys:
                d['key'] = {'@value': ' '.join(keys)}
            self._yjson_add_subtree(snode, d)
            return 'list', d
        if isinstance(snode, ly.SContainer):
            self._yjson_add_subtree(snode, d)
            return 'container', d
        if isinstance(snode, ly.SChoice):
            cases: List[Dict[str, Any]] = []
            for case in snode.children(with_case=True, types=(ly.SNode.CASE,)):
                case_dict: Dict[str, Any] = {'@name': case.name()}
                case_desc = case.description()
                if case_desc is not None:
                    case_dict['description'] = {'text': case_desc}
                self._yjson_add_subtree(case, case_dict)
                cases.append(case_dict)
            if cases:
                d['case'] = cases[0] if len(cases) == 1 else cases
            return 'choice', d
        # Anything else (rpc, action, notification, anyxml/anydata) — skip;
        # legacy yJson consumers don't read those branches.
        return None, d

    def _yjson_type(self, type_obj: Any) -> Dict[str, Any]:
        # Prefer the typedef-aware name (e.g. "stypes:hwsku") so consumers
        # that text-match on prefixed type names continue to work; fall back
        # to basename for built-ins.
        name = type_obj.name() or type_obj.basename()
        d: Dict[str, Any] = {'@name': name}
        base = type_obj.basename()
        if base == 'leafref':
            path = type_obj.leafref_path()
            if path:
                d['path'] = {'@value': path}
        elif base == 'union':
            members = [self._yjson_type(t) for t in type_obj.union_types()]
            if members:
                d['type'] = members[0] if len(members) == 1 else members
        return d

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
        self.sysLog(msg=str(e), debug=syslog.LOG_ERR, doPrint=True)
        raise e

    """
    load_schema_module(): load a Yang model file
    input:    yang_file - full path of a Yang model file
    returns:  Exception if error
    """
    def _load_schema_module(self, yang_file):
        try:
            # Use libyang's own FILEPATH mode rather than opening the file in
            # Python and passing a file object. This matches the libyang1
            # behaviour (parse_module_path(path)) and, equally important,
            # keeps tests that mock builtins.open working — libyang reads the
            # file via its own C runtime, so a mocked open() is irrelevant.
            return self.ctx.parse_module(yang_file, ly.IOType.FILEPATH, "yang")
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
           # Use libyang's FILEPATH mode so libyang reads the file itself —
           # avoids passing a Python file object whose fileno() may not exist
           # (e.g. when builtins.open is mocked in tests).
           data_node = self.ctx.parse_data("json", in_type=ly.IOType.FILEPATH,
                                           in_data=str(data_file),
                                           no_state=True, strict=True,
                                           json_string_datatypes=True)
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
    load_module_str_name(): load a module based on the provided string and return
                            the loaded module name.  This is needed by
                            sonic-utilities to prevent direct dependency on
                            libyang.
    input: yang_module_str yang-formatted module
    returns: module name on success, exception on failure
    """
    def load_module_str_name(self, yang_module_str):
        try:
            module = self.ctx.parse_module_str(yang_module_str)
        except Exception as e:
            self.fail(e)

        return module.name()

    """
    print_data_mem():  print the data tree
    input:  option:  "JSON" or "XML"
    """
    def _print_data_mem(self, option):
        if self.root is None:
            raise Exception("data not loaded")
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
        if self.root is None:
            raise Exception("data not loaded")
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
            self.sysLog(msg="Could not get module: " + str(module_name), debug=syslog.LOG_ERR, doPrint=True)
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
        if node is None:
            raise Exception("data not loaded")

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
        if self.root is None:
            return None
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
        "/sonic-port:sonic-port/PORT/PORT_LIST/port_name"
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
            if data_node is not None:
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
            # See _load_data_file: use FILEPATH mode so libyang reads the file
            # via its own runtime; avoids issues with mocked builtins.open in
            # tests and lets libyang handle encoding/EOL details.
            source_node = self.ctx.parse_data("json", in_type=ly.IOType.FILEPATH,
                                              in_data=str(data_file),
                                              no_state=True, strict=True,
                                              json_string_datatypes=True)
            if source_node is None:
                raise Exception("Failed to parse data file: " + str(data_file))
            # merge
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
                schema = data_node.schema()
                if isinstance(schema, (ly.SLeaf, ly.SLeafList)):
                    value = cast(ly.DLeaf, data_node).value()
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
        node = next(cast(ly.DContainer, self.root).children())
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

        leafRefPaths = self.ctx.find_backlinks_paths(None)  # type: ignore[arg-type]
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
    input:    schema_xpath     of the schema node
              match_ancestors  whether or not to treat the specified path as
                               an ancestor rather than a full path. If set to
                               true, will add recursively.
    returns:  - list of xpath of the dependencies
              - Exception if schema node not found
    """
    def find_schema_dependencies(self, schema_xpath, match_ancestors: bool=False):
        # Lazy building of cache
        self._cache_schema_dependencies()

        match_path = schema_xpath
        if match_path is not None and (match_path == "/" or len(match_path) == 0):
            match_path = None

        if self.backlinkMap is None:
            return []

        # This is an odd case where you want to know about the subtree.  Do a
        # string prefix match and create a list.
        if match_path is None or match_ancestors is True:
            ret = []
            for target, leafrefs in self.backlinkMap.items():
                if match_path is None or target == match_path or target.startswith(match_path + "/"):
                    ret.extend(leafrefs)
            return ret

        # Common case
        result = self.backlinkMap.get(match_path)
        if result is None:
            return []
        return result

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

        if schema_node is None:
            return 0

        # If not doing recursion, just return the result.  This will internally
        # cache the child so no need to update the cache ourselves
        if not match_ancestors:
            return self.__find_schema_must_count_only(schema_node)

        count = 0
        # Recurse first
        for elem in schema_node.iter_tree():
            count += self.__find_schema_must_count_only(elem)

        # Pull self
        count += self.__find_schema_must_count_only(schema_node)

        # Save in cache
        self.mustCache[key] = count

        return count

    def __find_schema_must_count_only(self, schema_node):
        # Check non-recursive cache
        key = ( schema_node.schema_path(), False )
        result = self.mustCache.get(key)
        if result is not None:
            return result

        count = 0
        musts = schema_node.musts()
        if musts is not None:
            count = len(list(musts))

        # Cache result
        self.mustCache[key] = count
        return count

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
        ref_set = set()

        if data_xpath is not None and (len(data_xpath) == 0 or data_xpath == "/"):
            data_xpath = None

        if data_xpath is None:
            return self._find_data_dependencies_global(ref_list, ref_set)

        if self.root is None:
            return ref_list

        dnode_list = []
        try:
            dnode_list = list(self.root.find_all(data_xpath))
        except Exception as e:
            # Possible no data paths matched, ignore
            pass

        if len(dnode_list) == 0:
            self.sysLog(msg="find_data_dependencies(): Failed to find data node from xpath: {}".format(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
            return ref_list

        # Iterate all matched data nodes and perform per-leaf
        # value-matched dependency lookups via DFS.
        for dnode in dnode_list:
            self._find_data_dependencies_node(dnode, ref_list, ref_set)

        return ref_list

    def _find_data_dependencies_leaf(self, dnode, ref_list, ref_set):
        """Process a single leaf/leaf-list node for backlink resolution."""
        leaf_value = None
        try:
            leaf_value = dnode.value()
        except Exception:
            return

        leaf_schema_path = dnode.schema().schema_path()
        backlinks = self.find_schema_dependencies(leaf_schema_path, match_ancestors=False)
        if not backlinks:
            return

        self._resolve_backlink_data(backlinks, leaf_value, ref_list, ref_set)

    def _find_data_dependencies_node(self, dnode: ly.DNode, ref_list: List[str], ref_set: Set[str]) -> None:
        """DFS into dnode: for each leaf descendant, find backlinks and value-match."""
        if isinstance(dnode, (ly.DLeaf, ly.DLeafList)):
            leaf_schema_path = dnode.schema().schema_path()
            backlinks = self.find_schema_dependencies(leaf_schema_path, match_ancestors=False)
            if backlinks:
                self._resolve_backlink_data(backlinks, dnode.value(), ref_list, ref_set)
            return

        # Not a leaf - recurse into children
        if isinstance(dnode, (ly.DContainer, ly.DList)):
            try:
                for child in dnode.children():
                    self._find_data_dependencies_node(child, ref_list, ref_set)
            except Exception:
                pass

    def _find_data_dependencies_global(self, ref_list, ref_set):
        """Find all data dependencies globally by iterating all data nodes."""
        if self.root is None:
            return ref_list
        try:
            for top_node in self.root.siblings():
                self._find_data_dependencies_node(top_node, ref_list, ref_set)
        except Exception:
            pass
        return ref_list

    def _resolve_backlink_data(self, lreflist, required_value, ref_list, ref_set):
        if self.root is None:
            return
        for lref in lreflist:
            try:
                for dnode in self.root.find_all(lref):
                    if required_value is not None:
                        try:
                            if isinstance(dnode, (ly.DLeaf, ly.DLeafList)) and dnode.value() != required_value:
                                continue
                        except Exception:
                            continue
                    path = dnode.path()
                    if path not in ref_set:
                        ref_set.add(path)
                        ref_list.append(path)
            except Exception:
                pass

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

    def _get_data_type(self, schema_xpath: str) -> Optional[str]:
        schema_node = self._find_schema_node(schema_xpath)

        if schema_node is None or not isinstance(schema_node, (ly.SLeaf, ly.SLeafList)):
            return None

        return schema_node.type().basename()

    """
    get_leafref_type:   find the type of node that leafref references to
    input:    data_xpath - xpath of a data node
    output:   type of the node this leafref references to
    """
    def _get_leafref_type(self, data_xpath: str) -> Optional[str]:
        data_node = self._find_data_node(data_xpath)
        if data_node is not None:
            schema = data_node.schema()
            if isinstance(schema, (ly.SLeaf, ly.SLeafList)):
                if schema.type().base() != ly.Type.LEAFREF:
                    self.sysLog(msg="get_leafref_type() node type for data xpath: {} is not LEAFREF".format(data_xpath), debug=syslog.LOG_ERR, doPrint=True)
                    return None
                else:
                    leafref = schema.type().leafref_type()
                    return leafref.basename() if leafref else None

        return None

    """
    get_leafref_path():   find the leafref path
    input:    schema_xpath - xpath of a schema node
    output:   path value of the leafref node
    """
    def _get_leafref_path(self, schema_xpath: str) -> Optional[str]:
        try:
            schemas = self.ctx.find_path(schema_xpath)

            for schema_node in schemas:
                if isinstance(schema_node, (ly.SLeaf, ly.SLeafList)):
                    if schema_node.type().base() == ly.Type.LEAFREF:
                        return schema_node.type().leafref_path()
        except Exception as e:
             self.fail(e)
             return None

    """
    get_leafref_type_schema:   find the type of node that leafref references to
    input:    schema_xpath - xpath of a schema node
    output:   type of the node this leafref references to
    """
    def _get_leafref_type_schema(self, schema_xpath: str) -> Optional[str]:
        schema_node = self._find_schema_node(schema_xpath)
        if schema_node is None or not isinstance(schema_node, (ly.SLeaf, ly.SLeafList)):
            return None

        subtype = schema_node.type()
        if subtype is None:
            return None

        if subtype.base() != ly.Type.LEAFREF:
            return None

        leafref = subtype.leafref_type()
        return leafref.basename() if leafref else None
