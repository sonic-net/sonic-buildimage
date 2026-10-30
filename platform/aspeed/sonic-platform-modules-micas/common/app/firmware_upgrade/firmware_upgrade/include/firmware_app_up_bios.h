#ifndef _FIRMWARE_APP_UP_BIOS_H
#define _FIRMWARE_APP_UP_BIOS_H


/* Private structure for each extended command. 
    For future commands, add a corresponding private structure 
    in the firmware_app_cmd_t structure. */
typedef struct firmware_app_cmd_bios_s {
    int uconf_skip;          /* bios upgrade skip user config flag, 0: not skip, 1: skip */
    int checked;             /* file has been header-checked and matches environment */
    char *up_file_path;
    int remain_argc;
    char **remain_argv;
} firmware_app_cmd_bios_t;

#endif
