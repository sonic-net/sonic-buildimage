#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "mock_helper.h"
#include <libtac/support.h>

#define IS_LOCAL_USER              0
#define IS_REMOTE_USER             1
#define ERROR_CHECK_LOCAL_USER     2

/* tacacs debug flag */
extern int tacacs_ctrl;

int clean_up() {
  return 0;
}

int start_up() {
  initialize_tacacs_servers();
  tacacs_ctrl = PAM_TAC_DEBUG;
  return 0;
}

/* Test tacacs_authorization all tacacs server connect failed case */
void testcase_tacacs_authorization_all_failed() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";


	// test connection failed case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_ALL_FAILED);
	int result = tacacs_authorization("test_user","tty0","test_host","test_command",testargv,2);

	CU_ASSERT_STRING_EQUAL(mock_syslog_message_buffer, "Failed to connect to TACACS server(s)\n");

	// check return value, -2 for all server not reachable
	CU_ASSERT_EQUAL(result, -2);
}

/* Test tacacs_authorization get failed result case */
void testcase_tacacs_authorization_faled() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	// test connection failed case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_FAILED_RESULT);
	int result = tacacs_authorization("test_user","tty0","test_host","test_command",testargv,2);

    // send auth message failed.
	CU_ASSERT_EQUAL(result, -1);
}

/* Test tacacs_authorization read failed case */
void testcase_tacacs_authorization_read_failed() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	// test connection failed case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_READ_FAILED);
	int result = tacacs_authorization("test_user","tty0","test_host","test_command",testargv,2);

	CU_ASSERT_STRING_EQUAL(mock_syslog_message_buffer, "test_command not authorized from TestAddress2\n");

    // read auth message failed.
	CU_ASSERT_EQUAL(result, -1);
}

/* Test tacacs_authorization get denined case */
void testcase_tacacs_authorization_denined() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	// test connection denined case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_DENINED_RESULT);
	int result = tacacs_authorization("test_user","tty0","test_host","test_command",testargv,2);

	CU_ASSERT_STRING_EQUAL(mock_syslog_message_buffer, "test_command not authorized from TestAddress2\n");

    // send auth message denined.
	CU_ASSERT_EQUAL(result, 1);
}

/* Test tacacs_authorization get success case */
void testcase_tacacs_authorization_success() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	// test connection success case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT);
	int result = tacacs_authorization("test_user","tty0","test_host","test_command",testargv,2);

	// wuthorization success
	CU_ASSERT_EQUAL(result, 0);
}

/* Test send_authorization_message adds TraceId attribute when present */
void testcase_send_authorization_message_trace_id() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	reset_mock_tac_attrs();
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT);
	setenv("TraceId", "trace-123:abc.def", 1);

	int result = send_authorization_message(0, "test_user", "tty0", "test_host", 42, "test_command", testargv, 2);

	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(mock_tac_trace_id_attr_count, 1);
	CU_ASSERT_STRING_EQUAL(mock_tac_trace_id_attr_value, "trace-123:abc.def");

	unsetenv("TraceId");
}

/* Test send_authorization_message skips TraceId attribute when not present */
void testcase_send_authorization_message_without_trace_id() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	reset_mock_tac_attrs();
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT);
	unsetenv("TraceId");

	int result = send_authorization_message(0, "test_user", "tty0", "test_host", 42, "test_command", testargv, 2);

	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(mock_tac_trace_id_attr_count, 0);
}

/* Test send_authorization_message skips empty TraceId values */
void testcase_send_authorization_message_empty_trace_id() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	reset_mock_tac_attrs();
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT);
	setenv("TraceId", "", 1);

	int result = send_authorization_message(0, "test_user", "tty0", "test_host", 42, "test_command", testargv, 2);

	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(mock_tac_trace_id_attr_count, 0);

	unsetenv("TraceId");
}

/* Test send_authorization_message skips unsafe TraceId values */
void testcase_send_authorization_message_invalid_trace_id() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	reset_mock_tac_attrs();
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT);
	setenv("TraceId", "trace\n123", 1);

	int result = send_authorization_message(0, "test_user", "tty0", "test_host", 42, "test_command", testargv, 2);

	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(mock_tac_trace_id_attr_count, 0);

	unsetenv("TraceId");
}

/* Test send_authorization_message skips oversized TraceId values */
void testcase_send_authorization_message_long_trace_id() {
	char *testargv[2];
	char trace_id[249];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	memset(trace_id, 'a', sizeof(trace_id) - 1);
	trace_id[sizeof(trace_id) - 1] = '\0';

	reset_mock_tac_attrs();
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT);
	setenv("TraceId", trace_id, 1);

	int result = send_authorization_message(0, "test_user", "tty0", "test_host", 42, "test_command", testargv, 2);

	CU_ASSERT_EQUAL(result, 0);
	CU_ASSERT_EQUAL(mock_tac_trace_id_attr_count, 0);

	unsetenv("TraceId");
}

/* Test authorization_with_host_and_tty get success case */
void testcase_authorization_with_host_and_tty_success() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";

	// test connection success case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT);
	int result = authorization_with_host_and_tty("test_user","test_command",testargv,2);

	// wuthorization success
	CU_ASSERT_EQUAL(result, 0);
}

/* Test check_and_load_changed_tacacs_config */
void testcase_check_and_load_changed_tacacs_config() {

	set_test_scenario(TEST_SCEANRIO_LOAD_CHANGED_TACACS_CONFIG);

	// test connection failed case
	check_and_load_changed_tacacs_config();

    // check server config updated.
	CU_ASSERT_STRING_EQUAL(mock_syslog_message_buffer, "Server 2, address:TestAddress2, key:key2\n");

	// check and load file again.
	check_and_load_changed_tacacs_config();

    // check server config not update.
	char* configNotChangeLog = "tacacs config file not change: last modified time";
	CU_ASSERT_TRUE(strncmp(mock_syslog_message_buffer, configNotChangeLog, strlen(configNotChangeLog)) == 0);
}

/* Test on_shell_execve authorization successed */
void testcase_on_shell_execve_success() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";
	testargv[2] = 0;

	// test connection failed case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT);
	on_shell_execve("test_user", 1, "test_command", testargv);

    // check authorized success.
	CU_ASSERT_STRING_EQUAL(mock_syslog_message_buffer, "test_command authorize successed by TACACS+ with given arguments\n");
}

/* Test on_shell_execve authorization denined */
void testcase_on_shell_execve_denined() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";
	testargv[2] = 0;

	// test connection failed case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_SEND_DENINED_RESULT);
	on_shell_execve("test_user", 1, "test_command", testargv);

    // check authorized failed.
	CU_ASSERT_STRING_EQUAL(mock_syslog_message_buffer, "test_command authorize failed by TACACS+ with given arguments, not executing\n");
}

/* Test on_shell_execve authorization failed */
void testcase_on_shell_execve_failed() {
	char *testargv[2];
	testargv[0] = "arg1";
	testargv[1] = "arg2";
	testargv[2] = 0;

	// test connection failed case
	set_test_scenario(TEST_SCEANRIO_CONNECTION_ALL_FAILED);
	on_shell_execve("test_user", 1, "test_command", testargv);

    // check not authorized.
	CU_ASSERT_STRING_EQUAL(mock_syslog_message_buffer, "test_command not authorized by TACACS+ with given arguments, not executing\n");
}

/* Test is_local_user unknown user */
void testcase_is_local_user_unknown() {
	set_test_scenario(TEST_SCEANRIO_IS_LOCAL_USER_UNKNOWN);
	int result = is_local_user("UNKNOWN");

    // check unknown user is remote.
	CU_ASSERT_EQUAL(result, IS_REMOTE_USER);
}

/* Test is_local_user not found user */
void testcase_is_local_user_not_found() {
	set_test_scenario(TEST_SCEANRIO_IS_LOCAL_USER_NOT_FOUND);
	int result = is_local_user("notexist");

    // check unknown user is remote.
	CU_ASSERT_EQUAL(result, ERROR_CHECK_LOCAL_USER);
	CU_ASSERT_STRING_EQUAL(mock_syslog_message_buffer, "get user information user failed, user: notexist not found\n");
}

/* Test is_local_user root user */
void testcase_is_local_user_root() {
	set_test_scenario(TEST_SCEANRIO_IS_LOCAL_USER_ROOT);
	int result = is_local_user("root");

    // check unknown user is remote.
	CU_ASSERT_EQUAL(result, IS_LOCAL_USER);
}

/* Test is_local_user remote user */
void testcase_is_local_user_remote() {
	set_test_scenario(TEST_SCEANRIO_IS_LOCAL_USER_REMOTE);
	int result = is_local_user("test_user");

    // check unknown user is remote.
	CU_ASSERT_EQUAL(result, IS_REMOTE_USER);
}

int main(void) {
  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  CU_pSuite ste = CU_add_suite("plugin_test", start_up, clean_up);
  if (NULL == ste) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (CU_get_error() != CUE_SUCCESS) {
    fprintf(stderr, "Error creating suite: (%d)%s\n", CU_get_error(), CU_get_error_msg());
    return CU_get_error();
  }

  if (!CU_add_test(ste, "Test testcase_tacacs_authorization_all_failed()...\n", testcase_tacacs_authorization_all_failed)
	  || !CU_add_test(ste, "Test testcase_tacacs_authorization_faled()...\n", testcase_tacacs_authorization_faled)
	  || !CU_add_test(ste, "Test testcase_tacacs_authorization_read_failed()...\n", testcase_tacacs_authorization_read_failed)
	  || !CU_add_test(ste, "Test testcase_tacacs_authorization_denined()...\n", testcase_tacacs_authorization_denined)
	  || !CU_add_test(ste, "Test testcase_tacacs_authorization_success()...\n", testcase_tacacs_authorization_success)
	  || !CU_add_test(ste, "Test testcase_send_authorization_message_trace_id()...\n", testcase_send_authorization_message_trace_id)
	  || !CU_add_test(ste, "Test testcase_send_authorization_message_without_trace_id()...\n", testcase_send_authorization_message_without_trace_id)
	  || !CU_add_test(ste, "Test testcase_send_authorization_message_empty_trace_id()...\n", testcase_send_authorization_message_empty_trace_id)
	  || !CU_add_test(ste, "Test testcase_send_authorization_message_invalid_trace_id()...\n", testcase_send_authorization_message_invalid_trace_id)
	  || !CU_add_test(ste, "Test testcase_send_authorization_message_long_trace_id()...\n", testcase_send_authorization_message_long_trace_id)
	  || !CU_add_test(ste, "Test testcase_authorization_with_host_and_tty_success()...\n", testcase_authorization_with_host_and_tty_success)
	  || !CU_add_test(ste, "Test testcase_check_and_load_changed_tacacs_config()...\n", testcase_check_and_load_changed_tacacs_config)
	  || !CU_add_test(ste, "Test testcase_on_shell_execve_success()...\n", testcase_on_shell_execve_success)
	  || !CU_add_test(ste, "Test testcase_on_shell_execve_denined()...\n", testcase_on_shell_execve_denined)
	  || !CU_add_test(ste, "Test testcase_on_shell_execve_failed()...\n", testcase_on_shell_execve_failed)
	  || !CU_add_test(ste, "Test testcase_is_local_user_unknown()...\n", testcase_is_local_user_unknown)
	  || !CU_add_test(ste, "Test testcase_is_local_user_not_found()...\n", testcase_is_local_user_not_found)
	  || !CU_add_test(ste, "Test testcase_is_local_user_root()...\n", testcase_is_local_user_root)
	  || !CU_add_test(ste, "Test testcase_is_local_user_remote()...\n", testcase_is_local_user_remote)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (CU_get_error() != CUE_SUCCESS) {
    fprintf(stderr, "Error adding test: (%d)%s\n", CU_get_error(), CU_get_error_msg());
  }

  // run all test
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_ErrorCode run_errors = CU_basic_run_suite(ste);
  if (run_errors != CUE_SUCCESS) {
    fprintf(stderr, "Error running tests: (%d)%s\n", run_errors, CU_get_error_msg());
  }

  CU_basic_show_failures(CU_get_failure_list());

  // use failed UT count as return value
  return CU_get_number_of_failure_records();
}