#include <stdio.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "plugin.h"
#include "mock_helper.h"

int clean_up() {
  return 0;
}

int start_up() {
  return 0;
}

/* Test plugin not exist scenario */
void testcase_try_load_plugin_by_path_not_exist() {
	set_test_scenario(TEST_SCEANRIO_PLUGIN_NOT_EXIT);

	try_load_plugin_by_path("./testplugin.so");

	CU_ASSERT_STRING_EQUAL(mock_itrace_message_buffer, "Plugin: can't load plugin ./testplugin.so: MOCK error\n");
}

/* Test plugin exist but not support shell_execve scenario */
void testcase_try_load_plugin_by_path_execve_not_exist() {
	set_test_scenario(TEST_SCEANRIO_PLUGIN_EXECVE_NOT_EXIT);

	try_load_plugin_by_path("./testplugin.so");

	CU_ASSERT_STRING_EQUAL(mock_itrace_message_buffer, "Plugin: can't find on_shell_execve function ./testplugin.so: MOCK error\n");
}

/* Test plugin exist but not support plugin_uninit scenario */
void testcase_try_load_plugin_by_path_plugin_uninit_not_exist() {
	set_test_scenario(TEST_SCEANRIO_PLUGIN_UNINIT_NOT_EXIT);

	try_load_plugin_by_path("./testplugin.so");

	CU_ASSERT_STRING_EQUAL(mock_itrace_message_buffer, "Plugin: can't find plugin_uninit function ./testplugin.so: MOCK error\n");
}

/* Test plugin exist but not support plugin_init scenario */
void testcase_try_load_plugin_by_path_plugin_init_not_exist() {
	set_test_scenario(TEST_SCEANRIO_PLUGIN_INIT_NOT_EXIT);

	try_load_plugin_by_path("./testplugin.so");

	CU_ASSERT_STRING_EQUAL(mock_itrace_message_buffer, "Plugin: can't find plugin_init function ./testplugin.so: MOCK error\n");
}

/* Test plugin exist but not support plugin_init scenario */
void testcase_try_load_plugin_by_path_plugin_init_success() {
	set_test_scenario(TEST_SCEANRIO_PLUGIN_INIT_SUCCESS);
	set_memory_allocate_count(0);
	set_plugin_init_status(PLUGIN_NOT_INITIALIZE);

	try_load_plugin_by_path("./testplugin.so");

	// check plugin init success
	CU_ASSERT_EQUAL(get_plugin_init_status(), PLUGIN_INITIALIZED);

	// check API success
	CU_ASSERT_STRING_EQUAL(mock_itrace_message_buffer, "Plugin: plugin ./testplugin.so loaded\n");

	// check global plugin list not empty and contains correct pluginglobal_plugin_list
	CU_ASSERT_NOT_EQUAL(global_plugin_list, NULL);
	CU_ASSERT_EQUAL(global_plugin_list->plugin_handle, TEST_MOCK_PLUGIN_HANDLE);

	// release all loaded plugins
	free_loaded_plugins();

	// check if memory fully released
	CU_ASSERT_EQUAL(global_plugin_list, NULL);
	CU_ASSERT_EQUAL(get_memory_allocate_count(), 0);
}

/* Test free loaded plugins */
void testcase_release_loaded_plugin() {
	set_test_scenario(TEST_SCEANRIO_PLUGIN_INIT_SUCCESS);
	set_memory_allocate_count(0);
	try_load_plugin_by_path("./testplugin.so");

	// check memory allocated
	CU_ASSERT_NOT_EQUAL(get_memory_allocate_count(), 0);

	// check plugin init success
	CU_ASSERT_EQUAL(get_plugin_init_status(), PLUGIN_INITIALIZED);

	// release all loaded plugins
	free_loaded_plugins();

	// check if memory fully released
	CU_ASSERT_EQUAL(global_plugin_list, NULL);
	CU_ASSERT_EQUAL(get_memory_allocate_count(), 0);
}

/* Test load plugin by config */
void testcase_load_plugin_by_config() {
	set_test_scenario(TEST_SCEANRIO_PLUGIN_INIT_SUCCESS);
	set_memory_allocate_count(0);
	load_plugin_by_config("./bash_plugins.conf");

	// check memory allocated
	CU_ASSERT_NOT_EQUAL(get_memory_allocate_count(), 0);

	// check plugin init success
	CU_ASSERT_EQUAL(get_plugin_init_status(), PLUGIN_INITIALIZED);

	// check target plugin in config file loaded
	CU_ASSERT_STRING_EQUAL(mock_itrace_message_buffer, "Plugin: plugin /usr/lib/bash-plugins/another_test_plugin.so loaded\n");

	// check there are 2 plugins loaded
	CU_ASSERT_EQUAL(get_memory_allocate_count(), 2);

	// release all loaded plugins
	free_loaded_plugins();

	// check if memory fully released
	CU_ASSERT_EQUAL(global_plugin_list, NULL);
	printf("Count %d\n", get_memory_allocate_count());
	CU_ASSERT_EQUAL(get_memory_allocate_count(), 0);
}

/* Test invoke on_shell_execve plugin method */
void testcase_invoke_plugin_on_shell_execve() {
	set_test_scenario(TEST_SCEANRIO_PLUGIN_INIT_SUCCESS);
	set_memory_allocate_count(0);
	load_plugin_by_config("./bash_plugins.conf");

	// invoke plugin method
	char** pargv = (char**)0x5234;
	invoke_plugin_on_shell_execve("testuser", "testcommand", pargv);
	printf(mock_onshell_execve_command_buffer);
	CU_ASSERT_STRING_EQUAL(mock_onshell_execve_command_buffer, "on_shell_execve: user: testuser, level: 1, command: testcommand, argv: 0x5234\n");

	// release all loaded plugins
	free_loaded_plugins();

	// check if memory fully released
	CU_ASSERT_EQUAL(global_plugin_list, NULL);
	printf("Count %d\n", get_memory_allocate_count());
	CU_ASSERT_EQUAL(get_memory_allocate_count(), 0);
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

  if (!CU_add_test(ste, "Test testcase_try_load_plugin_by_path_not_exist()...\n", testcase_try_load_plugin_by_path_not_exist)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (!CU_add_test(ste, "Test testcase_try_load_plugin_by_path_execve_not_exist()...\n", testcase_try_load_plugin_by_path_execve_not_exist)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (!CU_add_test(ste, "Test testcase_try_load_plugin_by_path_plugin_uninit_not_exist()...\n", testcase_try_load_plugin_by_path_plugin_uninit_not_exist)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (!CU_add_test(ste, "Test testcase_try_load_plugin_by_path_plugin_init_not_exist()...\n", testcase_try_load_plugin_by_path_plugin_init_not_exist)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (!CU_add_test(ste, "Test testcase_try_load_plugin_by_path_plugin_init_success()...\n", testcase_try_load_plugin_by_path_plugin_init_success)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (!CU_add_test(ste, "Test testcase_release_loaded_plugin()...\n", testcase_release_loaded_plugin)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (!CU_add_test(ste, "Test testcase_load_plugin_by_config()...\n", testcase_load_plugin_by_config)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (!CU_add_test(ste, "Test testcase_invoke_plugin_on_shell_execve()...\n", testcase_invoke_plugin_on_shell_execve)) {
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
