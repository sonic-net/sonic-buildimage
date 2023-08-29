# This script is used to

import yang as ly
import logging
import argparse
import sys
import ijson
import json
#import sonic_yang as sy
from glob import glob
from os import listdir
from os.path import isfile, join, splitext

#Globals vars
PASS = 0
FAIL = 1
logging.basicConfig(level=logging.DEBUG)
log = logging.getLogger("YANG-TEST")
log.setLevel(logging.INFO)
log.addHandler(logging.NullHandler())

# Global functions
def printExceptionDetails():
    try:
        excType, excObj, traceBack = sys.exc_info()
        fileName = traceBack.tb_frame.f_code.co_filename
        lineNumber = traceBack.tb_lineno
        log.error(" Exception >{}< in {}:{}".format(excObj, fileName, lineNumber))
    except Exception as e:
        log.error(" Exception in printExceptionDetails")
    return

# class for YANG Model YangModelTesting
# Run function will run all the tests
# from a user given list.

class Test_yang_models:

    def initTest(self):
        self.defaultYANGFailure = {
            'Must': ['Must condition', 'not satisfied'],
            'InvalidValue': ['Invalid value'],
            'LeafRef': ['Leafref', 'non-existing'],
            'When': ['When condition', 'not satisfied'],
            'Pattern': ['pattern', 'does not satisfy'],
            'Mandatory': ['required element', 'Missing'],
            'Verify': ['verified'],
            'Range': ['does not satisfy', 'range'],
            'MinElements': ['Too few'],
            'MaxElements': ['Too many'],
            'UnknownElement': ['Unknown element'],
            'None': []
        }

        self.ExceptionTests = { }

        for test_file in glob("./tests/yang_model_tests/tests/*.json"):
            try:
                with open(test_file) as fp:
                    log.info("Loading file " + test_file)
                    data = json.load(fp)
                    for key, test in data.items():
                        eStr = []
                        if 'eStrKey' in test:
                            eStr = list(self.defaultYANGFailure[test['eStrKey']])
                        if 'eStr' in test:
                            eStr += list(test['eStr'])
                        test['eStr'] = eStr
                        log.debug("estr for " + key + " is  " + str(eStr))
                        self.ExceptionTests[key] = test
            except Exception as e:
                log.error("Failed to load file " + test_file)
                raise(e)

        self.SpecialTests = {
            'ALL_VLAN_TEST': {
                'desc': 'VLAN TEST.',
                'eStr': self.defaultYANGFailure['None']
            }
        }

        self.tests = list(self.ExceptionTests.keys())+list(self.SpecialTests.keys())
        self.yangDir = './yang-models/'
        self.testFile = {}

        for test_file in glob("./tests/yang_model_tests/tests_config/*.json"):
            try:
                with open(test_file) as fp:
                    log.info("Loading file " + test_file)
                    data = json.load(fp)
                    for key in data:
                        self.testFile[key] = test_file
            except Exception as e:
                log.error("Failed to load file " + test_file)
                raise(e)

        self.testNum = 1
        # other class vars
        # self.ctx
        return

    """
        load all YANG models before test run
    """
    def loadYangModel(self, yangDir):

        try:
            # create context
            self.ctx = ly.Context(yangDir)
            # get all files
            yangFiles = glob(yangDir +"/*.yang")
            # load yang modules
            for file in yangFiles:
                log.debug(file)
                m = self.ctx.parse_module_path(file, ly.LYS_IN_YANG)
                if m is not None:
                    log.info("module: {} is loaded successfully".format(m.name()))
                else:
                    log.info("Could not load module: {}".format(file))

        except Exception as e:
            printExceptionDetails()
            raise e
        return

    """
        Get the JSON input based on func name
        and return jsonInput
    """
    def readJsonInput(self, test):
        try:
            # load test specific Dictionary, using Key = func
            # this is to avoid loading very large JSON in memory
            log.debug(" Read JSON Section: " + test)
            jInput = ""
            test_file = self.testFile[test]
            with open(test_file, 'rb') as f:
                jInst = ijson.items(f, test)
                for it in jInst:
                    jInput = jInput + json.dumps(it)
            log.debug("Read json JIn: {}".format(jInput))
        except Exception as e:
            printExceptionDetails()
        return jInput

    """
        Log the start of a test
    """
    def logStartTest(self, desc):
        log.info("\n------------------- Test "+ str(self.testNum) +\
        ": " + desc + "---------------------")
        self.testNum = self.testNum + 1
        return

    """
        Load Config Data and return Exception as String

        Parameters:
            jInput (dict): input config to load.
            verify (dict): contains xpath, key and value. This is used to verify,
            that node tree at xpath contains correct key and value.
            Example:
            'verify': {'xpath': "/sonic-acl:sonic-acl/ACL_TABLE/ACL_TABLE_LIST\
                        [ACL_TABLE_NAME='NO-NSW-PACL-V4']/stage",
                        'key': 'sonic-acl:stage',
                        'value': 'INGRESS'
            }
    """
    def loadConfigData(self, jInput, verify=None):
        s = ""
        try:
            node = self.ctx.parse_data_mem(jInput, ly.LYD_JSON, \
            ly.LYD_OPT_CONFIG | ly.LYD_OPT_STRICT)
            # verify the data tree if asked
            if verify is not None:
                xpath = verify['xpath']
                log.info("Verify xpath: {}".format(xpath))
                set = node.find_path(xpath)
                for dnode in set.data():
                    if (xpath == dnode.path()):
                        log.info("Verify dnode: {}".format(dnode.path()))
                        data = dnode.print_mem(ly.LYD_JSON, ly.LYP_WITHSIBLINGS \
                            | ly.LYP_FORMAT | ly.LYP_WD_ALL)
                        data = json.loads(data)
                        log.info("Verify data: {}".format(data))
                        assert (data[verify['key']] == verify['value'])
                        s = 'verified'
        except Exception as e:
            s = str(e)
            log.info(s)
        return s

    """
        Run Exception Test
    """
    def runExceptionTest(self, test):
        try:
            desc = self.ExceptionTests[test]['desc']
            self.logStartTest(desc)
            jInput = self.readJsonInput(test)
            # load the data, expect a exception with must condition failure
            s = self.loadConfigData(jInput, self.ExceptionTests[test].get('verify'))

            eStr = self.ExceptionTests[test]['eStr']
            log.debug("eStr: {}".format(eStr))
            log.debug("s: {}".format(s))

            if len(eStr) == 0 and s != "":
                raise Exception("{} in not empty".format(s))
            elif (sum(1 for str in eStr if str not in s) == 0):
                log.info(desc + " Passed\n")
                return PASS
            else:
                raise Exception("Mismatch {} and {}".format(eStr, s))
        except Exception as e:
            printExceptionDetails()
        log.info(desc + " Failed\n")
        return FAIL

    """
        Run Special Tests
    """
    def runSpecialTest(self, test):
        try:
            if test == 'ALL_VLAN_TEST':
                return self.runVlanSpecialTest(test);
        except Exception as e:
            printExceptionDetails()
        log.info(desc + " Failed\n")
        return FAIL

    def runVlanSpecialTest(self, test):
        try:
            desc = self.SpecialTests[test]['desc']
            self.logStartTest(desc)
            jInput = json.loads(self.readJsonInput(test))
            # check all Vlan from 1 to 4094
            for i in range(4095):
                vlan = 'Vlan'+str(i)
                jInput["sonic-vlan:sonic-vlan"]["sonic-vlan:VLAN"]["VLAN_LIST"]\
                      [0]["name"] = vlan
                log.debug(jInput)
                s = self.loadConfigData(json.dumps(jInput))
                if s!="":
                    raise Exception("{} in not empty".format(s))
            return PASS
        except Exception as e:
            printExceptionDetails()
        log.info(desc + " Failed\n")
        return FAIL

    """
        Run all tests from list self.tests
    """
    def test_run_tests(self):
        ret = 0
        try:
            self.initTest()
            self.loadYangModel(self.yangDir)
            assert len(self.tests) != 0
            print("Tests:{}".format(self.tests))
            for test in self.tests:
                test = test.strip()
                if test in self.ExceptionTests:
                    ret = ret + self.runExceptionTest(test);
                elif test in self.SpecialTests:
                    ret = ret + self.runSpecialTest(test);
                else:
                    raise Exception("Unexpected Test")
        except Exception as e:
            ret = FAIL * len(self.tests)
            printExceptionDetails()

        assert ret == 0
        return
# End of Class

"""
    Start Here
"""
def main():
    parser = argparse.ArgumentParser(description='Script to run YANG model tests',
                                     formatter_class=argparse.RawTextHelpFormatter,
                                     epilog="""
Usage:
python yangModelTesting.py -h
""")
    parser.add_argument('-t', '--tests', type=str, \
    help='tests to run separated by comma')
    parser.add_argument('-f', '--json-file', type=str, \
    help='JSON input for tests ', required=True)
    parser.add_argument('-y', '--yang-dir', type=str, \
    help='Path to YANG models', required=True)
    parser.add_argument('-v', '--verbose-level', \
    help='Verbose mode', action='store_true')
    parser.add_argument('-l', '--list-tests', \
    help='list all tests', action='store_true')

    args = parser.parse_args()
    try:
        tests = args.tests
        jsonFile = args.json_file
        yangDir = args.yang_dir
        logLevel = args.verbose_level
        listTests = args.list_tests
        if logLevel:
            log.setLevel(logging.DEBUG)
        # Make a list
        if (tests):
            tests = tests.split(",")

        yTest = YangModelTesting(tests, yangDir, jsonFile)
        if (listTests):
            for key in yTest.ExceptionTests.keys():
                log.info(key)
            sys.exit(0)

        ret = yTest.run()
        if ret == 0:
            log.info("All Test Passed")
        sys.exit(ret)

    except Exception as e:
        printExceptionDetails()
        sys.exit(1)

    return
if __name__ == '__main__':
    main()
