import json
import filecmp
import os
import re
import sys
import subprocess
import argparse

PY3x = sys.version_info >= (3, 0)
PYvX_DIR = "py3" if PY3x else "py2"
PYTHON_INTERPRETTER = "python3" if PY3x else "python2"
YANG_MODELS_DIR = "/usr/local/yang-models"

def tuple_to_str(tuplestr):
    """ Convert Python tuple '('elem1', 'elem2')' representation into string on the for "elem1|elem2" """
    def to_str(tupleobj):
        tupleobj = re.sub(r"([\(\)])", '"', tupleobj.group(0))
        return re.sub(r"( *, *)", '|', tupleobj).replace("'", '')

    return re.sub(r"(\(.*?\))", to_str, tuplestr)

def to_dict(dictstr):
    """ Convert string represention of Python dict into dict """
    # handle tuple elements
    dictstr = tuple_to_str(dictstr)

    return json.loads(dictstr.replace("'", '"'))

def liststr_to_dict(liststr):
    """ Convert string represention of Python list into dict with list key and sorted liststr as its value """
    # handle tuple elements
    liststr = tuple_to_str(liststr)

    list_obj = json.loads("{\"list\":" + liststr.replace("'", '"') +'}')
    list_obj["list"] = sorted(list_obj["list"])

    return list_obj

class YangWrapper(object):
    def __init__(self, path=YANG_MODELS_DIR):
        """
        sonic_yang only supports python3
        """
        if PY3x:
            import sonic_yang
            self.yang_parser = sonic_yang.SonicYang(path)
            self.yang_parser.loadYangModel()
            self.test_dir = os.path.dirname(os.path.realpath(__file__))
            self.script_file = [PYTHON_INTERPRETTER, os.path.join(self.test_dir, '..', 'sonic-cfggen')]

    def validate(self, argument):
        """
        Raise exception when yang validation failed
        """
        if PY3x and ("-m" in argument or "--preset" in argument):
            import sonic_yang
            parser=argparse.ArgumentParser(description="Render configuration file from minigraph data and jinja2 template.")
            parser.add_argument("-m", "--minigraph", help="minigraph xml file", nargs='?', const='/etc/sonic/minigraph.xml')
            parser.add_argument("-k", "--hwsku", help="HwSKU")
            parser.add_argument("-n", "--namespace", help="namespace name", nargs='?', const=None, default=None)
            parser.add_argument("-p", "--port-config", help="port config file, used with -m or -k", nargs='?', const=None)
            parser.add_argument("-S", "--hwsku-config", help="hwsku config file, used with -p and -m or -k", nargs='?', const=None)
            parser.add_argument("-j", "--json", help="additional json file input, used with -p, -S and -m or -k", nargs='?', const=None)
            parser.add_argument("-a", "--additional-data", help="addition data, in json string", nargs='?', const=None)
            parser.add_argument("--preset", help="generate sample configuration from a preset template",  nargs='?', const=None)
            args, unknown = parser.parse_known_args(argument)

            print('\n    Validating yang schema')
            if "-m" in argument:
                cmd = self.script_file + ['-m', args.minigraph]
                cmd += ['--print-data']
            elif "--preset" in argument:
                cmd = self.script_file + ['--preset', args.preset]

            if args.hwsku is not None:
                cmd += ['-k', args.hwsku]
            if args.hwsku_config is not None:
                cmd += ['-S', args.hwsku_config]
            if args.port_config is not None:
                cmd += ['-p', args.port_config]
            if args.namespace is not None:
                cmd += ['-n', args.namespace]
            if args.json is not None:
                cmd += ['-j', args.json]
            if args.additional_data is not None:
                cmd += ['-a', args.additional_data]

            output = subprocess.check_output(cmd).decode()
            try:
                self.yang_parser.loadData(configdbJson=json.loads(output))
                self.yang_parser.validate_data_tree()
            except sonic_yang.SonicYangException as e:
                print("yang data generated from %s is not valid: %s"%(args.minigraph, str(e)))
                return False
            if len(self.yang_parser.tablesWithOutYang):
                return False
        return True


def cmp(file1, file2):
    """ compare files """
    try:
        with open(file1, 'r') as f:
            obj1 = json.load(f)
        with open(file2, 'r') as f:
            obj2 = json.load(f)
        return obj1 == obj2
    except:
        return filecmp.cmp(file1, file2)

def cmp_tables(f_rcvd, f_exp):
    """ Check if the tables present in rcvd matches with exp """
    try:
        with open(f_rcvd, 'r') as f:
            rcvd = json.load(f)
        with open(f_exp, 'r') as f:
            exp = json.load(f)

        for key in rcvd.keys():
            print(key, rcvd[key], exp.get(key,{}))
            assert rcvd[key] == exp.get(key,{})
    except Exception as e:
        return False
    return True
