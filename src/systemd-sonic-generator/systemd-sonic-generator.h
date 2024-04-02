/*------------------------------------------------------------------
 * systemd-sonic-generator.h - Header file
 *
 * Initial: Apr 2021
 *
 * Copyright (c) 2021 by Cisco Systems, Inc.
 *------------------------------------------------------------------
 */
#ifdef __cplusplus
extern "C" {
#endif

/* expose global vars for testing purpose */
extern const char* UNIT_FILE_PREFIX;
extern const char* CONFIG_FILE;
extern const char* MACHINE_CONF_FILE;
extern const char* ASIC_CONF_FORMAT;
extern const char* PLATFORM_FILE_FORMAT;
extern const char* g_lib_systemd;
extern const char* g_etc_systemd;
extern const char* g_unit_file_prefix; 
extern const char* g_config_file;
extern const char* g_machine_config_file;
extern const char* g_asic_conf_format;
extern const char* g_platform_file_format;

/* C-functions under test */
extern const char* get_unit_file_prefix();
extern const char* get_config_file();
extern const char* get_machine_config_file();
extern const char* get_asic_conf_format();
extern char* insert_instance_number(char* unit_file, int instance, const char *instance_prefix);
extern int ssg_main(int argc, char** argv);
extern int get_num_of_asic();
extern int get_install_targets(char* unit_file, char* targets[]);
extern int get_unit_files(char* unit_files[]);
#ifdef __cplusplus
}
#endif
