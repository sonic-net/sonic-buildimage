/* plugin.h - functions from plugin.c. */

/* Copyright (C) 1993-2015 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined (_MOCK_HELPER_H_)
#define _MOCK_HELPER_H_

/* Mock syslog buffer */
extern char mock_syslog_message_buffer[1024];

#define TEST_SCEANRIO_CONNECTION_ALL_FAILED                 1
#define TEST_SCEANRIO_CONNECTION_SEND_FAILED_RESULT         2
#define TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_READ_FAILED   3
#define TEST_SCEANRIO_CONNECTION_SEND_DENINED_RESULT        4
#define TEST_SCEANRIO_CONNECTION_SEND_SUCCESS_RESULT        5
#define TEST_SCEANRIO_LOAD_CHANGED_TACACS_CONFIG            6
#define TEST_SCEANRIO_IS_LOCAL_USER_UNKNOWN                 7
#define TEST_SCEANRIO_IS_LOCAL_USER_NOT_FOUND               8
#define TEST_SCEANRIO_IS_LOCAL_USER_ROOT                    9
#define TEST_SCEANRIO_IS_LOCAL_USER_REMOTE                  10

/* Set test scenario for test*/
void set_test_scenario(int scenario);

/* Get test scenario for test*/
int get_test_scenario();

/* Set memory allocate count for test*/
void set_memory_allocate_count(int count);

/* Get memory allocate count for test*/
int get_memory_allocate_count();


#endif /* _MOCK_HELPER_H_ */