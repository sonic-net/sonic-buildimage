import os
import json


from bgpcfgd.template import TemplateFabric
from bgpcfgd.config import ConfigMgr
from .util import load_constants_dir_mappings


TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')


def load_tests(peer_type, template_name):
    constants = load_constants_dir_mappings()
    path = "tests/data/%s/%s" % (constants[peer_type], template_name)
    param_files = [name for name in os.listdir(path)
                   if os.path.isfile(os.path.join(path, name)) and name.startswith("param_")]
    tests = []
    for param_fname in param_files:
        casename = param_fname.replace("param_", "").replace(".json", "")
        result_fname = "result_%s.conf" % casename
        full_param_fname = os.path.join(path, param_fname)
        full_result_fname = os.path.join(path, result_fname)
        tests.append((casename, full_param_fname, full_result_fname))
    tmpl_path = os.path.join("bgpd", "templates", constants[peer_type], "%s.j2" % template_name)
    return tmpl_path, tests

def load_json(fname):
    with open(fname) as param_fp:
        raw_params = json.load(param_fp)
    params = {}
    for table_key, table_entries in raw_params.items():
        if table_key.startswith("CONFIG_DB__"):
            # convert CONFIG_DB__* entries keys into tuple if needed
            new_table_entries = {}
            for entry_key, entry_value in table_entries.items():
                if '|' in entry_key:
                    new_key = tuple(entry_key.split('|'))
                else:
                    new_key = entry_key
                new_table_entries[new_key] = entry_value
            params[table_key] = new_table_entries
        else:
            params[table_key] = table_entries
    return params

def compress_comments(raw_config):
    comment_counter = 0
    output = []
    for line in raw_config.split('\n'):
        stripped_line = line.strip()
        # Skip empty lines
        if stripped_line == '':
            pass
        # Write lines without comments
        elif not stripped_line.startswith('!'):
            if comment_counter > 0:
                output.append("!")
                comment_counter = 0
            output.append(line)
        # Write non-empty comments
        elif stripped_line.startswith('!') and len(stripped_line) > 1:
            if comment_counter > 0:
                output.append("!")
                comment_counter = 0
            output.append(line)
        # Count empty comments
        else: # stripped_line == '!'
            comment_counter += 1
    # Flush last comment if we have one
    if comment_counter > 0:
        output.append("!")
    return "\n".join(output) + "\n"

def write_result(fname, raw_result):
    with open(fname, 'w') as fp:
        raw_result_w_commpressed_comments = compress_comments(raw_result)
        fp.write(raw_result_w_commpressed_comments)

def run_tests(test_name, template_fname, tests):
    tf = TemplateFabric(TEMPLATE_PATH)
    template = tf.from_file(template_fname)
    for case_name, param_fname, result_fname in tests:
        params = load_json(param_fname)
        raw_generated_result = str(template.render(params))
        assert "None" not in raw_generated_result, "Test %s.%s" % (test_name, case_name)
        # this is used only for initial generation write_result(result_fname, raw_generated_result)
        canonical_generated_result = ConfigMgr.to_canonical(raw_generated_result)
        with open(result_fname) as result_fp:
            raw_saved_result = result_fp.read()
        canonical_saved_result = ConfigMgr.to_canonical(raw_saved_result)
        assert canonical_saved_result == canonical_generated_result, "Test %s.%s" % (test_name, case_name)

# Tests

def test_general_policies():
    test_data = load_tests("general", "policies.conf")
    run_tests("general_policies", *test_data)

def test_general_pg():
    test_data = load_tests("general", "peer-group.conf")
    run_tests("general_pg", *test_data)

def test_general_instance():
    test_data = load_tests("general", "instance.conf")
    run_tests("general_instance", *test_data)

def test_internal_policies():
    test_data = load_tests("internal", "policies.conf")
    run_tests("internal_policies", *test_data)

def test_internal_pg():
    test_data = load_tests("internal", "peer-group.conf")
    run_tests("internal_pg", *test_data)

def test_internal_instance():
    test_data = load_tests("internal", "instance.conf")
    run_tests("internal_instance", *test_data)

def test_dynamic_policies():
    test_data = load_tests("dynamic", "policies.conf")
    run_tests("dynamic_policies", *test_data)

def test_dynamic_pg():
    test_data = load_tests("dynamic", "peer-group.conf")
    run_tests("dynamic_pg", *test_data)

def test_dynamic_instance():
    test_data = load_tests("dynamic", "instance.conf")
    run_tests("dynamic_instance", *test_data)

def test_monitors_policies():
    test_data = load_tests("monitors", "policies.conf")
    run_tests("monitors_policies", *test_data)

def test_monitors_pg():
    test_data = load_tests("monitors", "peer-group.conf")
    run_tests("monitors_pg", *test_data)

def test_monitors_instance():
    test_data = load_tests("monitors", "instance.conf")
    run_tests("monitors_instance", *test_data)

def test_voq_chassis_policies():
    test_data = load_tests("voq_chassis", "policies.conf")
    run_tests("voq_chassis_policies", *test_data)

def test_voq_chassis_pg():
    test_data = load_tests("voq_chassis", "peer-group.conf")
    run_tests("voq_chassis_pg", *test_data)

def test_voq_chassis_instance():
    test_data = load_tests("voq_chassis", "instance.conf")
    run_tests("voq_chassis_instance", *test_data)

def test_sentinel_policies():
    test_data = load_tests("sentinels", "policies.conf")
    run_tests("sentinel_policies", *test_data)

def test_sentinel_pg():
    test_data = load_tests("sentinels", "peer-group.conf")
    run_tests("sentinel_pg", *test_data)

def test_sentinel_instance():
    test_data = load_tests("sentinels", "instance.conf")
    run_tests("sentinel_instance", *test_data)
