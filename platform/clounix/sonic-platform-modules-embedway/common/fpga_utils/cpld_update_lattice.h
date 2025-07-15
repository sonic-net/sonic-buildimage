#ifndef CPLD_UPDATE_LATTICE
#define CPLD_UPDATE_LATTICE

#define MAX_CPLD_IMAGE_SIZE 0x40000
#define PAGE_SIZE (128/8)
#define CMD_EXTRA_BYTES 4
#define DEVICE_ID_6900 0x612bd043
#define DEVICE_ID_1300 0xe12bb043
#define DEVICE_ID_1200U 0x012bb043
#define DEVICE_ID_640 0x012b9043
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
int lattice_cpld_update_init(uint32_t index,void **cpld_op);
void lattice_cpld_update_exit(uint32_t index);
#define CPLD_IDX_1 1
#define CPLD_IDX_2 2
#define CPLD_IDX_3 3 /*EVB8T fan cpld   hengwei：CPLD3(CB)*/
#define CPLD_IDX_4 4 /*hengwei CPLD4(FB)*/
#endif 
