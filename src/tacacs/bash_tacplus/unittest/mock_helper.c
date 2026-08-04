/* mock_helper.c -- mock helper for bash plugin UT. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/* Tacacs+ lib */
#include <libtac/libtac.h>

#include "mock_helper.h"

// define BASH_PLUGIN_UT_DEBUG to output UT debug message.
#if defined (BASH_PLUGIN_UT_DEBUG)
#define debug_printf printf
#define debug_vprintf vprintf
#else
#define debug_printf
#define debug_vprintf
#endif

/* Mock syslog buffer */
char mock_syslog_message_buffer[1024];

/* define test scenarios for mock functions return different value by scenario. */
int test_scenario;

/* Mock tac_netop method result buffer. */
char tac_natop_result_buffer[128];

/* Mock tacplus_server_t. */
typedef struct {
    struct addrinfo *addr;
    char key[256];
} tacplus_server_t;

/* Mock VRF name. */
char *__vrfname = "MOCK VRF name";

/* Mock tac timeout setting. */
int tac_timeout = 10;

/* Mock TACACS servers. */
int tac_srv_no = 3;
tacplus_server_t tac_srv[TAC_PLUS_MAXSERVERS];
struct addrinfo tac_srv_addr[TAC_PLUS_MAXSERVERS];
struct sockaddr tac_sock_addr[TAC_PLUS_MAXSERVERS];

/* Mock tac_source_addr. */
struct addrinfo tac_source_addr;

/* define memory allocate counter. */
int memory_allocate_count;

/* Initialize tacacs servers for test*/
void initialize_tacacs_servers()
{
	for (int idx=0; idx < tac_srv_no; idx++)
	{
		// generate address with index
		struct addrinfo hints, *servers;
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "1.2.3.%d", idx);
		getaddrinfo(buffer, "49", &hints, &servers);
		tac_srv[idx].addr = &(tac_srv_addr[idx]);
		memcpy(tac_srv[idx].addr, servers, sizeof(struct addrinfo));

        tac_srv[idx].addr->ai_addr = &(tac_sock_addr[idx]);
        memcpy(tac_srv[idx].addr->ai_addr, servers->ai_addr, sizeof(struct sockaddr));

		snprintf(tac_srv[idx].key, sizeof(tac_srv[idx].key), "key%d", idx);
        freeaddrinfo(servers);

		debug_printf("MOCK: initialize_tacacs_servers with index: %d, address: %p\n", idx, tac_srv[idx].addr);
	}
}

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

/* Mock xcalloc method */
void *xcalloc(size_t count, size_t size)
{
	memory_allocate_count++;
	debug_printf("MOCK: xcalloc memory count: %d\n", memory_allocate_count);
	return malloc(count*size);
}

/* Mock tac_free_attrib method */
void tac_add_attrib(struct tac_attrib **attr, char *attrname, char *attrvalue)
{
	debug_printf("MOCK: tac_add_attrib add attribute: %s, value: %s\n", attrname, attrvalue);
}

/* Mock tac_free_attrib method */
void tac_free_attrib(struct tac_attrib **attr)
{
	memory_allocate_count--;
	debug_printf("MOCK: tac_free_attrib memory count: %d\n", memory_allocate_count);

	// the mock code here only free first allocated memory, because the mock tac_add_attrib implementation not allocate new memory.
	free(*attr);
}

/* Mock tac_author_send method */
int tac_author_send(int tac_fd, const char *user, char *tty, char *host,struct tac_attrib *attr)
{
	debug_printf("MOCK: tac_author_send with fd: %d, user:%s, tty:%s, host:%s, attr:%p\n", tac_fd, user, tty, host, attr);
	if(TEST_SCEANRIO_CONNECTION_SEND_FAILED_RESULT == test_scenario)
	{
		// send auth message failed
		return -1;
	}

	return 0;
}

/* Mock tac_author_read method */
int tac_author_read(int tac_fd, struct areply *reply)
{
	// TODO: fill reply message here for test
	debug_printf("MOCK: tac_author_read with fd: %d\n", tac_fd);
	if (TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_READ_FAILED == test_scenario)
	{
		return -1;
	}

	if (TEST_SCEANRIO_CONNECTION_SEND_DENINED_RESULT == test_scenario)
	{
		reply->status = AUTHOR_STATUS_FAIL;
	}
	else
	{
		reply->status = AUTHOR_STATUS_PASS_REPL;
	}

	return 0;
}

/* Mock tac_connect_single method */
int tac_connect_single(const struct addrinfo *address, const char *key, struct addrinfo *source_address, int timeout, char *vrfname)
{
	debug_printf("MOCK: tac_connect_single with address: %p\n", address);

	switch (test_scenario)
	{
		case TEST_SCEANRIO_CONNECTION_ALL_FAILED:
			return -1;
	}
	return 0;
}

/* Mock tac_ntop method */
char *tac_ntop(const struct sockaddr *address)
{
	for (int idx=0; idx < tac_srv_no; idx++)
	{
		if (address == &(tac_sock_addr[idx]))
		{
			snprintf(tac_natop_result_buffer, sizeof(tac_natop_result_buffer), "TestAddress%d", idx);
			return tac_natop_result_buffer;
		}
	}

	return "UnknownTestAddress";
}

/* Mock parse_config_file method */
int parse_config_file(const char *file)
{
	debug_printf("MOCK: parse_config_file: %s\n", file);
}

/* Mock syslog method */
void mock_syslog(int priority, const char *format, ...)
{
  // set mock message data to buffer for UT.
  memset(mock_syslog_message_buffer, 0, sizeof(mock_syslog_message_buffer));

  va_list args;
  va_start (args, format);
  // save message to buffer to UT check later
  vsnprintf(mock_syslog_message_buffer, sizeof(mock_syslog_message_buffer), format, args);
  va_end (args);

  debug_printf("MOCK: syslog: %s\n", mock_syslog_message_buffer);
}

int mock_getpwent_r(struct passwd *restrict pwbuf,
                      char *buf, size_t buflen,
                      struct passwd **restrict pwbufp)
{
	static char* test_user = "test_user";
	static char* root_user = "root";
	static char* empty_gecos = "";
	static char* remote_gecos = "remote_user";
	*pwbufp = pwbuf;
	switch (test_scenario)
	{
		case TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT:
		case TEST_SCEANRIO_CONNECTION_SEND_DENINED_RESULT:
		case TEST_SCEANRIO_IS_LOCAL_USER_REMOTE:
			pwbuf->pw_name = test_user;
			pwbuf->pw_gecos = remote_gecos;
			pwbuf->pw_uid = 1000;
			return 0;
		case TEST_SCEANRIO_IS_LOCAL_USER_ROOT:
			pwbuf->pw_name = root_user;
			pwbuf->pw_gecos = empty_gecos;
			pwbuf->pw_uid = 0;
			return 0;
		case TEST_SCEANRIO_IS_LOCAL_USER_NOT_FOUND:
			return 1;
	}
	return 1;
}