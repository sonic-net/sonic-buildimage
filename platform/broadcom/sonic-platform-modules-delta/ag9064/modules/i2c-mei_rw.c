#include "i2c-mei_rw.h"

#define MicroSecondDelay(time) udelay(time)

/****** Parameter *****/

/****** Struct *****/
typedef struct 
{
  volatile UINT32 CB_WW;   // Circular Buffer Write Window
  volatile UINT32 H_CSR;   // Host Control and Status Register
  volatile UINT32 CB_RW;   // Circular Buffer Read Window
  volatile UINT32 ME_CSR;  // ME Control and Status Register (read only)
} HECI_MBAR_REGS;


typedef union 
{
  UINT32    DWord;
  struct 
  {
    UINT32  H_IE    : 1,  // 0 -  Interrupt Enable ME
            H_IS    : 1,  // 1 -  Interrupt Status ME
            H_IG    : 1,  // 2 -  Interrupt Generate
            H_RDY   : 1,  // 3 -  Ready
            H_RST   : 1,  // 4 -  Reset
            Reserved: 3,  // 5~7
            H_CBRP  : 8,  // 8~15 - CB Read Pointer
            H_CBWP  : 8,  // 16~23 - CB Write Pointer
            H_CBD   : 8;  // 24~31 - Circular Buffer Depth
  } Bits;
} HECI_HOST_CSR;

// HECI_MBAR_REGS::ME_CSR - ME Control and Status Register
typedef union 
{
  UINT32   DWord;
  struct 
  {
    UINT32 ME_IE   : 1,  // 0 -  Interrupt Enable (Host Read Access)
           ME_IS   : 1,  // 1 -  Interrupt Status (Host Read Access)
           ME_IG   : 1,  // 2 -  Interrupt Generate (Host Read Access)
           ME_RDY  : 1,  // 3 -  Ready (Host Read Access)
           ME_RST  : 1,  // 4 -  Reset (Host Read Access)
           Reserved: 3,  // 5~7
           ME_CBRP : 8,  // 8~15 -  CB Read Pointer (Host Read Access)
           ME_CBWP : 8,  // 16~23 -  CB Write Pointer (Host Read Access)
           ME_CBD  : 8;  // 24~31 -  Circular Buffer Depth (Host Read Access)
  } Bits;
} HECI_ME_CSR;

/****** Function  *****/
VOID* HeciMbarRead(IN HECI_DEVICE *pThis);
VOID  HeciTrace(IN HECI_DEVICE*, IN CHAR8*, IN HECI_MSG_HEADER*, IN INT32);

EFI_STATUS HeciInit (   HECI_DEVICE   *pThis,
                        UINT32        *pTimeout)
{
  EFI_STATUS      Status = EFI_SUCCESS;
  UINT32          Timeout = 0;
  HECI_HOST_CSR   sHCsr;
  HECI_ME_CSR     sMeCsr;
  HECI_MBAR_REGS *pMbarRegs;
  VOID *pAddrPoint;
  
  if (pThis == NULL || (pThis->Mbar & 0xF) != 0 || pThis->Hidm > HECI_HIDM_LAST)
  {
    printk("Heci Init Failed");
    return EFI_INVALID_PARAMETER;
  }
  if (pTimeout != NULL)
  {
    Timeout = *pTimeout;
  }

  //  HECI vendor and device information away
  pThis->PciCfg = PCI_LIB_ADDRESS(pThis->Bus, pThis->Dev, pThis->Fun, 0);
  pThis->Vid = PciRead16(pThis->PciCfg + HECI_REG_VENDORID);
  pThis->Did = PciRead16(pThis->PciCfg + HECI_REG_DEVICEID);

  if (pThis->Vid != 0x8086)
  {
    printk("[HECI] Init failed, PCI device %d/%d/%d not valid HECI (%2X-%2X)\n",
           pThis->Bus, pThis->Dev, pThis->Fun, pThis->Vid, pThis->Did);
    return EFI_DEVICE_ERROR;
  }
  
  // Check MBAR, 
  pAddrPoint = HeciMbarRead(pThis);
  pMbarRegs = (HECI_MBAR_REGS*)ioremap_nocache(pAddrPoint, 0x1000);
  if (pMbarRegs == NULL)
  {
    printk("[HECI-%d] Init failed (device disabled)\n", pThis->Fun);
    printk("Check MBAR Failed");
    Status = EFI_DEVICE_ERROR;
    goto GO_FAIL;
  }
  
  // Set HECI interrupt delivery mode.
  sHCsr.DWord = pMbarRegs->H_CSR;
  sHCsr.Bits.H_IE = 0;
  pMbarRegs->H_CSR = sHCsr.DWord;
  PciWrite8(pThis->PciCfg + HECI_REG_HIDM, pThis->Hidm);
 
  // Check HECI was free
  sMeCsr.DWord = pMbarRegs->ME_CSR;
  if (!sMeCsr.Bits.ME_RDY)
  {
    Status = HecClearQue(pThis, &Timeout);
  }
  else
  {
    if (!sHCsr.Bits.H_RDY)
    {
      sHCsr.Bits.H_IG = 1;
      sHCsr.Bits.H_RDY = 1;
      sHCsr.Bits.H_RST = 0;
      pMbarRegs->H_CSR = sHCsr.DWord;
    }
    pThis->HMtu = sHCsr.Bits.H_CBD * sizeof(UINT32) - sizeof(HECI_MSG_HEADER);
    pThis->MeMtu = sMeCsr.Bits.ME_CBD * sizeof(UINT32) - sizeof(HECI_MSG_HEADER);
  }
  
 GO_FAIL:
  if (pTimeout != NULL)
  {
    *pTimeout = Timeout;
  }
  pThis->Mefs1.DWord = HeciPciReadMefs1();
  iounmap((VOID *)pMbarRegs);
  return Status;
} 

EFI_STATUS HecClearQue (    HECI_DEVICE   *pThis,
                            UINT32        *pTimeout)
{
  EFI_STATUS      Status;
  UINT32          Timeout = 0;
  HECI_HOST_CSR   sHCsr;
  HECI_ME_CSR     sMeCsr;
  HECI_MBAR_REGS *pMbarRegs;
  VOID *pAddrPoint;
  
  if (pThis == NULL)
  {
    printk("Failed");
    return EFI_INVALID_PARAMETER;
  }
  
  // Check for HECI availability on PCI
  pAddrPoint = HeciMbarRead(pThis);
  pMbarRegs = (HECI_MBAR_REGS*)ioremap_nocache(pAddrPoint, 0x1000);
  printk("pMbarRegs: %x\n", pMbarRegs);

  if (pMbarRegs == NULL)
  {
    printk("[HECI-%d] Reset failed (device disabled)\n", pThis->Fun);
    printk("Failed");
    return EFI_DEVICE_ERROR;
  }
  if (pTimeout != NULL)
  {
    Timeout = *pTimeout;
  }
  printk("[HECI-%d] Resetting HECI interface (CSR %X/%X)\n",
         pThis->Fun, pMbarRegs->H_CSR, pMbarRegs->ME_CSR);
  
  sHCsr.DWord = pMbarRegs->H_CSR;
  if (!sHCsr.Bits.H_RST)
  {
    sHCsr.Bits.H_RST = 1;
    sHCsr.Bits.H_IG = 1;
    pMbarRegs->H_CSR = sHCsr.DWord;
  }

  // Wait for H_RDY cleared to make sure that the reset started.
  while (1)
  {
    sHCsr.DWord = pMbarRegs->H_CSR;
    if (!sHCsr.Bits.H_RDY)
    {
      break;
    }
    if (Timeout == 0)
    {
      printk("[HECI-%d] Reset failed (timeout)(CSR %X/%X)\n",
             pThis->Fun, pMbarRegs->H_CSR, pMbarRegs->ME_CSR);
      Status = EFI_TIMEOUT;
      goto GO_FAIL;
    }
    MicroSecondDelay(HECI_TIMEOUT_UNIT);
    Timeout--;
  }

  // Wait for ME to perform reset and signal it is ready.
  while (1)
  {
    sMeCsr.DWord = pMbarRegs->ME_CSR;
    if (sMeCsr.Bits.ME_RDY)
    {
      break;
    }
    if (Timeout == 0)
    {
      printk("[HECI-%d] Reset failed (timeout)(CSR %X/%X)\n",
             pThis->Fun, pMbarRegs->H_CSR, pMbarRegs->ME_CSR);
      Status = EFI_TIMEOUT;
      goto GO_FAIL;
    }
    MicroSecondDelay(HECI_TIMEOUT_UNIT);
    Timeout--;
  }
  
  // ME side is ready, signal host side is ready too.
  sHCsr.DWord = pMbarRegs->H_CSR;
  sHCsr.Bits.H_RST = 0;
  sHCsr.Bits.H_RDY = 1;
  sHCsr.Bits.H_IG = 1;
  pMbarRegs->H_CSR = sHCsr.DWord;
  
  // Update MTU, ME could change it during reset.
  pThis->HMtu = sHCsr.Bits.H_CBD * sizeof(UINT32) - sizeof(HECI_MSG_HEADER);
  pThis->MeMtu = sMeCsr.Bits.ME_CBD * sizeof(UINT32) - sizeof(HECI_MSG_HEADER);
  Status = EFI_SUCCESS;
  
GO_FAIL:
  if (pTimeout != NULL)
  {
    *pTimeout = Timeout;
  }
  pThis->Mefs1.DWord = HeciPciReadMefs1();
  iounmap((VOID *)pMbarRegs);
  return Status;
} 

EFI_STATUS HeciMsgRecv (    HECI_DEVICE     *pThis,
                            UINT32          *pTimeout,
                            HECI_MSG_HEADER *pMsgBuf,
                            UINT32          *pBufLen )
{
  EFI_STATUS      Status = EFI_SUCCESS;
  UINT32          Timeout = 0;
  UINT32          DWord, dwDataReads, dwBufLen;
  UINT8           bFilledSlots, bMsgLen = 0;
  HECI_HOST_CSR   sHCsr;
  HECI_ME_CSR     sMeCsr;
  HECI_MBAR_REGS  *pMbarRegs;
  VOID *pAddrPoint;
  
  if (pThis == NULL || pMsgBuf == NULL ||
      pBufLen == NULL || *pBufLen < sizeof(HECI_MSG_HEADER))
  {
    printk("Heci MsgRecv Failed\n");
    return EFI_INVALID_PARAMETER;
  }

  // Check for HECI availability on PCI
  pAddrPoint = HeciMbarRead(pThis);
  pMbarRegs = (HECI_MBAR_REGS*)ioremap_nocache(pAddrPoint, 0x1000);
  if (pMbarRegs == NULL)
  {
    printk("[HECI-%d] Receive failed (device disabled)\n", pThis->Fun);
    printk("pMbarRegs Failed\n");
    return EFI_DEVICE_ERROR;
  }
  if (pTimeout != NULL)
  {
    Timeout = *pTimeout;
  }
  
  
  //  read from queue.
  dwBufLen = *pBufLen;
  *pBufLen = dwDataReads = 0;
  while (1)
  {
    sHCsr.DWord = pMbarRegs->H_CSR;
    sMeCsr.DWord = pMbarRegs->ME_CSR;
    
    bFilledSlots = (UINT8)((INT8)sMeCsr.Bits.ME_CBWP - (INT8)sMeCsr.Bits.ME_CBRP);
    // Is it ready ?
    if (!sMeCsr.Bits.ME_RDY || !sHCsr.Bits.H_RDY || bFilledSlots > sHCsr.Bits.H_CBD)
    {
      Status = HecClearQue(pThis, &Timeout);
      if (EFI_ERROR(Status))
      {
        goto GO_FAIL;
      }
      continue;
    }

    // Read queue 
    while (bFilledSlots-- > 0)
    {
      
      DWord = pMbarRegs->CB_RW;
      if (*pBufLen < dwBufLen)
      {
        
        if (dwDataReads < dwBufLen / sizeof(UINT32))
        {
          ((UINT32*)pMsgBuf)[dwDataReads] = DWord;
          *pBufLen += sizeof(UINT32);
        }
        else
        {
          switch (dwBufLen % sizeof(UINT32))
          {
            case 3: ((UINT8*)pMsgBuf)[*pBufLen + 2] = (UINT8)(DWord >> 16);
            case 2: ((UINT8*)pMsgBuf)[*pBufLen + 1] = (UINT8)(DWord >> 8);
            case 1: ((UINT8*)pMsgBuf)[*pBufLen + 0] = (UINT8)DWord;
          } 
          *pBufLen += dwBufLen % sizeof(UINT32);
        }
      }
      else
      {
        printk("[HECI-%d] Message 0x%08X exceeds buffer size (%dB)\n",
               pThis->Fun, pMsgBuf[0].DWord, dwBufLen);
      }
      dwDataReads++;
      
      // Read message length.
      if (bMsgLen == 0)
      {
        bMsgLen = (UINT8)((pMsgBuf[0].Bits.Length + sizeof(UINT32) - 1) / sizeof(UINT32));
        bMsgLen++; // One more double word for message header
        //
        // Sanity check. If message length exceeds queue length this is
        // not valid header. We are out of synch, let's reset the queue.
        //
        if (bMsgLen > sMeCsr.Bits.ME_CBD)
        {
          printk("[HECI-%d] 0x%08X does not seem to be msg header, reseting...\n",
                 pThis->Fun, pMsgBuf[0].DWord);
          Status = HecClearQue(pThis, &Timeout);
          if (EFI_ERROR(Status))
          {
            goto GO_FAIL;
          }
          *pBufLen = dwDataReads = bMsgLen = 0;
          break; // while (bFilledSlots)
        }
      }
      
      // If message is complete set interrupt to ME to let it know that next
      // message can be sent and exit.
      if (dwDataReads >= bMsgLen)
      {
       
        sMeCsr.DWord = pMbarRegs->ME_CSR;
        sHCsr.DWord = pMbarRegs->H_CSR;
        if (!sMeCsr.Bits.ME_RDY)
        {
          HecClearQue(pThis, &Timeout);
          Status = EFI_ABORTED;
        }
        else
        {
          HeciTrace(pThis, " Got msg: ", pMsgBuf, *pBufLen);
          sHCsr.Bits.H_IG = 1;
          pMbarRegs->H_CSR = sHCsr.DWord;
        }
        goto GO_FAIL;
      }
    } 
    if (Timeout == 0)
    {
      printk("[HECI-%d] Receive failed (timeout)\n", pThis->Fun);
      Status = EFI_TIMEOUT;
      goto GO_FAIL;
    }
    MicroSecondDelay(HECI_TIMEOUT_UNIT);
    Timeout--;
  }
 GO_FAIL:
  if (pTimeout != NULL)
  {
    *pTimeout = Timeout;
  }
  pThis->Mefs1.DWord = HeciPciReadMefs1();
  iounmap((VOID *)pMbarRegs);
  return Status;
} 

EFI_STATUS HeciMsgSend (    HECI_DEVICE     *pThis,
                            UINT32          *pTimeout,
                            HECI_MSG_HEADER *pMessage)
{
  EFI_STATUS      Status;
  UINT32          Timeout = 0;
  UINT8           bEmptySlots;
  UINT8           i, bMsgLen;
  HECI_HOST_CSR   sHCsr;
  HECI_ME_CSR     sMeCsr;
  HECI_MBAR_REGS *pMbarRegs;
  VOID *pAddrPoint;
  
  if (pThis == NULL || pMessage == NULL)
  {
    printk("HeciMsgSend Failed\n");
    return EFI_INVALID_PARAMETER;
  }
  HeciTrace(pThis, "Send msg: ", pMessage, sizeof(HECI_MSG_HEADER) + pMessage->Bits.Length);
  
  // Check for HECI availability 
  pAddrPoint = HeciMbarRead(pThis);
  pMbarRegs = (HECI_MBAR_REGS*)ioremap_nocache(pAddrPoint, 0x1000);
  if (pMbarRegs == NULL)
  {
    printk("[HECI-%d] Send failed (device disabled)\n", pThis->Fun);
    printk("Failed\n");
    return EFI_DEVICE_ERROR;
  }
  if (pTimeout != NULL)
  {
    Timeout = *pTimeout;
  }
  
  bMsgLen = (UINT8)((pMessage->Bits.Length + sizeof(UINT32) - 1) / sizeof(UINT32));
  bMsgLen++; //message header
  while (1)
  {
              sHCsr.DWord = pMbarRegs->H_CSR;
              sMeCsr.DWord = pMbarRegs->ME_CSR;
              
              // If message is more than queue length go fail.
              if (bMsgLen > sHCsr.Bits.H_CBD)
              {
                          printk("[HECI-%d] Send failed (msg %d B, queue %d B only)\n",
                            pThis->Fun, pMessage->Bits.Length, sHCsr.Bits.H_CBD * sizeof(UINT32));
                          Status = EFI_BAD_BUFFER_SIZE;
                          goto GO_FAIL;
              }
              bEmptySlots = (UINT8)sHCsr.Bits.H_CBD -
                     (UINT8)((INT8)sHCsr.Bits.H_CBWP - (INT8)sHCsr.Bits.H_CBRP);

              // Is it ready ?
              if (!sMeCsr.Bits.ME_RDY || !sHCsr.Bits.H_RDY || bEmptySlots > sHCsr.Bits.H_CBD)
              {
                        Status = HecClearQue(pThis, &Timeout);
                        if (EFI_ERROR(Status))
                        {
                        goto GO_FAIL;
                        }
                        continue;
              }
              
              if (bMsgLen <= bEmptySlots)
              {
                      for (i = 0; i < bMsgLen; i++)
                      {
                      pMbarRegs->CB_WW = ((UINT32*)pMessage)[i];
                      }
                      
                      
                      sMeCsr.DWord = pMbarRegs->ME_CSR;
                      if (!sMeCsr.Bits.ME_RDY)
                      {
                              printk("[HECI-%d] Queue has been reset while sending\n", pThis->Fun);
                              continue;
                      }
                      
                      sHCsr.DWord = pMbarRegs->H_CSR;
                      sHCsr.Bits.H_IS = 0; 
                      sHCsr.Bits.H_IG = 1;
                      pMbarRegs->H_CSR = sHCsr.DWord;
                      Status = EFI_SUCCESS;
                      goto GO_FAIL;
              }
             
              
              if (Timeout == 0)
              {
                printk("[HECI-%d] Send failed (timeout)\n", pThis->Fun);
                Status = EFI_TIMEOUT;
                goto GO_FAIL;
              }
              MicroSecondDelay(HECI_TIMEOUT_UNIT);
              Timeout--;
  }
 GO_FAIL:
  if (pTimeout != NULL)
  {
    *pTimeout = Timeout;
  }
  pThis->Mefs1.DWord = HeciPciReadMefs1();
  iounmap((VOID *)pMbarRegs);
  return Status;
} 


VOID *HeciMbarRead(HECI_DEVICE *pThis)
{
  VOID *start;

  UINT16 Cmd;
  union
  {
    UINT64   QWord;
    struct
    {
      UINT32 DWordL;
      UINT32 DWordH;
    } Bits;
  } Mbar;
  
  //
  // Read MBAR. 
  Mbar.QWord = 0;
  Mbar.Bits.DWordL = PciRead32(pThis->PciCfg + HECI_REG_MBAR);
  if (Mbar.Bits.DWordL == 0xFFFFFFFF)
  {
    printk("[HECI-%d] Device disabled\n", pThis->Fun);
    Mbar.Bits.DWordL = 0;
    goto GO_FAIL;
  }
  if (Mbar.Bits.DWordL & 0x4) // if 64-bit address add the upper half
  {
    Mbar.Bits.DWordH = PciRead32(pThis->PciCfg + HECI_REG_MBAR + 4);
  }
  Mbar.Bits.DWordL &= 0xFFFFFFF0; 
  if (Mbar.QWord == 0)
  {
    if (pThis->Mbar == 0)
    {
      printk("[HECI-%d] MBAR not programmed\n", pThis->Fun);
      goto GO_FAIL;
    }
    else
    {
      Mbar.QWord = pThis->Mbar;
      printk("[HECI-%d] MBAR not programmed, using default 0x%08X%08X\n",
             pThis->Fun, Mbar.Bits.DWordH, Mbar.Bits.DWordL);

      // Programm the MBAR, set the 64-bit support bit regardless of the size
      // of the address currently used.
      PciWrite32(pThis->PciCfg + HECI_REG_MBAR + 4, Mbar.Bits.DWordH);
      PciWrite32(pThis->PciCfg + HECI_REG_MBAR, Mbar.Bits.DWordL | 4);
    }
  }
  else
  {
    pThis->Mbar = Mbar.QWord;
  }

  // Enable the MBAR
  Cmd = PciRead16(pThis->PciCfg + HECI_REG_COMMAND);
  if (!(Cmd & HECI_CMD_MSE))
  {
    PciWrite16(pThis->PciCfg + HECI_REG_COMMAND, Cmd | HECI_CMD_BME | HECI_CMD_MSE);
  }
 GO_FAIL:
  return (VOID*)(INTN)Mbar.QWord;
} 



VOID HeciTrace(     HECI_DEVICE     *pThis,
                    CHAR8           *pPrefix,
                    HECI_MSG_HEADER *pMsg,
                    INT32            MsgLen)
{
#if 0     /// Trace Enable or Disable
    if (MsgLen > 4)
    {
        UINT32  dwLineBreak = 0;
        UINT32  dwIndex = 0;
        UINT8   *pMsgBody = (UINT8*)&pMsg[1];
    
        MsgLen -= 4;
        while (MsgLen-- > 0)
        {
            if (dwLineBreak == 0)
                printk("%02x: ", (dwIndex & 0xF0));
            printk("%02x ", pMsgBody[dwIndex++]);
            dwLineBreak++;
            if (dwLineBreak == 16)
            {
                printk("\n");
                dwLineBreak = 0;
            }
            if (dwLineBreak == 8)
            {
                printk("-");
            }
        }
        printk("\n");
    }
#endif
} 