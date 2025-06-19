"""
Description:
- This script provides access to Elastictest test plan API, including creating, canceling, and polling status.

Important!!!:
- This script is only used in Networking-acs-buildimage repo.
- This script is maintained by internal branch, and used by .pipelines/elastictest_template/run-test-elastictest-template.yaml in internal branch.
- Any updates to this file must be tested on all dependent pipelines to ensure compatibility and prevent disruptions.
"""

from __future__ import print_function, division

import argparse
import ast
import json
import os
import sys
import subprocess
import copy
import time

import requests
import yaml
from enum import Enum

__metaclass__ = type
BUILDIMAGE_REPO_FLAG = "buildimage"
MGMT_REPO_FLAG = "sonic-mgmt"
INTERNAL_REPO_LIST = ["Networking-acs-buildimage", "sonic-mgmt-int"]
GITHUB_SONIC_MGMT_REPO = "https://github.com/sonic-net/sonic-mgmt"
INTERNAL_SONIC_MGMT_REPO = "https://dev.azure.com/mssonic/internal/_git/sonic-mgmt-int"
PR_TEST_SCRIPTS_FILE = "pr_test_scripts.yaml"
SPECIFIC_PARAM_KEYWORD = "specific_param"
MAX_POLL_RETRY_TIMES = 10
MAX_GET_TOKEN_RETRY_TIMES = 3
TEST_PLAN_STATUS_UNSUCCESSFUL_FINISHED = ["FAILED", "CANCELLED"]
TEST_PLAN_STEP_STATUS_UNFINISHED = ["EXECUTING", None]


class PollTimeoutException(Exception):
    pass


class TestPlanStatus(Enum):
    INIT = 10
    LOCK_TESTBED = 20
    PREPARE_TESTBED = 30
    EXECUTING = 40
    KVMDUMP = 50
    FAILED = 60
    CANCELLED = 70
    FINISHED = 80


def get_test_scripts(test_set):
    _self_path = os.path.abspath(__file__)
    pr_test_scripts_file = os.path.join(os.path.dirname(_self_path), PR_TEST_SCRIPTS_FILE)
    with open(pr_test_scripts_file) as f:
        pr_test_scripts = yaml.safe_load(f)

        test_script_list = pr_test_scripts.get(test_set, [])
        specific_param_list = pr_test_scripts.get(SPECIFIC_PARAM_KEYWORD, {}).get(test_set, [])
        return test_script_list, specific_param_list


def test_plan_status_factory(status):
    if status == "INIT":
        return InitStatus()
    elif status == "LOCK_TESTBED":
        return LockStatus()
    elif status == "PREPARE_TESTBED":
        return PrePareStatus()
    elif status == "EXECUTING":
        return ExecutingStatus()
    elif status == "KVMDUMP":
        return KvmDumpStatus()
    elif status == "FAILED":
        return FailedStatus()
    elif status == "CANCELLED":
        return CancelledStatus()
    elif status == "FINISHED":
        return FinishStatus()

    raise Exception("The status is not correct.")


class AbstractStatus:
    def __init__(self, status):
        self.status = status

    def get_status(self):
        return self.status.value

    def print_logs(self, test_plan_id, resp_data, expected_status, start_time):
        status = resp_data.get("status", None)
        current_status = test_plan_status_factory(status).get_status()

        if current_status == self.get_status():
            print(
                f"Test plan id: {test_plan_id}, status: {resp_data.get('status', None)}, "
                f"expected_status: {expected_status}, elapsed: {time.time() - start_time:.0f} seconds"
            )


class InitStatus(AbstractStatus):
    def __init__(self):
        super(InitStatus, self).__init__(TestPlanStatus.INIT)


class LockStatus(AbstractStatus):
    def __init__(self):
        super(LockStatus, self).__init__(TestPlanStatus.LOCK_TESTBED)


class PrePareStatus(AbstractStatus):
    def __init__(self):
        super(PrePareStatus, self).__init__(TestPlanStatus.PREPARE_TESTBED)


class ExecutingStatus(AbstractStatus):
    def __init__(self):
        super(ExecutingStatus, self).__init__(TestPlanStatus.EXECUTING)

    def print_logs(self, test_plan_id, resp_data, expected_status, start_time):
        print(
            f"Test plan id: {test_plan_id}, status: {resp_data.get('status', None)}, "
            f"expected_status: {expected_status}, progress: {resp_data.get('progress', 0) * 100:.2f}%, "
            f"elapsed: {time.time() - start_time:.0f} seconds"
        )


class KvmDumpStatus(AbstractStatus):
    def __init__(self):
        super(KvmDumpStatus, self).__init__(TestPlanStatus.KVMDUMP)


class FailedStatus(AbstractStatus):
    def __init__(self):
        super(FailedStatus, self).__init__(TestPlanStatus.FAILED)


class CancelledStatus(AbstractStatus):
    def __init__(self):
        super(CancelledStatus, self).__init__(TestPlanStatus.CANCELLED)


class FinishStatus(AbstractStatus):
    def __init__(self):
        super(FinishStatus, self).__init__(TestPlanStatus.FINISHED)


def parse_list_from_str(s):
    # Since Azure Pipeline doesn't support to receive an empty parameter,
    # We use ' ' as a magic code for empty parameter.
    # So we should consider ' ' as en empty input.
    if isinstance(s, str):
        s = s.strip()
    if not s:
        return None
    return [single_str.strip()
            for single_str in s.split(',')
            if single_str.strip()]


def run_cmd(cmd):
    process = subprocess.Popen(
        cmd.split(),
        shell=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    stdout, stderr = process.communicate()
    return_code = process.returncode

    if return_code != 0:
        raise Exception(f'Command {cmd} execution failed, rc={return_code}, error={stderr}')
    return stdout, stderr, return_code


def get_trigger_type(tp_type: str, build_reason: str):
    if build_reason.upper() == "PULLREQUEST":
        return "PRTest"

    # Else, return None, no impact
    return None


class TestPlanManager(object):

    def __init__(self, scheduler_url, frontend_url, client_id):
        self.scheduler_url = scheduler_url
        self.frontend_url = frontend_url
        self.client_id = client_id

    def get_token(self):
        # Get access token with re-try
        get_token_cmd = f"az account get-access-token --resource {self.client_id}"
        get_token_attempts = 0
        while get_token_attempts < MAX_GET_TOKEN_RETRY_TIMES:
            try:
                stdout, _, _ = run_cmd(get_token_cmd)

                token = json.loads(stdout.decode("utf-8"))
                access_token = token.get("accessToken", None)
                if not access_token:
                    raise Exception("Parse token from stdout failed, accessToken is None.")

                # Parse token expires time from string
                token_expires_on = token.get("expiresOn", "")
                if token_expires_on:
                    print(f"Get token successfully. Token will expire on {token_expires_on}.")

                return access_token

            except Exception as exception:
                get_token_attempts += 1
                print(f"Failed to get token with exception: {repr(exception)}.")

        # If az get token failed, return with exception
        if get_token_attempts >= MAX_GET_TOKEN_RETRY_TIMES:
            raise Exception(f"Failed to get token after {MAX_GET_TOKEN_RETRY_TIMES} attempts")

    def create(self, topology, test_plan_name="my_test_plan", deploy_mg_extra_params="", kvm_build_id="",
               min_worker=None, max_worker=None, pr_id="unknown", output=None,
               common_extra_params="", **kwargs):
        tp_url = f"{self.scheduler_url}/test_plan"
        testbed_name = parse_list_from_str(kwargs.get("testbed_name", None))
        image_url = kwargs.get("image_url", None)
        hwsku = kwargs.get("hwsku", None)
        test_plan_type = kwargs.get("test_plan_type", "PR")
        platform = kwargs.get("platform", "kvm")
        scripts = parse_list_from_str(kwargs.get("scripts", None))
        features = parse_list_from_str(kwargs.get("features", None))
        scripts_exclude = parse_list_from_str(kwargs.get("scripts_exclude", None))
        features_exclude = parse_list_from_str(kwargs.get("features_exclude", None))
        ptf_image_tag = kwargs.get("ptf_image_tag", None)
        build_reason = kwargs.get("build_reason", "PullRequest")
        lock_wait_timeout_seconds = kwargs.get("lock_wait_timeout_seconds", 0)
        # If not set lock tb timeout, set to 2 hours for pr test plans by default
        if lock_wait_timeout_seconds == 0 and test_plan_type == "PR":
            lock_wait_timeout_seconds = int(os.environ.get("TIMEOUT_IN_SECONDS_PR_TEST_PLAN_LOCK_TB", 7200))
        # if not set test plan timeout, set to 6 hours for pr test plans by default
        max_execute_seconds = kwargs.get("max_execute_seconds", 0)
        if max_execute_seconds == 0 and test_plan_type == "PR":
            max_execute_seconds = int(os.environ.get("TIMEOUT_IN_SECONDS_PR_TEST_PLAN", 21600))

        print(
            f"Creating test plan, topology: {topology}, name: {test_plan_name}, "
            f"build info:{repo_name} {pr_id} {build_id}"
        )
        print("Test scripts to be covered in this test plan:")
        print(json.dumps(scripts, indent=4))

        common_extra_params = common_extra_params + " --allow_recover"

        # Add topo and device type args for PR test
        if test_plan_type == "PR":
            # Add topo arg
            if topology in ["t0", "t0-64-32"]:
                common_extra_params = common_extra_params + " --topology=t0,any"
            elif topology in ["t1-lag", "t1-8-lag"]:
                common_extra_params = common_extra_params + " --topology=t1,any"
            elif topology == "dualtor":
                common_extra_params = common_extra_params + " --topology=t0,dualtor,any"
            elif topology == "dpu":
                common_extra_params = common_extra_params + " --topology=dpu,any"

            # Add device type arg
            common_extra_params = common_extra_params + " --device_type=vs"

        # If triggered by the internal repos, use internal sonic-mgmt repo as the code base
        sonic_mgmt_repo_url = GITHUB_SONIC_MGMT_REPO
        if kwargs.get("source_repo") in INTERNAL_REPO_LIST:
            sonic_mgmt_repo_url = INTERNAL_SONIC_MGMT_REPO

        # If triggered by mgmt repo, use pull request id as the code base
        sonic_mgmt_pull_request_id = ""
        if MGMT_REPO_FLAG in kwargs.get("source_repo"):
            sonic_mgmt_pull_request_id = pr_id

        # If triggered by buildimage repo, use image built from the buildId
        kvm_image_build_id = kvm_build_id
        kvm_image_branch = kwargs.get("kvm_image_branch", "")
        if BUILDIMAGE_REPO_FLAG in kwargs.get("source_repo"):
            kvm_image_build_id = build_id
            kvm_image_branch = ""
        affinity = json.loads(kwargs.get("affinity", "[]"))
        payload = {
            "name": test_plan_name,
            "testbed": {
                "platform": platform,
                "name": testbed_name,
                "topology": topology,
                "hwsku": hwsku,
                "min": min_worker,
                "max": max_worker,
                "nbr_type": kwargs["vm_type"],
                "asic_num": kwargs["num_asic"],
                "lock_wait_timeout_seconds": lock_wait_timeout_seconds,
            },
            "test_option": {
                "stop_on_failure": kwargs.get("stop_on_failure", True),
                "retry_times": kwargs.get("retry_times", 2),
                "test_cases": {
                    "features": features,
                    "scripts": scripts,
                    "features_exclude": features_exclude,
                    "scripts_exclude": scripts_exclude
                },
                "ptf_image_tag": ptf_image_tag,
                "image": {
                    "url": image_url,
                    "upgrade_image_param": kwargs.get("upgrade_image_param", None),
                    "release": "",
                    "kvm_image_build_id": kvm_image_build_id,
                    "kvm_image_branch": kvm_image_branch
                },
                "sonic_mgmt": {
                    "repo_url": sonic_mgmt_repo_url,
                    "branch": kwargs["mgmt_branch"],
                    "pull_request_id": sonic_mgmt_pull_request_id
                },
                "common_param": common_extra_params,
                "specific_param": kwargs.get("specific_param", []),
                "affinity": affinity,
                "deploy_mg_param": deploy_mg_extra_params,
                "max_execute_seconds": max_execute_seconds,
            },
            "type": test_plan_type,
            "trigger": {
                "requester": kwargs.get("requester", "Pull Request"),
                "source_repo": kwargs.get("source_repo"),
                "pull_request_id": pr_id,
                "build_id": build_id,
                "type": get_trigger_type(test_plan_type, build_reason)
            },
            "extra_params": {},
            "priority": 10
        }
        print(f"Creating test plan with payload:\n{json.dumps(payload, indent=4)}")
        headers = {
            "Authorization": f"Bearer {self.get_token()}",
            "Content-Type": "application/json"
        }
        raw_resp = {}
        try:
            raw_resp = requests.post(tp_url, headers=headers, data=json.dumps(payload), timeout=20)
            resp = raw_resp.json()
        except Exception as exception:
            raise Exception(f"HTTP execute failure, url: {tp_url}, raw_resp: {raw_resp}, exception: {str(exception)}")
        if not resp["data"]:
            raise Exception(f"Create test plan failed with error: {resp['errmsg']}")
        if not resp["success"]:
            raise Exception(f"Create test plan failed with error: {resp['errmsg']}")

        print(f"Result of creating test plan: {str(resp['data'])}")

        if output:
            print(f"Store new test plan id to file {output}")
            with open(output, "a") as f:
                f.write(str(resp["data"]) + "\n")

        return resp["data"]

    def cancel(self, test_plan_id):

        tp_url = f"{self.scheduler_url}/test_plan/{test_plan_id}"
        cancel_url = f"{tp_url}/cancel"

        print(f"Cancelling test plan at {cancel_url}")

        payload = json.dumps({})
        headers = {
            "Authorization": f"Bearer {self.get_token()}",
            "Content-Type": "application/json"
        }

        raw_resp = {}
        try:
            raw_resp = requests.post(cancel_url, headers=headers, data=payload, timeout=10)
            resp = raw_resp.json()
        except Exception as exception:
            raise Exception(f"HTTP execute failure, url: {cancel_url}, raw_resp: {str(raw_resp)}, "
                            f"exception: {str(exception)}")
        if not resp["success"]:
            raise Exception(f"Cancel test plan failed with error: {resp['errmsg']}")

        print(f"Result of cancelling test plan at {tp_url}:")
        print(str(resp["data"]))

    def poll(self, test_plan_id, interval=60, timeout=-1, expected_state="", expected_result=None):
        print(f"Polling progress and status of test plan at {self.frontend_url}/scheduler/testplan/{test_plan_id}")
        print(f"Polling interval: {interval} seconds")

        poll_url = f"{self.scheduler_url}/test_plan/{test_plan_id}/get_test_plan_status"
        # In current polling task, initialize headers one time to avoid frequent token accessing
        # For some tasks running over 24h, then token may expire and pipeline fail, but pr test should not run over 24h
        headers = {
            "Authorization": f"Bearer {self.get_token()}",
            "Content-Type": "application/json"
        }
        start_time = time.time()
        poll_retry_times = 0
        while timeout < 0 or (time.time() - start_time) < timeout:
            resp = None
            try:
                resp = requests.get(poll_url, headers=headers, timeout=10).json()

                if not resp:
                    raise Exception("Poll test plan status failed with request error, no response!")

                if not resp["success"]:
                    raise Exception(f"Get test plan status failed with error: {resp['errmsg']}")

                resp_data = resp.get("data", None)
                if not resp_data:
                    raise Exception("No valid data in response.")

            except Exception as exception:
                print(f"Failed to get valid response, url: {poll_url}, raw_resp: {resp}, exception: {str(exception)}")

                poll_retry_times = poll_retry_times + 1
                if poll_retry_times >= MAX_POLL_RETRY_TIMES:
                    raise Exception("Poll test plan status failed, exceeded the maximum number of retries.")
                else:
                    time.sleep(interval)
                continue

            current_tp_status = resp_data.get("status", None)
            current_tp_result = resp_data.get("result", None)

            if expected_state:
                current_status = test_plan_status_factory(current_tp_status)
                expected_status = test_plan_status_factory(expected_state)

                current_status.print_logs(test_plan_id, resp_data, expected_state, start_time)

                # If test plan has finished current step, its now status will behind the expected status
                if expected_status.get_status() < current_status.get_status():
                    steps = None
                    step_status = None
                    runtime = resp_data.get("runtime", None)
                    if runtime:
                        steps = runtime.get("steps", None)
                    if steps:
                        for step in steps:
                            if step.get("step") == expected_state:
                                step_status = step.get("status")
                                break

                    # Print test summary
                    test_summary = resp_data.get("runtime", {}).get("test_summary", None)
                    if test_summary:
                        print(f"Test summary:\n{json.dumps(test_summary, indent=4)}")

                    """
                    In below scenarios, need to return false to pipeline.
                    1. If step status is {FAILED}, exactly need to return false to pipeline.
                    2. If current test plan status finished but unsuccessful, need to check if current step status
                       executed successfully, if not, return false to pipeline.
                    """
                    current_step_unsuccessful = (step_status == "FAILED"
                                                 or (current_tp_status in TEST_PLAN_STATUS_UNSUCCESSFUL_FINISHED
                                                     and step_status in TEST_PLAN_STEP_STATUS_UNFINISHED))

                    if current_step_unsuccessful:

                        # Print error type and message
                        err_code = resp_data.get("runtime", {}).get("err_code", None)
                        if err_code:
                            print(f"Error type: {err_code}")

                        err_msg = resp_data.get("runtime", {}).get("message", None)
                        if err_msg:
                            print(f"Error message: {err_msg}")

                        raise Exception(
                            f"Test plan id: {test_plan_id}, status: {step_status}, "
                            f"result: {current_tp_result}, Elapsed {time.time() - start_time:.0f} seconds. "
                            f"Check {self.frontend_url}/scheduler/testplan/{test_plan_id} for test plan status"
                        )
                    if expected_result:
                        if current_tp_result != expected_result:
                            raise Exception(
                                f"Test plan id: {test_plan_id}, status: {step_status}, "
                                f"result: {current_tp_result} not match expected result: {expected_result}, "
                                f"Elapsed {time.time() - start_time:.0f} seconds. "
                                f"Check {self.frontend_url}/scheduler/testplan/{test_plan_id} for test plan status"
                            )

                    print(f"Current step status is {step_status}.")
                    return

                time.sleep(interval)

        else:
            raise PollTimeoutException(
                f"Max polling time reached, test plan at {poll_url} is not successfully finished or cancelled"
            )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="Tool for managing test plan.")

    subparsers = parser.add_subparsers(
        title="action",
        help="Action to perform on test plan",
        dest="action"
    )

    parser_create = subparsers.add_parser("create", help="Create new test plan.")
    parser_create.add_argument(
        "-t", "--topology",
        type=str,
        dest="topology",
        nargs="?",
        const="",
        default="",
        required=False,
        help="The test topology to be used."
    )
    parser_create.add_argument(
        "-o", "--output",
        type=str,
        dest="output",
        required=False,
        help="Output id of created test plan to the specified file."
    )
    parser_create.add_argument(
        "--min-worker",
        type=int,
        dest="min_worker",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Min worker number for the test plan."
    )
    parser_create.add_argument(
        "--max-worker",
        type=int,
        dest="max_worker",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Max worker number for the test plan."
    )
    parser_create.add_argument(
        "--lock-wait-timeout-seconds",
        type=int,
        dest="lock_wait_timeout_seconds",
        nargs='?',
        const=0,
        default=0,
        required=False,
        help="Max lock testbed wait seconds. None or the values <= 0 means endless."
    )
    parser_create.add_argument(
        "--test-set",
        type=str,
        dest="test_set",
        nargs='?',
        const='',
        default="",
        required=False,
        help="Test set."
    )
    parser_create.add_argument(
        "--deploy-mg-extra-params",
        type=str,
        nargs='?',
        const='',
        dest="deploy_mg_extra_params",
        default="",
        required=False,
        help="Deploy minigraph extra params"
    )
    parser_create.add_argument(
        "--kvm-image-branch",
        type=str,
        dest="kvm_image_branch",
        nargs='?',
        const="",
        default="",
        required=False,
        help="KVM build branch."
    )
    parser_create.add_argument(
        "--kvm-build-id",
        type=str,
        nargs='?',
        const='',
        dest="kvm_build_id",
        default="",
        required=False,
        help="KVM build id."
    )
    parser_create.add_argument(
        "--mgmt-branch",
        type=str,
        dest="mgmt_branch",
        nargs='?',
        const="master",
        default="master",
        required=False,
        help="Branch of sonic-mgmt repo to run the test"
    )
    parser_create.add_argument(
        "--vm-type",
        type=str,
        dest="vm_type",
        default="ceos",
        required=False,
        help="VM type of neighbors"
    )
    parser_create.add_argument(
        "--specified-params",
        type=str,
        dest="specified_params",
        default="{}",
        required=False,
        help="Test module specified params"
    )
    parser_create.add_argument(
        "--common-extra-params",
        type=str,
        dest="common_extra_params",
        nargs='?',
        const="",
        default="",
        required=False,
        help="Run test common extra params"
    )
    parser_create.add_argument(
        "--num-asic",
        type=int,
        dest="num_asic",
        default=1,
        required=False,
        help="The asic number of dut"
    )
    parser_create.add_argument(
        "--build-reason",
        type=str,
        dest="build_reason",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Build reason"
    )
    parser_create.add_argument(
        "--repo-name",
        type=str,
        dest="repo_name",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Repository name"
    )
    parser_create.add_argument(
        "--testbed-name",
        type=str,
        dest="testbed_name",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Testbed name, Split by ',', like: 'testbed1, testbed2'"
    )
    parser_create.add_argument(
        "--ptf_image_tag",
        type=str,
        dest="ptf_image_tag",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="PTF image tag"
    )
    parser_create.add_argument(
        "--image_url",
        type=str,
        dest="image_url",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Image url"
    )
    parser_create.add_argument(
        "--upgrade-image-param",
        type=str,
        dest="upgrade_image_param",
        nargs="?",
        const="",
        default="",
        required=False,
        help="Parameter of upgrade image"
    )
    parser_create.add_argument(
        "--hwsku",
        type=str,
        dest="hwsku",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Hardware SKU."
    )
    parser_create.add_argument(
        "--test-plan-type",
        type=str,
        dest="test_plan_type",
        nargs='?',
        const='PR',
        default="PR",
        required=False,
        choices=['PR', 'NIGHTLY'],
        help="Test plan type. Optional: ['PR', 'NIGHTLY']"
    )
    parser_create.add_argument(
        "--platform",
        type=str,
        dest="platform",
        nargs='?',
        const='kvm',
        default="kvm",
        required=False,
        help="Testbed platform."
    )
    parser_create.add_argument(
        "--features",
        type=str,
        dest="features",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Test features, Split by ',', like: 'bgp, lldp'"
    )
    parser_create.add_argument(
        "--scripts",
        type=str,
        dest="scripts",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Test scripts, Split by ',', like: 'bgp/test_bgp_fact.py, test_feature.py'"
    )
    parser_create.add_argument(
        "--scripts-exclude",
        type=str,
        dest="scripts_exclude",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Exclude test scripts, Split by ',', like: 'bgp/test_bgp_fact.py, test_feature.py'"
    )
    parser_create.add_argument(
        "--features-exclude",
        type=str,
        dest="features_exclude",
        nargs='?',
        const=None,
        default=None,
        required=False,
        help="Exclude test features, Split by ',', like: 'bgp, lldp'"
    )
    parser_create.add_argument(
        "--specific-param",
        type=str,
        dest="specific_param",
        nargs='?',
        const="[]",
        default="[]",
        required=False,
        help='Specific param, like: '
             '[{"name": "macsec", "param": "--enable_macsec --macsec_profile=128_SCI,256_XPN_SCI"}]'
    )
    parser_create.add_argument(
        "--affinity",
        type=str,
        dest="affinity",
        nargs='?',
        const="[]",
        default="[]",
        required=False,
        help='Test module affinity, like: '
             '[{"name": "bgp/test_bgp_fact.py", "op": "NOT_ON", "value": ["vms-kvm-t0"]}]'
    )
    parser_create.add_argument(
        "--stop-on-failure",
        type=ast.literal_eval,
        dest="stop_on_failure",
        nargs='?',
        const='True',
        default='True',
        required=False,
        choices=[True, False],
        help="Stop whole test plan if test failed."
    )
    parser_create.add_argument(
        "--retry-times",
        type=int,
        dest="retry_times",
        nargs='?',
        const=2,
        default=2,
        required=False,
        help="Retry times after tests failed."
    )
    parser_create.add_argument(
        "--requester",
        type=str,
        dest="requester",
        nargs='?',
        const='Pull Request',
        default="Pull Request",
        required=False,
        help="Requester of the test plan."
    )
    parser_create.add_argument(
        "--max-execute-seconds",
        type=int,
        dest="max_execute_seconds",
        nargs='?',
        const=0,
        default=0,
        required=False,
        help="Max execute seconds of the test plan."
    )
    parser_create.add_argument(
        "--test-plan-num",
        type=int,
        dest="test_plan_num",
        nargs="?",
        const=1,
        default=1,
        required=False,
        help="Test plan num to be created."
    )

    parser_poll = subparsers.add_parser("poll", help="Poll test plan status.")
    parser_cancel = subparsers.add_parser("cancel", help="Cancel running test plan.")

    for p in [parser_cancel, parser_poll]:
        p.add_argument(
            "-i", "--test-plan-id",
            type=str,
            dest="test_plan_id",
            required=True,
            help="Test plan id."
        )

    parser_poll.add_argument(
        "--expected-state",
        type=str,
        dest="expected_state",
        required=False,
        help="Expected state.",
        default=""
    )
    parser_poll.add_argument(
        "--expected-result",
        type=str,
        dest="expected_result",
        nargs='?',
        const=None,
        default=None,
        required=False,
        choices=['PENDING', 'EXECUTING', 'SUCCESS', 'FAILED', 'CANCELLED'],
        help="If specify expected result, check test plan result after expected state matched."
    )
    parser_poll.add_argument(
        "--interval",
        type=int,
        required=False,
        default=60,
        dest="interval",
        help="Polling interval. Default 60 seconds."
    )
    parser_poll.add_argument(
        "--timeout",
        type=int,
        required=False,
        default=-1,
        dest="timeout",
        help="Max polling time in seconds. Default -1, no timeout."
    )

    if len(sys.argv) == 1:
        parser.print_help(sys.stderr)
        sys.exit(1)

    args = parser.parse_args()

    if "test_plan_id" in args:
        # vso may add unexpected "'" as trailing symbol
        # https://github.com/microsoft/azure-pipelines-tasks/issues/10331
        args.test_plan_id = args.test_plan_id.replace("'", "")

    print(f"Test plan utils parameters: {args}")

    required_env = ["ELASTICTEST_SCHEDULER_BACKEND_URL", "CLIENT_ID"]

    env = {
        "ELASTICTEST_SCHEDULER_BACKEND_URL": os.environ.get("ELASTICTEST_SCHEDULER_BACKEND_URL"),
        "CLIENT_ID": os.environ.get("ELASTICTEST_MSAL_CLIENT_ID"),
        "FRONTEND_URL": os.environ.get("ELASTICTEST_FRONTEND_URL", "https://elastictest.org"),
    }
    env_missing = [k.upper() for k, v in env.items() if k.upper() in required_env and not v]
    if env_missing:
        print(f"Missing required environment variables: {env_missing}.")
        sys.exit(1)

    try:
        tp = TestPlanManager(
            env["ELASTICTEST_SCHEDULER_BACKEND_URL"],
            env["FRONTEND_URL"],
            env["CLIENT_ID"],
        )

        if args.action == "create":
            pr_id = os.environ.get("SYSTEM_PULLREQUEST_PULLREQUESTNUMBER") or os.environ.get(
                "SYSTEM_PULLREQUEST_PULLREQUESTID")
            build_repo_provider = os.environ.get("BUILD_REPOSITORY_PROVIDER")
            build_reason = args.build_reason if args.build_reason else os.environ.get("BUILD_REASON")
            build_id = os.environ.get("BUILD_BUILDID")
            job_name = os.environ.get("SYSTEM_JOBDISPLAYNAME")
            repo_name = args.repo_name if args.repo_name else os.environ.get("BUILD_REPOSITORY_NAME")
            branch_name = os.environ.get("SYSTEM_PULLREQUEST_TARGETBRANCH") if build_reason.upper() == "PULLREQUEST" \
                else os.environ.get("BUILD_SOURCEBRANCHNAME")

            # Only pr test show pr id
            pr_info = f"PR_{pr_id}_" if build_reason.upper() == "PULLREQUEST" else ""

            # Only pr test and baseline test show repo and branch
            source_repo_info = f"{repo_name}_{branch_name}_" if args.test_plan_type == "PR" else ""

            test_plan_prefix = (f"{build_repo_provider}_{build_reason}_{source_repo_info}{pr_info}"
                                f"BUILD_{build_id}_JOB_{job_name}").replace(' ', '_')

            scripts = args.scripts
            specific_param = json.loads(args.specific_param)
            # For PR test, if specify test modules and specific_param explicitly, use them to run PR test.
            # Otherwise, get test modules from pr_test_scripts.yaml.
            explicitly_specify_test_module = args.features or args.scripts
            if args.test_plan_type == "PR":
                args.test_set = args.test_set if args.test_set else args.topology
                parsed_script, parsed_specific_param = get_test_scripts(args.test_set)
                if not explicitly_specify_test_module:
                    scripts = ",".join(parsed_script)
                if not specific_param:
                    specific_param = parsed_specific_param

            for num in range(args.test_plan_num):
                test_plan_name = copy.copy(test_plan_prefix)
                if args.test_plan_num > 1:
                    test_plan_name = f"{test_plan_name}_{num + 1}"

                tp.create(
                    args.topology,
                    test_plan_name=test_plan_name,
                    deploy_mg_extra_params=args.deploy_mg_extra_params,
                    kvm_build_id=args.kvm_build_id,
                    kvm_image_branch=args.kvm_image_branch,
                    min_worker=args.min_worker,
                    max_worker=args.max_worker,
                    pr_id=pr_id,
                    scripts=scripts,
                    features=args.features,
                    scripts_exclude=args.scripts_exclude,
                    features_exclude=args.features_exclude,
                    output=args.output,
                    source_repo=repo_name,
                    mgmt_branch=args.mgmt_branch,
                    common_extra_params=args.common_extra_params,
                    num_asic=args.num_asic,
                    specified_params=args.specified_params,
                    specific_param=specific_param,
                    affinity=args.affinity,
                    vm_type=args.vm_type,
                    testbed_name=args.testbed_name,
                    ptf_image_tag=args.ptf_image_tag,
                    image_url=args.image_url,
                    upgrade_image_param=args.upgrade_image_param,
                    hwsku=args.hwsku,
                    test_plan_type=args.test_plan_type,
                    platform=args.platform,
                    stop_on_failure=args.stop_on_failure,
                    retry_times=args.retry_times,
                    requester=args.requester,
                    max_execute_seconds=args.max_execute_seconds,
                    lock_wait_timeout_seconds=args.lock_wait_timeout_seconds,
                    build_reason=build_reason
                )
        elif args.action == "poll":
            tp.poll(args.test_plan_id, args.interval, args.timeout, args.expected_state, args.expected_result)
        elif args.action == "cancel":
            tp.cancel(args.test_plan_id)
        sys.exit(0)
    except PollTimeoutException as e:
        print(f"Polling test plan failed with exception: {repr(e)}")
        sys.exit(2)
    except Exception as e:
        print(f"Operation failed with exception: {repr(e)}")
        sys.exit(3)
