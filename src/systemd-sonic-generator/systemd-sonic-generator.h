/*------------------------------------------------------------------
 * systemd-sonic-generator.h - Header file
 *
 * Initial: Apr 2021
 *
 * Copyright (c) 2021 by Cisco Systems, Inc.
 *------------------------------------------------------------------
 */

/* expose global vars for testing purpose */
extern const std::string UNIT_FILE_PREFIX;
extern const std::string CONFIG_FILE;
extern const std::string MACHINE_CONF_FILE;
extern const std::vector<std::string> ASIC_CONF_FORMAT;
extern std::string g_unit_file_prefix;
extern std::string g_config_file;
extern std::string g_machine_config_file;
extern std::vector<std::string> g_asic_conf_format;

/* C-functions under test */
extern const std::string get_unit_file_prefix();
extern const std::string get_config_file();
extern const std::string get_machine_config_file();
extern const std::string get_asic_conf_format();
extern std::string insert_instance_number(const std::string& unit_file, int instance);
extern int ssg_main(int argc, char** argv);
extern int get_num_of_asic();
extern std::vector<std::string> get_install_targets(std::string unit_file);
extern std::vector<std::string> get_unit_files();
