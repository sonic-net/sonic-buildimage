
#include "i2c-mei_io.h"

/****** Parameter *****/

#define HECI_READ_TIMEOUT        12500000  // 12.5sec
#define HECI_SEND_TIMEOUT        12500000  // 12.5sec
#define HECI_TIMEOUT_UNIT        10      


// HECI functions location
#define HECI_BUS   0
#define HECI_DEV   22
#define HECI_FUN   0

// HECI register
#define HECI_REG_VENDORID  0x00
#define HECI_REG_DEVICEID  0x02
#define HECI_REG_COMMAND   0x04
#define HECI_REG_REVID     0x08
#define HECI_REG_MBAR      0x10
#define HECI_REG_IRQ       0x3C
#define HECI_REG_HIDM      0xA0
#define HECI_REG_HFS       0x40
#define HECI_REG_MISC_SHDW 0x44
#define HECI_REG_GS_SHDW   0x48
#define HECI_REG_H_GS      0x4C
#define HECI_REG_GS_SHDW2  0x60
#define HECI_REG_GS_SHDW3  0x64
#define HECI_REG_GS_SHDW4  0x68
#define HECI_REG_GS_SHDW5  0x6C
#define HECI_REG_H_GS2     0x70
#define HECI_REG_H_GS3     0x74
#define HECI_REG_MEFS1     HECI_REG_HFS 
#define HECI_REG_MEFS2     HECI_REG_GS_SHDW 

#define HECI_MBAR_DEFAULT 0xFEDB0000

// HECI Interrupt Delivery Mode to be set in HECI_REG_HIDM.
#define HECI_HIDM_MSI 0
#define HECI_HIDM_SCI 1
#define HECI_HIDM_SMI 2
#define HECI_HIDM_LAST HECI_HIDM_SMI

// HECI command register bits
#define HECI_CMD_BME     0x04  // Bus master enable
#define HECI_CMD_MSE     0x02  // Memory space enable


/****** Struct *****/

typedef union
{
  UINT32   DWord;
  struct
  {
    UINT32 MeAddress  : 8,  // Addressee on ME side
           HostAddress: 8,  // Addressee on host siede, zero for BIOS
           Length     : 9,  // Number of bytes following the header
           Reserved   : 6,
           MsgComplete: 1;  // Whether this is last fragment of a message
  } Bits;
} HECI_MSG_HEADER;

// ME Firmware Status 1 register basics. offset:40h
typedef union
{
  UINT32   DWord;
  struct
  {
    UINT32 CurrentState : 4,  //  0~3   Current ME firmware state
           Reserved_5   : 5,  //  4~8
           InitComplete : 1,  //  9     ME firmware finished initialization
           Reserved_10  : 2,  //  10~11
           ErrorCode    : 4,  //  12~15 If set means fatal error
           OperatingMode: 4,  //  16~19 Current ME operating mode
           Reserved_20  : 5,  //  20~24
           MsgAckData   : 3,  //  25~27 MSG ACK Data specific for acknowledged BIOS message
           MsgAck       : 4;  //  28~31 Acknowledge for register based BIOS message
  } Bits;
} HECI_MEFS1;


typedef struct 
{ 

  UINT8      Bus;     // PCI bus
  UINT8      Dev;     // PCI device 
  UINT8      Fun;     // PCI function number 
  
  UINTN      PciCfg;
  UINT16     Vid;     // Device ID 
  UINT16     Did;     // Vendor ID
  UINT8      Hidm;    // interrupt mode
  UINT64     Mbar;
  UINT32     HMtu;    // Max transfer unit configured by ME minus header
  UINT32     MeMtu;   // Max transfer unit configured by ME minus header
  HECI_MEFS1 Mefs1;   // ME Firmware Status at recent operation
} HECI_DEVICE;

/****** Function  *****/



/**
 * @param pThis     Pointer to HECI device structure
 * @param pTimeout  On input timeout in ms, on exit time left
 */
EFI_STATUS HeciInit (   HECI_DEVICE   *pThis,
                        UINT32        *pTimeout);

/**
 * @param pThis      Pointer to HECI device structure
 * @param pTimeout   On input timeout in ms, on exit time left
 */
EFI_STATUS HecClearQue (    HECI_DEVICE   *pThis,
                            UINT32        *pTimeout);

/**
 * @param pThis      Pointer to HECI device structure
 * @param pTimeout   On input timeout in ms, on exit time left
 * @param pMsgBuf    Buffer for the received message
 * @param pBufLen    On input buffer size, on exit message, in bytes
 */
EFI_STATUS HeciMsgRecv (    HECI_DEVICE     *pThis,
                            UINT32          *pTimeout,
                            HECI_MSG_HEADER *pMsgBuf,
                            UINT32          *pBufLen );



/**
 * @param pThis      Pointer to HECI device structure
 * @param pTimeout   On input timeout in ms, on exit time left
 * @param pMessage   The header of the message to send
 */
EFI_STATUS HeciMsgSend (    HECI_DEVICE     *pThis,
                            UINT32          *pTimeout,
                            HECI_MSG_HEADER *pMessage);


  
#define HeciPciReadMefs1()   PciRead32(PCI_LIB_ADDRESS(HECI_BUS, HECI_DEV, HECI_FUN, HECI_REG_MEFS1))