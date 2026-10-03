#ifndef _FIRMWARE_COMMON_H
#define _FIRMWARE_COMMON_H

int firmware_error_type(int action, name_info_t *info);
int firmware_check_file_info(name_info_t *info, int main_type, int sub_type, int slot);
int firmware_upgrade(char *file_name, name_info_t *info);
int firmware_upgrade_test(char *file_name, name_info_t *info);
int firmware_upgrade_read_header(char *file_name, name_info_t *info);
firmware_file_type_t firmware_upgrade_file_type_map(char *type_str);
#endif
