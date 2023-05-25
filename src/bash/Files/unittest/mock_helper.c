/* mock_helper.c -- mock helper for bash plugin UT. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "mock_helper.h"

// define BASH_PLUGIN_UT_DEBUG to output UT debug message.
//#define BASH_PLUGIN_UT_DEBUG
#if defined (BASH_PLUGIN_UT_DEBUG)
#       define debug_printf printf
#else
#       define debug_printf
#endif

/* itrace buffer */
char mock_itrace_message_buffer[1024];

/* bash run command buffer */
char mock_onshell_execve_command_buffer[1024];

/* plugin handles. */
void* mock_plugin_handle = (void*)TEST_MOCK_PLUGIN_HANDLE;
void* mock_plugin_default_function_handle = (void*)0x2234;
void* mock_plugin_on_shell_execve_handle = (void*)0x3234;
char* mock_dlerror_failed = "MOCK error";
char* mock_dlerror = NULL;

/* define test scenarios for mock functions return different value by scenario. */
int test_scenario;

/* define test scenarios for different return value. */
int plugin_init_status;

/* define memory allocate counter. */
int memory_allocate_count;

/* Set test scenario for test*/
void set_test_scenario(int scenario)
{
  test_scenario = scenario;
}

/* Get test scenario for test*/
int get_test_scenario()
{
  return test_scenario;
}

/* Set plugin init status for test*/
void set_plugin_init_status(int status)
{
  plugin_init_status = status;
}

/* Get plugin init status for test*/
int get_plugin_init_status()
{
  return plugin_init_status;
}

/* Set memory allocate count for test*/
void set_memory_allocate_count(int count)
{
  memory_allocate_count = count;
}

/* Get memory allocate count for test*/
int get_memory_allocate_count()
{
  return memory_allocate_count;
}

/* MOCK plugin_init method*/
int mock_plugin_init()
{
  set_plugin_init_status(PLUGIN_INITIALIZED);
}

/* MOCK plugin_init method*/
int mock_plugin_uninit()
{
  set_plugin_init_status(PLUGIN_NOT_INITIALIZE);
}

/* MOCK on_shell_execve method*/
int mock_on_shell_execve (char *user, int shell_level, char *cmd, char **argv)
{
  // set mock command data to buffer for UT.
  memset(mock_onshell_execve_command_buffer, 0, sizeof(mock_onshell_execve_command_buffer));

  snprintf(mock_onshell_execve_command_buffer, sizeof(mock_onshell_execve_command_buffer), "on_shell_execve: user: %s, level: %d, command: %s, argv: %p\n", user, shell_level, cmd, argv);

  debug_printf("MOCK: mock_on_shell_execve: %s\n", mock_onshell_execve_command_buffer);
}

/* MOCK dlopen*/
void *dlopen(const char *filename, int flags)
{
	debug_printf("MOCK: dlopen: %s\n", filename);
  if (TEST_SCEANRIO_PLUGIN_NOT_EXIT == test_scenario)
  {
	  // return null when plugin not exist
	  mock_dlerror = mock_dlerror_failed;
	  return NULL;
  }

  // all other case return mock handle
  mock_dlerror = NULL;
  return mock_plugin_handle;
}

/* MOCK dlclose*/
int dlclose(void *handle)
{
	debug_printf("MOCK: dlclose: %p\n", handle);
	// check if the close handle match the opened handle
	CU_ASSERT_EQUAL(handle, mock_plugin_handle);
}

/* MOCK dlsym*/
void *dlsym(void *restrict handle, const char *restrict symbol)
{
	debug_printf("MOCK: dlsym: %p, %s\n", handle, symbol);
	mock_dlerror = NULL;
	switch (test_scenario)
	{
		case TEST_SCEANRIO_PLUGIN_EXECVE_NOT_EXIT:
			if (strcmp(symbol, "on_shell_execve") == 0)
			{
				mock_dlerror = mock_dlerror_failed;
				return NULL;
			}

		case TEST_SCEANRIO_PLUGIN_UNINIT_NOT_EXIT:
			if (strcmp(symbol, "plugin_uninit") == 0)
			{
				mock_dlerror = mock_dlerror_failed;
				return NULL;
			}

		case TEST_SCEANRIO_PLUGIN_INIT_NOT_EXIT:
			if (strcmp(symbol, "plugin_init") == 0)
			{
				mock_dlerror = mock_dlerror_failed;
				return NULL;
			}

		case TEST_SCEANRIO_PLUGIN_INIT_SUCCESS:
			if (strcmp(symbol, "plugin_init") == 0)
			{
				// return mock method handle so plugin framework will call it to initialize
				return mock_plugin_init;
			}
			else if (strcmp(symbol, "plugin_uninit") == 0)
			{
				// return mock method handle so plugin framework will call it to initialize
				return mock_plugin_uninit;
			}
			else if (strcmp(symbol, "on_shell_execve") == 0)
			{
				// return mock method handle so plugin framework will call it to initialize
				return mock_on_shell_execve;
			}
	}

	return mock_plugin_default_function_handle;
}

/* MOCK dlerror*/
char *dlerror(void)
{
	return mock_dlerror;
}

/* MOCK get_string_value*/
char *get_string_value(const char * str)
{
	return "1";
}

/* MOCK absolute_program*/
int absolute_program (const char * str)
{
	return 0;
}

/* MOCK itrace*/
void itrace (const char * format, ...)
{
  // set mock message data to buffer for UT.
  memset(mock_itrace_message_buffer, 0, sizeof(mock_itrace_message_buffer));

  va_list args;
  va_start(args, format);
  // save message to buffer to UT check later
  vsnprintf(mock_itrace_message_buffer, sizeof(mock_itrace_message_buffer), format, args);
  va_end(args);
  debug_printf("MOCK: itrace: %s\n", mock_itrace_message_buffer);
}

/* MOCK malloc method*/
void* mock_malloc (size_t size)
{
	memory_allocate_count++;
	debug_printf("MOCK: malloc memory count: %d\n", memory_allocate_count);
	return malloc(size);
}

/* MOCK free method*/
void mock_free (void* ptr)
{
	memory_allocate_count--;
	debug_printf("MOCK: free memory count: %d\n", memory_allocate_count);
	free(ptr);
}