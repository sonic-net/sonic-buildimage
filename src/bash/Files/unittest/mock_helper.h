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

#include "plugin.h"

#define TEST_MOCK_PLUGIN_HANDLE 				0x1234

#define TEST_SCEANRIO_PLUGIN_NOT_EXIT 			1
#define TEST_SCEANRIO_PLUGIN_EXECVE_NOT_EXIT 	2
#define TEST_SCEANRIO_PLUGIN_UNINIT_NOT_EXIT 	3
#define TEST_SCEANRIO_PLUGIN_INIT_NOT_EXIT 		4
#define TEST_SCEANRIO_PLUGIN_INIT_SUCCESS 		5

#define PLUGIN_NOT_INITIALIZE 					-1
#define PLUGIN_INITIALIZED 						1

/* The global plugin list */
extern PLUGIN_NODE *global_plugin_list;

/* itrace buffer */
extern char mock_itrace_message_buffer[1024];

/* bash run command buffer */
extern char mock_onshell_execve_command_buffer[1024];

/* Set test scenario for test*/
void set_test_scenario(int scenario);

/* Get test scenario for test*/
int get_test_scenario();

/* Set plugin init status for test*/
void set_plugin_init_status(int status);

/* Get plugin init status for test*/
int get_plugin_init_status();

/* Set memory allocate count for test*/
void set_memory_allocate_count(int count);

/* Get memory allocate count for test*/
int get_memory_allocate_count();


#endif /* _MOCK_HELPER_H_ */