#ifndef CPLD_UPDATE_ANLOGIC_
#define CPLD_UPDATE_ANLOGIC_

#define MAX_CPLD_IMAGE_SIZE 0x40000
#define PAGE_SIZE 256
#define SECTOR_SIZE             0x10000
#define CMD_EXTRA_BYTES 4
#define DEVICE_ID 0x612bd043
/*status bit*/
#define STATUS_FLASH_DONE_FLAG          0x100  /*bit8*/
#define STATUS_ENABLE_CONFIG_FLAG     0x200 /*bit9*/
#define STATUS_BUSY_FLAG                      0x1000
#define STATUS_CONFIG_CHECK                0x3800000

struct cpld_update_info {
	int idx;
    int g_extra_bytes;
    int (*init)(uint32_t index);
    int (*cmdxfer)(uint8_t *SendBufPtr,uint32_t wcnt,uint8_t *RecvBufPtr,uint32_t rcnt);
    void (*exit)(uint32_t index);
};
int anlogic_cpld_update_init(uint32_t index,void **cpld_op);
void anlogic_cpld_update_exit(uint32_t index);
#define CPLD_IDX_1 1 /*CPLD 1*/
#define CPLD_IDX_2 2 /*CPLD 2*/
#define CPLD_IDX_3 3 /*sys cpld*/
#define CPLD_IDX_4 4 /*fan cpld*/
#endif 