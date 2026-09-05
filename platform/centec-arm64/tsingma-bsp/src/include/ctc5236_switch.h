#ifndef __CTC5236_SWITCH_H
#define __CTC5236_SWITCH_H

#define OMCMEM_BASE  0x00041000
#define SYS_TSINGMA_MDIO_CTLR_NUM 2
#define SYS_TSINGMA_TEMP_TABLE_NUM 166
#define SYS_TSINGMA_SENSOR_TIMEOUT 1000

typedef struct ctc_switch_cmd_status_s {
	u32 cmdReadType:1;
	u32 pcieReqCmdChk:3;
	u32 cmdEntryWords:4;
	u32 cmdDataLen:5;
	u32 reserved:1;
	u32 pcieReqError:1;
	u32 pcieDataError:1;
	u32 reqProcDone:1;
	u32 reqProcError:1;
	u32 reqProcTimeout:1;
	u32 reqProcAckError:1;
	u32 reqProcAckCnt:5;
	u32 regProcBusy:1;
	u32 cmdStatusWaitReq:1;
	u32 pciePoisoned:1;
	u32 regProcState:3;
	u32 pcieReqOverlap:1;
} ctc_switch_cmd_status_t;

typedef union drv_pci_cmd_status_u_e {
	ctc_switch_cmd_status_t cmd_status;
	u32 val;
} ctc_switch_cmd_status_u_t;

typedef struct ctc_access_s {
	u32 cmd_status;
	u32 addr;
	u32 data[16];
} ctc_access_t;

extern int ctc5236_switch_read(u32 offset, u32 len, u32 * p_value);
extern int ctc5236_switch_write(u32 offset, u32 len, u32 * p_value);
extern int get_switch_temperature(void);
#endif
