/*------------------------------------------------------------------
 * systemd-sonic-generator.h - Header file
 *
 * Initial: Apr 2021
 *
 * Copyright (c) 2021 by Cisco Systems, Inc.
 *------------------------------------------------------------------
 */
// #ifdef __cplusplus
// extern "C" {
// #endif
#include <string>
#include <unordered_set>

/* expose global vars for testing purpose */
extern const char* UNIT_FILE_PREFIX;
extern const char* CONFIG_FILE;
extern const char* MACHINE_CONF_FILE;
extern const char* ASIC_CONF_FORMAT;
extern const char* PLATFORM_FILE_FORMAT;
extern const char* PLATFORM_CONF_FORMAT;
extern const char* g_lib_systemd;
extern const char* g_etc_systemd;
extern const char* g_unit_file_prefix; 
extern const char* g_config_file;
extern const char* g_machine_config_file;
extern const char* g_asic_conf_format;
extern const char* g_platform_file_format;
extern const char* g_platform_conf_format;

/* C-functions under test */
extern const char* get_unit_file_prefix();
extern const char* get_config_file();
extern const char* get_machine_config_file();
extern const char* get_asic_conf_format();
extern const char* get_platform_conf_format();
extern std::string insert_instance_number(const std::string& unit_file, int instance, const std::string& instance_prefix);
extern int ssg_main(int argc, char** argv);
extern int get_num_of_asic();
extern int get_install_targets(std::string unit_file, char* targets[]);
extern int get_unit_files(const char* config_file, char* unit_files[], int unit_files_size);
extern int get_platform_unit_files(char* unit_files[], int unit_files_size);
// #ifdef __cplusplus
// }
// #endif
