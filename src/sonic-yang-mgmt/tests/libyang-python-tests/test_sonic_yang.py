import sys
import os
import pytest
import sonic_yang as sy
import json
import glob
import logging
from ijson import items as ijson_itmes

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

logging.basicConfig(level=logging.DEBUG)
log = logging.getLogger("YANG-TEST")
log.setLevel(logging.INFO)
log.addHandler(logging.NullHandler())

class Test_SonicYang(object):
    # class vars

    @pytest.fixture(autouse=True, scope='class')
    def data(self):
        test_file = "./tests/libyang-python-tests/test_SonicYang.json"
        data = self.jsonTestParser(test_file)
        return data

    @pytest.fixture(autouse=True, scope='class')
    def yang_s(self, data):
        yang_dir = str(data['yang_dir'])
        yang_s = sy.SonicYang(yang_dir)
        return yang_s

    def jsonTestParser(self, file):
        """
        Open the json test file
        """
        with open(file) as data_file:
            data = json.load(data_file)
        return data

    """
        Get the JSON input based on func name
        and return jsonInput
    """
    def readIjsonInput(self, yang_test_file, test):
        try:
            # load test specific Dictionary, using Key = func
            # this is to avoid loading very large JSON in memory
            print(" Read JSON Section: " + test)
            jInput = ""
            with open(yang_test_file, 'rb') as f:
                jInst = ijson_itmes(f, test)
                for it in jInst:
                    jInput = jInput + json.dumps(it)
        except Exception as e:
            print("Reading Ijson failed")
            raise(e)
        return jInput

    def setup_class(self):
        pass

    def load_yang_model_file(self, yang_s, yang_dir, yang_file, module_name):
        yfile = yang_dir + yang_file
        try:
            yang_s._load_schema_module(str(yfile))
        except Exception as e:
            print(e)
            raise

    #test load and get yang module
    def test_load_yang_model_files(self, data, yang_s):
        yang_dir = data['yang_dir']
        for module in data['modules']:
            file = str(module['file'])
            module = str(module['module'])

            self.load_yang_model_file(yang_s, yang_dir, file, module)
            assert yang_s._get_module(module) is not None

    #test load non-exist yang module file
    def test_load_invalid_model_files(self, data, yang_s):
        yang_dir = data['yang_dir']
        file = "invalid.yang"
        module = "invalid"

        with pytest.raises(Exception):
             assert self.load_yang_model_file(yang_s, yang_dir, file, module)

    # test load_module_str_name on test-acl.yang
    def test_load_module_str_name(self, data, yang_s):
        yang_dir = data['yang_dir']
        file = "test-acl.yang"

        try:
            with open(f'{yang_dir}/test-acl.yang', 'r') as f:
                content = f.read()

            name = yang_s.load_module_str_name(content)
        except Exception as e:
            print(e)
            raise

        assert name == "test-acl"

    #test load yang modules in directory
    def test_load_yang_model_dir(self, data, yang_s):
        yang_dir = data['yang_dir']
        yang_s._load_schema_modules(str(yang_dir))

        for module_name in data['modules']:
            assert yang_s._get_module(str(module_name['module'])) is not None

    #test load yang modules and data files
    def test_load_yang_model_data(self, data, yang_s):
        yang_dir = str(data['yang_dir'])
        yang_files = glob.glob(yang_dir+"/*.yang")
        data_file = str(data['data_file'])
        data_merge_file = str(data['data_merge_file'])

        data_files = []
        data_files.append(data_file)
        data_files.append(data_merge_file)
        print(yang_files)
        yang_s._load_data_model(yang_dir, yang_files, data_files)

        #validate the data tree from data_merge_file is loaded
        for node in data['merged_nodes']:
            xpath = str(node['xpath'])
            value = str(node['value'])
            val = yang_s._find_data_node_value(xpath)
            assert str(val) == str(value)

    #test load data file
    def test_load_data_file(self, data, yang_s):
        data_file = str(data['data_file'])
        yang_s._load_data_file(data_file)

    #test_validate_data_tree():
    def test_validate_data_tree(self, data, yang_s):
        yang_s.validate_data_tree()

    #test find node
    def test_find_node(self, data, yang_s):
        for node in data['data_nodes']:
            expected = node['valid']
            xpath = str(node['xpath'])
            dnode = yang_s._find_data_node(xpath)

            if(expected == "True"):
                 assert dnode is not None
                 assert dnode.path() == xpath
            else:
                 assert dnode is None

    #test add node
    def test_add_node(self, data, yang_s):
        for node in data['new_nodes']:
            xpath = str(node['xpath'])
            value = node['value']
            yang_s._add_data_node(xpath, str(value))

            data_node = yang_s._find_data_node(xpath)
            assert data_node is not None

    #test find node value
    def test_find_data_node_value(self, data, yang_s):
       for node in data['node_values']:
            xpath = str(node['xpath'])
            value = str(node['value'])
            print(xpath)
            print(value)
            val = yang_s._find_data_node_value(xpath)
            assert str(val) == str(value)

    #test delete data node
    def test_delete_node(self, data, yang_s):
        for node in data['delete_nodes']:
            xpath = str(node['xpath'])
            yang_s._deleteNode(xpath)

    #test set node's value
    def test_set_datanode_value(self, data, yang_s):
        for node in data['set_nodes']:
            xpath = str(node['xpath'])
            value = node['value']
            yang_s._set_data_node_value(xpath, value)

            val = yang_s._find_data_node_value(xpath)
            assert str(val) == str(value)

    #test list of members
    def test_find_members(self, yang_s, data):
        for node in data['members']:
            members = node['members']
            xpath = str(node['xpath'])
            list = yang_s._find_data_nodes(xpath)
            assert list.sort() == members.sort()

    #get parent xpath
    def test_get_parent_data_xpath(self, yang_s, data):
        for node in data['parents']:
            xpath = str(node['xpath'])
            expected_xpath = str(node['parent'])
            path = yang_s._get_parent_data_xpath(xpath)
            assert path == expected_xpath

    #test find_data_node_schema_xpath
    def test_find_data_node_schema_xpath(self, yang_s, data):
        for node in data['schema_nodes']:
            xpath = str(node['xpath'])
            schema_xpath = str(node['value'])
            path = yang_s._find_data_node_schema_xpath(xpath)
            assert path == schema_xpath

    #test data dependencies
    def test_find_data_dependencies(self, yang_s, data):
        for node in data['dependencies']:
            xpath = str(node['xpath'])
            list = node['dependencies']
            depend = yang_s.find_data_dependencies(xpath)
            assert set(depend) == set(list)

    #test data dependencies
    def test_find_schema_dependencies(self, yang_s, data):
        for node in data['schema_dependencies']:
            xpath = str(node['xpath'])
            list = node['schema_dependencies']
            depend = yang_s.find_schema_dependencies(xpath)
            assert set(depend) == set(list)

    #test merge data tree
    def test_merge_data_tree(self, data, yang_s):
        data_merge_file = data['data_merge_file']
        yang_dir = str(data['yang_dir'])
        yang_s._merge_data(data_merge_file, yang_dir)
        #yang_s.root.print_mem(ly.LYD_JSON, ly.LYP_FORMAT)

    #test get module prefix
    def test_get_module_prefix(self, yang_s, data):
        for node in data['prefix']:
            xpath = str(node['module_name'])
            expected = node['module_prefix']
            prefix = yang_s._get_module_prefix(xpath)
            assert expected == prefix

    #test get data type
    def test_get_data_type(self, yang_s, data):
        for node in data['data_type']:
            xpath = str(node['xpath'])
            expected = node['data_type']
            expected_type = yang_s._str_to_type(expected)
            data_type = yang_s._get_data_type(xpath)
            assert expected_type == data_type

    def test_get_leafref_type(self, yang_s, data):
        for node in data['leafref_type']:
            xpath = str(node['xpath'])
            expected = node['data_type']
            expected_type = yang_s._str_to_type(expected)
            data_type = yang_s._get_leafref_type(xpath)
            assert expected_type == data_type

    def test_get_leafref_path(self, yang_s, data):
        for node in data['leafref_path']:
            xpath = str(node['xpath'])
            expected_path = node['leafref_path']
            path = yang_s._get_leafref_path(xpath)
            assert expected_path == path

    def test_get_leafref_type_schema(self, yang_s, data):
        for node in data['leafref_type_schema']:
            xpath = str(node['xpath'])
            expected = node['data_type']
            expected_type = yang_s._str_to_type(expected)
            data_type = yang_s._get_leafref_type_schema(xpath)
            assert expected_type == data_type

    def test_configdb_path_to_xpath(self, yang_s, data):
        yang_s.loadYangModel()
        for node in data['configdb_path_to_xpath']:
            configdb_path = str(node['configdb_path'])
            schema_xpath = bool(node['schema_xpath'])
            expected = node['xpath']
            received = yang_s.configdb_path_to_xpath(configdb_path, schema_xpath=schema_xpath)
            assert received == expected

    def test_xpath_to_configdb_path(self, yang_s, data):
        yang_s.loadYangModel()
        for node in data['xpath_to_configdb_path']:
            xpath = str(node['xpath'])
            expected = node['configdb_path']
            received = yang_s.xpath_to_configdb_path(xpath)
            assert received == expected

    def test_configdb_path_split(self, yang_s, data):
        def check(path, tokens):
            expected=tokens
            actual=yang_s.configdb_path_split(path)
            assert expected == actual

        check("", [])
        check("/", [])
        check("/token", ["token"])
        check("/more/than/one/token", ["more", "than", "one", "token"])
        check("/has/numbers/0/and/symbols/^", ["has", "numbers", "0", "and", "symbols", "^"])
        check("/~0/this/is/telda", ["~", "this", "is", "telda"])
        check("/~1/this/is/forward-slash", ["/", "this", "is", "forward-slash"])
        check("/\\\\/no-escaping", ["\\\\", "no-escaping"])
        check("////empty/tokens/are/ok", ["", "", "", "empty", "tokens", "are", "ok"])

    def configdb_path_join(self, yang_s, data):
        def check(tokens, path):
            expected=path
            actual=yang_s.configdb_path_join(tokens)
            assert expected == actual

        check([], "/",)
        check([""], "/",)
        check(["token"], "/token")
        check(["more", "than", "one", "token"], "/more/than/one/token")
        check(["has", "numbers", "0", "and", "symbols", "^"], "/has/numbers/0/and/symbols/^")
        check(["~", "this", "is", "telda"], "/~0/this/is/telda")
        check(["/", "this", "is", "forward-slash"], "/~1/this/is/forward-slash")
        check(["\\\\", "no-escaping"], "/\\\\/no-escaping")
        check(["", "", "", "empty", "tokens", "are", "ok"], "////empty/tokens/are/ok")
        check(["~token", "telda-not-followed-by-0-or-1"], "/~0token/telda-not-followed-by-0-or-1")

    """
    This is helper function to load YANG models for tests cases, which works
    on Real SONiC Yang models. Mainly tests  for translation and reverse
    translation.
    """
    @pytest.fixture(autouse=True, scope='class')
    def sonic_yang_data(self):
        sonic_yang_dir = "/usr/local/yang-models/"
        sonic_yang_test_file = "../sonic-yang-models/tests/files/sample_config_db.json"

        syc = sy.SonicYang(sonic_yang_dir)
        syc.loadYangModel()

        sonic_yang_data = dict()
        sonic_yang_data['yang_dir'] = sonic_yang_dir
        sonic_yang_data['test_file'] = sonic_yang_test_file
        sonic_yang_data['syc'] = syc

        return sonic_yang_data

    def test_validate_yang_models(self, sonic_yang_data):
        '''
        In this test, we validate yang models
        a.) by converting the config as per RFC 7951 using YANG Models,
        b.) by creating data tree using new YANG models and
        c.) by validating config against YANG models.
        Successful execution of these steps can be treated as
        validation of new Yang models.
        '''
        test_file = sonic_yang_data['test_file']
        syc = sonic_yang_data['syc']
        # Currently only 3 YANG files are not directly related to config, along with event YANG models
        # which are: sonic-extension.yang, sonic-types.yang and sonic-bgp-common.yang. Hard coding
        # it right now.
        # event YANG models do not map directly to config_db and are included to NON_CONFIG_YANG_FILES at run time
        # If any more such helper yang files are added, we need to update here.
        EVENT_YANG_FILES = sum(1 for yang_model in syc.yangFiles if 'sonic-events' in yang_model)
        NON_CONFIG_YANG_FILES = 3 + EVENT_YANG_FILES
        # read config
        jIn = self.readIjsonInput(test_file, 'SAMPLE_CONFIG_DB_JSON')
        jIn = json.loads(jIn)
        numTables = len(jIn)
        # load config and create Data tree
        syc.loadData(jIn)
        # check all tables are loaded and config related to all Yang Models is
        # loaded in Data tree.
        assert len(syc.jIn) == numTables
        print("{}:{}".format(len(syc.xlateJson), len(syc.yangFiles)))
        assert len(syc.xlateJson) == len(syc.yangFiles) - NON_CONFIG_YANG_FILES
        # Validate data tree
        validTree = False
        try:
            syc.validate_data_tree()
            validTree = True
        except Exception as e:
            pass
        assert validTree == True

        return

    def test_xlate_rev_xlate(self, sonic_yang_data):
        # In this test, xlation and revXlation is tested with latest Sonic
        # YANG model.
        test_file = sonic_yang_data['test_file']
        syc = sonic_yang_data['syc']

        jIn = self.readIjsonInput(test_file, 'SAMPLE_CONFIG_DB_JSON')
        jIn = json.loads(jIn)
        numTables = len(jIn)

        syc.loadData(jIn)
        # check all tables are loaded and no tables is without Yang Models
        assert len(syc.jIn) == numTables
        assert len(syc.tablesWithOutYang) == 0

        syc.getData()

        if syc.jIn and syc.jIn == syc.revXlateJson:
            print("Xlate and Rev Xlate Passed")
        else:
            print("Xlate and Rev Xlate failed")
            # print for better debugging, in case of failure.
            from jsondiff import diff
            print(diff(syc.jIn, syc.revXlateJson, syntax='symmetric'))
            # make it fail
            assert False == True

        return

    def test_table_with_no_yang(self, sonic_yang_data):
        # in this test, tables with no YANG models must be stored seperately
        # by this library.
        test_file = sonic_yang_data['test_file']
        syc = sonic_yang_data['syc']

        jIn = self.readIjsonInput(test_file, 'SAMPLE_CONFIG_DB_UNKNOWN')

        syc.loadData(json.loads(jIn))

        ty = syc.tablesWithOutYang

        assert (len(ty) and "UNKNOWN_TABLE" in ty)

        return

    def test_special_json_with_yang(self, sonic_yang_data):
        # in this test, we validate unusual json config and check if
        # loadData works successfully
        test_file = sonic_yang_data['test_file']
        syc = sonic_yang_data['syc']

        # read config
        jIn = self.readIjsonInput(test_file, 'SAMPLE_CONFIG_DB_SPECIAL_CASE')
        jIn = json.loads(jIn)

        # load config and create Data tree
        syc.loadData(jIn)

        return

    def teardown_class(self):
        pass
