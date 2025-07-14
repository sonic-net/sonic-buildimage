#define MIN_PATH_LEN (16)
#define MAX_NAME_LEN (PAGE_SIZE/2)

#define PATH_FORMAT "Format:\r\nFor create: target_node path link_node_path.\r\nFor del: del link_node_path\r\nThe path format must be full path like \"/sys/bus/...\"\r\n"

#define SYSFS_ROOT "/sys"
#define CLOUNIX_DIR_NAME "switch"
#define DEL_CMD "del "
#define ADD_CMD "add "

#define MAX_NODE_NAME_LEN (64-1)
#define MAX_NODE_INFO_LEN (256-1)

#define DEL_OPS 1
#define ADD_OPS 2
