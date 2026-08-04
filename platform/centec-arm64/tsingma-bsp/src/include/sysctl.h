/*
 * (C) Copyright 2004-2017 Centec Networks (suzhou) Co., LTD.
 * Jay Cao <caoj@centecnetworks.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __CTC5236_SYSCTL_H__
#define __CTC5236_SYSCTL_H__

#ifndef __ASSEMBLY__

/* define SYSCTL_MEM_BASE   0x0000fe00 */
/* defing SYSCTL_REG_BASE   0x00000000 */

struct SysCtl_regs {
	u32 SysResetCtl;	/* 0x00000000 */
	u32 SysResetAutoEn;	/* 0x00000004 */
	u32 SysGicResetCtl;	/* 0x00000008 */
	u32 rsv3;
	u32 SysWdtResetCtl;	/* 0x00000010 */
	u32 SysDmaResetCtl;	/* 0x00000014 */
	u32 SysDdrResetCtl;	/* 0x00000018 */
	u32 SysPcieResetCtl;	/* 0x0000001c */
	u32 SysMacResetCtl;	/* 0x00000020 */
	u32 SysMshResetCtl;	/* 0x00000024 */
	u32 SysUsbResetCtl;	/* 0x00000028 */
	u32 SysSpiResetCtl;	/* 0x0000002c */
	u32 SysQspiResetCtl;	/* 0x00000030 */
	u32 SysAxiSupResetCtl;	/* 0x00000034 */
	u32 SysGpioResetCtl;	/* 0x00000038 */
	u32 SysI2CResetCtl;	/* 0x0000003c */
	u32 SysMdioSocResetCtl;	/* 0x00000040 */
	u32 SysTimerResetCtl;	/* 0x00000044 */
	u32 SysUartResetCtl;	/* 0x00000048 */
	u32 SysTraceResetCtl;	/* 0x0000004c */
	u32 SysDbg0ResetEnCtl;	/* 0x00000050 */
	u32 SysDbg1ResetEnCtl;	/* 0x00000054 */
	u32 SysWarm0ResetEnCtl;	/* 0x00000058 */
	u32 SysWarm1ResetEnCtl;	/* 0x0000005c */
	u32 SysWdt0ResetEnCtl;	/* 0x00000060 */
	u32 SysWdt1ResetEnCtl;	/* 0x00000064 */
	u32 SysCtlSysRev;	/* 0x00000068 */
	u32 SysEnClkCfg;	/* 0x0000006c */
	u32 SysPllSocCfg0;	/* 0x00000070 */
	u32 SysPllSocCfg1;	/* 0x00000074 */
	u32 SysPllDdrCfg0;	/* 0x00000078 */
	u32 SysPllDdrCfg1;	/* 0x0000007c */
	u32 SysClkSelCfg;	/* 0x00000080 */
	u32 SysClkDivCfg;	/* 0x00000084 */
	u32 SysClkPeriCfg[2];	/* 0x00000088 */
	u32 SysDbgCreditCtl;	/* 0x00000090 */
	u32 rsv37;
	u32 SysI2CMultiCtl;	/* 0x00000098 */
	u32 BootStrapPin;	/* 0x0000009c */
	u32 SysCntValue[2];	/* 0x000000a0 */
	u32 SysCntLog;		/* 0x000000a8 */
	u32 rsv43;
	u32 rsv44;
	u32 rsv45;
	u32 SysCoreGicAddrBase;	/* 0x000000b8 */
	u32 rsv47;
	u32 SysCorePmCfg;	/* 0x000000c0 */
	u32 SysCoreStatus;	/* 0x000000c4 */
	u32 SysCorePmuEvent0;	/* 0x000000c8 */
	u32 SysCorePmuEvent1;	/* 0x000000cc */
	u32 SysRstVecBar0[2];	/* 0x000000d0 */
	u32 SysRstVecBar1[2];	/* 0x000000d8 */
	u32 SysGicCfg;		/* 0x000000e0 */
	u32 SysGicStatus;	/* 0x000000e4 */
	u32 rsv58;
	u32 SysMemCtl;		/* 0x000000ec */
	u32 SysDmaMapCfg;	/* 0x000000f0 */
	u32 SysDmaAbortCfg;	/* 0x000000f4 */
	u32 rsv62;
	u32 rsv63;
	u32 SysCstCfg;		/* 0x00000100 */
	u32 SysCstTargetIdCfg;	/* 0x00000104 */
	u32 SysCstMemMapIdCfg;	/* 0x00000108 */
	u32 rsv67;
	u32 SysQspiBootCfg0;	/* 0x00000110 */
	u32 SysQspiBootCfg1;	/* 0x00000114 */
	u32 SysQspiBootCfg2;	/* 0x00000118 */
	u32 SysQspiBootCfg3;	/* 0x0000011c */
	u32 SysDdrCfg;		/* 0x00000120 */
	u32 rsv73;
	u32 SysSpiSelCfg;	/* 0x00000128 */
	u32 SysMdioSocCfg;	/* 0x0000012c */
	u32 SysWdt0Cnt;		/* 0x00000130 */
	u32 SysWdt0Rev;		/* 0x00000134 */
	u32 SysWdt1Cnt;		/* 0x00000138 */
	u32 SysWdt1Rev;		/* 0x0000013c */
	u32 SysMshCfg;		/* 0x00000140 */
	u32 SysMshStatus;	/* 0x00000144 */
	u32 SysUsbCfg0;		/* 0x00000148 */
	u32 SysUsbCfg1;		/* 0x0000014c */
	u32 SysUsbCfg2;		/* 0x00000150 */
	u32 SysUsbStatus;	/* 0x00000154 */
	u32 rsv86;
	u32 rsv87;
	u32 rsv88;
	u32 rsv89;
	u32 rsv90;
	u32 rsv91;
	u32 rsv92;
	u32 rsv93;
	u32 SysPcieBaseCfg;	/* 0x00000178 */
	u32 SysRegCfg;		/* 0x0000017c */
	u32 SysInitCtl[2];	/* 0x00000180 */
	u32 SysPllSupCfg0;	/* 0x00000188 */
	u32 SysPllSupCfg1;	/* 0x0000018c */
	u32 SysApbProcTimer;	/* 0x00000190 */
	u32 rsv101;
	u32 SysUsbTest[2];	/* 0x00000198 */
	u32 SysGpioMultiCtl;	/* 0x000001a0 */
	u32 rsv105;
	u32 SysGpioHsMultiCtl[2];	/* 0x000001a8 */
	u32 SysPcieMemCtl;	/* 0x000001b0 */
	u32 rsv109;
	u32 SysPcieStatus[2];	/* 0x000001b8 */
	u32 SysMsixStatus[8];	/* 0x000001c0 */
	u32 SysMsixMask[8];	/* 0x000001e0 */
	u32 SysMsixAddr;	/* 0x00000200 */
	u32 SysMsixVecCtl;	/* 0x00000204 */
	u32 SysMsixAddrEn;	/* 0x00000208 */
	u32 SysMsixIntrLog;	/* 0x0000020c */
	u32 SysDebugCtl;	/* 0x00000210 */
	u32 rsv133;
	u32 rsv134;
	u32 rsv135;
	u32 SysMsixPending[8];	/* 0x00000220 */
	u32 SysPwmCtl[8];	/* 0x00000240 */
	u32 SysTachLog[8];	/* 0x00000260 */
	u32 MonAxiCpuCurInfo[14];	/* 0x00000280 */
	u32 rsv174;
	u32 rsv175;
	u32 MonAxiCpuLogInfo[14];	/* 0x000002c0 */
	u32 rsv190;
	u32 rsv191;
	u32 MonAxiDdr0CurInfo[14];	/* 0x00000300 */
	u32 rsv206;
	u32 rsv207;
	u32 MonAxiDdr0LogInfo[14];	/* 0x00000340 */
	u32 rsv222;
	u32 rsv223;
	u32 MonAxiDdr1CurInfo[10];	/* 0x00000380 */
	u32 rsv234;
	u32 rsv235;
	u32 rsv236;
	u32 rsv237;
	u32 rsv238;
	u32 rsv239;
	u32 MonAxiDdr1LogInfo[10];	/* 0x000003c0 */
	u32 rsv250;
	u32 rsv251;
	u32 rsv252;
	u32 rsv253;
	u32 rsv254;
	u32 rsv255;
	u32 MonAxiMemCurInfo[10];	/* 0x00000400 */
	u32 rsv266;
	u32 rsv267;
	u32 rsv268;
	u32 rsv269;
	u32 rsv270;
	u32 rsv271;
	u32 MonAxiMemLogInfo[10];	/* 0x00000440 */
	u32 rsv282;
	u32 rsv283;
	u32 rsv284;
	u32 rsv285;
	u32 rsv286;
	u32 rsv287;
	u32 MonAxiMshCurInfo[9];	/* 0x00000480 */
	u32 rsv297;
	u32 rsv298;
	u32 rsv299;
	u32 rsv300;
	u32 rsv301;
	u32 rsv302;
	u32 rsv303;
	u32 MonAxiMshLogInfo[9];	/* 0x000004c0 */
	u32 rsv313;
	u32 rsv314;
	u32 rsv315;
	u32 rsv316;
	u32 rsv317;
	u32 rsv318;
	u32 rsv319;
	u32 MonAxiPcieCurInfo[10];	/* 0x00000500 */
	u32 rsv330;
	u32 rsv331;
	u32 rsv332;
	u32 rsv333;
	u32 rsv334;
	u32 rsv335;
	u32 MonAxiPcieLogInfo[10];	/* 0x00000540 */
	u32 rsv346;
	u32 rsv347;
	u32 rsv348;
	u32 rsv349;
	u32 rsv350;
	u32 rsv351;
	u32 MonAxiQspiCurInfo[7];	/* 0x00000580 */
	u32 rsv359;
	u32 MonAxiQspiLogInfo[8];	/* 0x000005a0 */
	u32 MonAxiSupCurInfo[8];	/* 0x000005c0 */
	u32 MonAxiSupLogInfo[8];	/* 0x000005e0 */
	u32 MonSysApbCurInfo[4];	/* 0x00000600 */
	u32 MonSysApbLogInfo[4];	/* 0x00000610 */
	u32 MonSecApbCurInfo[4];	/* 0x00000620 */
	u32 MonSecApbLogInfo[4];	/* 0x00000630 */
	u32 DebugCpuCnt[2];	/* 0x00000640 */
	u32 DebugMemCnt[2];	/* 0x00000648 */
	u32 DebugDdrCnt[4];	/* 0x00000650 */
	u32 DebugMshCnt[2];	/* 0x00000660 */
	u32 DebugPcieCnt[2];	/* 0x00000668 */
	u32 DebugQspiCnt[2];	/* 0x00000670 */
	u32 DebugSupCnt[2];	/* 0x00000678 */
	u32 SysSpiVecLog[5];	/* 0x00000680 */
	u32 rsv421;
	u32 rsv422;
	u32 rsv423;
	u32 DebugAhbRespCnt;	/* 0x000006a0 */
	u32 DebugGicRespCnt;	/* 0x000006a4 */
	u32 DebugMemPtrCfg;	/* 0x000006a8 */
	u32 SysDdrEccCtl;	/* 0x000006ac */
	u32 SysDdrInitStartAddr[2];	/* 0x000006b0 */
	u32 SysDdrInitLastAddr[2];	/* 0x000006b8 */
	u32 SysDdrInitData[2];	/* 0x000006c0 */
	u32 SysDdrInitMode;	/* 0x000006c8 */
	u32 SysDdrInitCtl;	/* 0x000006cc */
	u32 rsv436;
	u32 rsv437;
	u32 rsv438;
	u32 rsv439;
	u32 rsv440;
	u32 rsv441;
	u32 rsv442;
	u32 rsv443;
	u32 rsv444;
	u32 rsv445;
	u32 rsv446;
	u32 rsv447;
	u32 Grant0IntMask;	/* 0x00000700 */
	u32 Grant0IntCtl;	/* 0x00000704 */
	u32 Grant1IntMask;	/* 0x00000708 */
	u32 Grant1IntCtl;	/* 0x0000070c */
	u32 Grant2IntMask;	/* 0x00000710 */
	u32 Grant2IntCtl;	/* 0x00000714 */
	u32 Grant3IntMask;	/* 0x00000718 */
	u32 Grant3IntCtl;	/* 0x0000071c */
	u32 SysIntrIntCtl[2];	/* 0x00000720 */
	u32 SupIntrIntCtl[2];	/* 0x00000728 */
	u32 SysMiscInfo0;	/* 0x00000730 */
	u32 SysMiscInfo1;	/* 0x00000734 */
	u32 SysMiscInfo2;	/* 0x00000738 */
	u32 SysMiscInfo3;	/* 0x0000073c */
	u32 CommonInfo0;	/* 0x00000740 */
	u32 CommonInfo1;	/* 0x00000744 */
	u32 CommonInfo2;	/* 0x00000748 */
	u32 CommonInfo3;	/* 0x0000074c */
	u32 rsv468;
	u32 rsv469;
	u32 rsv470;
	u32 rsv471;
	u32 rsv472;
	u32 rsv473;
	u32 rsv474;
	u32 rsv475;
	u32 rsv476;
	u32 rsv477;
	u32 rsv478;
	u32 rsv479;
	u32 Grant0ExtMask;	/* 0x00000780 */
	u32 Grant0ExtCtl;	/* 0x00000784 */
	u32 Grant1ExtMask;	/* 0x00000788 */
	u32 Grant1ExtCtl;	/* 0x0000078c */
	u32 Grant2ExtMask;	/* 0x00000790 */
	u32 Grant2ExtCtl;	/* 0x00000794 */
	u32 Grant3ExtMask;	/* 0x00000798 */
	u32 Grant3ExtCtl;	/* 0x0000079c */
	u32 SysIntrExtCtl[2];	/* 0x000007a0 */
	u32 SupIntrExtCtl[2];	/* 0x000007a8 */
	u32 SupMiscInfo0;	/* 0x000007b0 */
	u32 SupMiscInfo1;	/* 0x000007b4 */
	u32 SupMiscInfo2;	/* 0x000007b8 */
	u32 SupMiscInfo3;	/* 0x000007bc */
	u32 SysBusDbgEn[2];	/* 0x000007c0 */
	u32 SysApbErrLog;	/* 0x000007c8 */
	u32 MshClkPadSchmitEn;	/* 0x000007cc */
	u32 SysI2C0DebugStatus;	/* 0x000007d0 */
	u32 SysI2C1DebugStatus;	/* 0x000007d4 */
};

/* ################################################################################
 * # SysResetCtl Definition
 */
#define SYS_RESET_CTL_W0_CFG_NIC_RESET                               BIT(9)
#define SYS_RESET_CTL_W0_LOG_CST_RESET                               BIT(23)
#define SYS_RESET_CTL_W0_CFG_SYS_APB_RESET                           BIT(12)
#define SYS_RESET_CTL_W0_LOG_NIC_RESET                               BIT(25)
#define SYS_RESET_CTL_W0_LOG_CPU0_COLD_RESET                         BIT(16)
#define SYS_RESET_CTL_W0_CFG_CPU_APB_RESET                           BIT(5)
#define SYS_RESET_CTL_W0_CFG_CPU1_COLD_RESET                         BIT(1)
#define SYS_RESET_CTL_W0_CFG_CPU1_CORE_RESET                         BIT(3)
#define SYS_RESET_CTL_W0_LOG_CPU_APB_RESET                           BIT(21)
#define SYS_RESET_CTL_W0_CFG_SEC_APB_RESET                           BIT(11)
#define SYS_RESET_CTL_W0_CFG_CPU0_CORE_RESET                         BIT(2)
#define SYS_RESET_CTL_W0_LOG_SEC_APB_RESET                           BIT(27)
#define SYS_RESET_CTL_W0_CFG_CPU0_COLD_RESET                         BIT(0)
#define SYS_RESET_CTL_W0_CFG_CST_RESET                               BIT(7)
#define SYS_RESET_CTL_W0_LOG_JTAG_POT_RESET                          BIT(22)
#define SYS_RESET_CTL_W0_LOG_CPU1_CORE_RESET                         BIT(19)
#define SYS_RESET_CTL_W0_LOG_SYS_APB_RESET                           BIT(28)
#define SYS_RESET_CTL_W0_LOG_CPU0_CORE_RESET                         BIT(18)
#define SYS_RESET_CTL_W0_LOG_CPU1_COLD_RESET                         BIT(17)
#define SYS_RESET_CTL_W0_LOG_CPU_L2_RESET                            BIT(20)
#define SYS_RESET_CTL_W0_LOG_GPV_RESET                               BIT(26)
#define SYS_RESET_CTL_W0_CFG_CPU_L2_RESET                            BIT(4)
#define SYS_RESET_CTL_W0_CFG_GPV_RESET                               BIT(10)
#define SYS_RESET_CTL_W0_CFG_JTAG_POT_RESET                          BIT(6)
#define SYS_RESET_CTL_W0_CFG_CPU_MEM_RESET                           BIT(8)
#define SYS_RESET_CTL_W0_LOG_CPU_MEM_RESET                           BIT(24)

#define SYS_RESET_CTL_W0_CFG_NIC_RESET_MASK                          0x00000200
#define SYS_RESET_CTL_W0_LOG_CST_RESET_MASK                          0x00800000
#define SYS_RESET_CTL_W0_CFG_SYS_APB_RESET_MASK                      0x00001000
#define SYS_RESET_CTL_W0_LOG_NIC_RESET_MASK                          0x02000000
#define SYS_RESET_CTL_W0_LOG_CPU0_COLD_RESET_MASK                    0x00010000
#define SYS_RESET_CTL_W0_CFG_CPU_APB_RESET_MASK                      0x00000020
#define SYS_RESET_CTL_W0_CFG_CPU1_COLD_RESET_MASK                    0x00000002
#define SYS_RESET_CTL_W0_CFG_CPU1_CORE_RESET_MASK                    0x00000008
#define SYS_RESET_CTL_W0_LOG_CPU_APB_RESET_MASK                      0x00200000
#define SYS_RESET_CTL_W0_CFG_SEC_APB_RESET_MASK                      0x00000800
#define SYS_RESET_CTL_W0_CFG_CPU0_CORE_RESET_MASK                    0x00000004
#define SYS_RESET_CTL_W0_LOG_SEC_APB_RESET_MASK                      0x08000000
#define SYS_RESET_CTL_W0_CFG_CPU0_COLD_RESET_MASK                    0x00000001
#define SYS_RESET_CTL_W0_CFG_CST_RESET_MASK                          0x00000080
#define SYS_RESET_CTL_W0_LOG_JTAG_POT_RESET_MASK                     0x00400000
#define SYS_RESET_CTL_W0_LOG_CPU1_CORE_RESET_MASK                    0x00080000
#define SYS_RESET_CTL_W0_LOG_SYS_APB_RESET_MASK                      0x10000000
#define SYS_RESET_CTL_W0_LOG_CPU0_CORE_RESET_MASK                    0x00040000
#define SYS_RESET_CTL_W0_LOG_CPU1_COLD_RESET_MASK                    0x00020000
#define SYS_RESET_CTL_W0_LOG_CPU_L2_RESET_MASK                       0x00100000
#define SYS_RESET_CTL_W0_LOG_GPV_RESET_MASK                          0x04000000
#define SYS_RESET_CTL_W0_CFG_CPU_L2_RESET_MASK                       0x00000010
#define SYS_RESET_CTL_W0_CFG_GPV_RESET_MASK                          0x00000400
#define SYS_RESET_CTL_W0_CFG_JTAG_POT_RESET_MASK                     0x00000040
#define SYS_RESET_CTL_W0_CFG_CPU_MEM_RESET_MASK                      0x00000100
#define SYS_RESET_CTL_W0_LOG_CPU_MEM_RESET_MASK                      0x01000000

/* ################################################################################
 * # SysResetAutoEn Definition
 */
#define SYS_RESET_AUTO_EN_W0_CFG_CPU1_CORE_RESET_AUTO_EN             BIT(3)
#define SYS_RESET_AUTO_EN_W0_CFG_NIC_RESET_AUTO_EN                   BIT(9)
#define SYS_RESET_AUTO_EN_W0_CFG_CST_RESET_AUTO_EN                   BIT(7)
#define SYS_RESET_AUTO_EN_W0_CFG_WDT1_RESET_AUTO_EN                  BIT(14)
#define SYS_RESET_AUTO_EN_W0_CFG_CPU0_COLD_RESET_AUTO_EN             BIT(0)
#define SYS_RESET_AUTO_EN_W0_CFG_GPV_RESET_AUTO_EN                   BIT(10)
#define SYS_RESET_AUTO_EN_W0_CFG_WDT0_RESET_AUTO_EN                  BIT(13)
#define SYS_RESET_AUTO_EN_W0_CFG_JTAG_POT_RESET_AUTO_EN              BIT(6)
#define SYS_RESET_AUTO_EN_W0_CFG_SEC_APB_RESET_AUTO_EN               BIT(11)
#define SYS_RESET_AUTO_EN_W0_CFG_CPU_MEM_RESET_AUTO_EN               BIT(8)
#define SYS_RESET_AUTO_EN_W0_CFG_SYS_APB_RESET_AUTO_EN               BIT(12)
#define SYS_RESET_AUTO_EN_W0_CFG_CPU_L2_RESET_AUTO_EN                BIT(4)
#define SYS_RESET_AUTO_EN_W0_CFG_CPU_APB_RESET_AUTO_EN               BIT(5)
#define SYS_RESET_AUTO_EN_W0_CFG_CPU1_COLD_RESET_AUTO_EN             BIT(1)
#define SYS_RESET_AUTO_EN_W0_CFG_CPU0_CORE_RESET_AUTO_EN             BIT(2)

#define SYS_RESET_AUTO_EN_W0_CFG_CPU1_CORE_RESET_AUTO_EN_MASK        0x00000008
#define SYS_RESET_AUTO_EN_W0_CFG_NIC_RESET_AUTO_EN_MASK              0x00000200
#define SYS_RESET_AUTO_EN_W0_CFG_CST_RESET_AUTO_EN_MASK              0x00000080
#define SYS_RESET_AUTO_EN_W0_CFG_WDT1_RESET_AUTO_EN_MASK             0x00004000
#define SYS_RESET_AUTO_EN_W0_CFG_CPU0_COLD_RESET_AUTO_EN_MASK        0x00000001
#define SYS_RESET_AUTO_EN_W0_CFG_GPV_RESET_AUTO_EN_MASK              0x00000400
#define SYS_RESET_AUTO_EN_W0_CFG_WDT0_RESET_AUTO_EN_MASK             0x00002000
#define SYS_RESET_AUTO_EN_W0_CFG_JTAG_POT_RESET_AUTO_EN_MASK         0x00000040
#define SYS_RESET_AUTO_EN_W0_CFG_SEC_APB_RESET_AUTO_EN_MASK          0x00000800
#define SYS_RESET_AUTO_EN_W0_CFG_CPU_MEM_RESET_AUTO_EN_MASK          0x00000100
#define SYS_RESET_AUTO_EN_W0_CFG_SYS_APB_RESET_AUTO_EN_MASK          0x00001000
#define SYS_RESET_AUTO_EN_W0_CFG_CPU_L2_RESET_AUTO_EN_MASK           0x00000010
#define SYS_RESET_AUTO_EN_W0_CFG_CPU_APB_RESET_AUTO_EN_MASK          0x00000020
#define SYS_RESET_AUTO_EN_W0_CFG_CPU1_COLD_RESET_AUTO_EN_MASK        0x00000002
#define SYS_RESET_AUTO_EN_W0_CFG_CPU0_CORE_RESET_AUTO_EN_MASK        0x00000004

/* ################################################################################
 * # SysGicResetCtl Definition
 */
#define SYS_GIC_RESET_CTL_W0_CFG_GIC_RESET                           BIT(0)

#define SYS_GIC_RESET_CTL_W0_CFG_GIC_RESET_MASK                      0x00000001

/* ################################################################################
 * # SysWdtResetCtl Definition
 */
#define SYS_WDT_RESET_CTL_W0_LOG_WDT1_RESET                          BIT(5)
#define SYS_WDT_RESET_CTL_W0_CFG_WDT0_RESET                          BIT(0)
#define SYS_WDT_RESET_CTL_W0_LOG_WDT0_RESET                          BIT(4)
#define SYS_WDT_RESET_CTL_W0_CFG_WDT1_RESET                          BIT(1)

#define SYS_WDT_RESET_CTL_W0_LOG_WDT1_RESET_MASK                     0x00000020
#define SYS_WDT_RESET_CTL_W0_CFG_WDT0_RESET_MASK                     0x00000001
#define SYS_WDT_RESET_CTL_W0_LOG_WDT0_RESET_MASK                     0x00000010
#define SYS_WDT_RESET_CTL_W0_CFG_WDT1_RESET_MASK                     0x00000002

/* ################################################################################
 * # SysDmaResetCtl Definition
 */
#define SYS_DMA_RESET_CTL_W0_CFG_CPU_DMA_RESET                       BIT(0)

#define SYS_DMA_RESET_CTL_W0_CFG_CPU_DMA_RESET_MASK                  0x00000001

/* ################################################################################
 * # SysDdrResetCtl Definition
 */
#define SYS_DDR_RESET_CTL_W0_CFG_DDR_MC_RESET                        BIT(2)
#define SYS_DDR_RESET_CTL_W0_CFG_DDR_CFG_RESET                       BIT(0)
#define SYS_DDR_RESET_CTL_W0_CFG_DDR_AXI_RESET                       BIT(1)

#define SYS_DDR_RESET_CTL_W0_CFG_DDR_MC_RESET_MASK                   0x00000004
#define SYS_DDR_RESET_CTL_W0_CFG_DDR_CFG_RESET_MASK                  0x00000001
#define SYS_DDR_RESET_CTL_W0_CFG_DDR_AXI_RESET_MASK                  0x00000002

/* ################################################################################
 * # SysPcieResetCtl Definition
 */
#define SYS_PCIE_RESET_CTL_W0_CFG_PIPE_RESET                         BIT(1)
#define SYS_PCIE_RESET_CTL_W0_CFG_PHY_REG_RESET                      BIT(3)
#define SYS_PCIE_RESET_CTL_W0_CFG_PCIE_RESET                         BIT(0)
#define SYS_PCIE_RESET_CTL_W0_CFG_PCIE_POR                           BIT(2)
#define SYS_PCIE_RESET_CTL_W0_CFG_PCIE_SUP_RESET                	 BIT(4)

#define SYS_PCIE_RESET_CTL_W0_CFG_PIPE_RESET_MASK                    0x00000002
#define SYS_PCIE_RESET_CTL_W0_CFG_PHY_REG_RESET_MASK                 0x00000008
#define SYS_PCIE_RESET_CTL_W0_CFG_PCIE_RESET_MASK                    0x00000001
#define SYS_PCIE_RESET_CTL_W0_CFG_PCIE_POR_MASK                      0x00000004
#define SYS_PCIE_RESET_CTL_W0_CFG_PCIE_SUP_RESET_MASK                0x00000010

/* ################################################################################
 * # SysMacResetCtl Definition
 */
#define SYS_MAC_RESET_CTL_W0_CFG_CPU_MAC_RESET                       BIT(0)

#define SYS_MAC_RESET_CTL_W0_CFG_CPU_MAC_RESET_MASK                  0x00000001

/* ################################################################################
 * # SysMshResetCtl Definition
 */
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_C_TX_RESET                      BIT(3)
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_C_RX_DLL_RESET                  BIT(6)
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_CFG_RESET                       BIT(1)
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_REF_C_RESET                     BIT(4)
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_C_RX_RESET                      BIT(2)
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_AXI_RESET                       BIT(0)
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_TM_RESET                        BIT(5)

#define SYS_MSH_RESET_CTL_W0_CFG_MSH_C_TX_RESET_MASK                 0x00000008
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_C_RX_DLL_RESET_MASK             0x00000040
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_CFG_RESET_MASK                  0x00000002
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_REF_C_RESET_MASK                0x00000010
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_C_RX_RESET_MASK                 0x00000004
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_AXI_RESET_MASK                  0x00000001
#define SYS_MSH_RESET_CTL_W0_CFG_MSH_TM_RESET_MASK                   0x00000020

/* ################################################################################
 * # SysUsbResetCtl Definition
 */
#define SYS_USB_RESET_CTL_W0_CFG_USB_INTF_RESET                      BIT(0)
#define SYS_USB_RESET_CTL_W0_CFG_USB_PHY_RESET                       BIT(2)
#define SYS_USB_RESET_CTL_W0_CFG_USB_PHY_ATE_RESET                   BIT(6)
#define SYS_USB_RESET_CTL_W0_CFG_USB_PHY_PORT_RESET                  BIT(5)
#define SYS_USB_RESET_CTL_W0_CFG_USB_UTMI_RESET                      BIT(3)
#define SYS_USB_RESET_CTL_W0_CFG_USB_AUX_RESET                       BIT(1)
#define SYS_USB_RESET_CTL_W0_CFG_USB_PHY_PWR_ON_RESET                BIT(4)

#define SYS_USB_RESET_CTL_W0_CFG_USB_INTF_RESET_MASK                 0x00000001
#define SYS_USB_RESET_CTL_W0_CFG_USB_PHY_RESET_MASK                  0x00000004
#define SYS_USB_RESET_CTL_W0_CFG_USB_PHY_ATE_RESET_MASK              0x00000040
#define SYS_USB_RESET_CTL_W0_CFG_USB_PHY_PORT_RESET_MASK             0x00000020
#define SYS_USB_RESET_CTL_W0_CFG_USB_UTMI_RESET_MASK                 0x00000008
#define SYS_USB_RESET_CTL_W0_CFG_USB_AUX_RESET_MASK                  0x00000002
#define SYS_USB_RESET_CTL_W0_CFG_USB_PHY_PWR_ON_RESET_MASK           0x00000010

/* ################################################################################
 * # SysSpiResetCtl Definition
 */
#define SYS_SPI_RESET_CTL_W0_CFG_SPI_RESET                           BIT(0)

#define SYS_SPI_RESET_CTL_W0_CFG_SPI_RESET_MASK                      0x00000001

/* ################################################################################
 * # SysQspiResetCtl Definition
 */
#define SYS_QSPI_RESET_CTL_W0_CFG_QSPI_RESET                         BIT(0)

#define SYS_QSPI_RESET_CTL_W0_CFG_QSPI_RESET_MASK                    0x00000001

/* ################################################################################
 * # SysAxiSupResetCtl Definition
 */
#define SYS_AXI_SUP_RESET_CTL_W0_CFG_SWITCH_CORE_RESET               BIT(1)
#define SYS_AXI_SUP_RESET_CTL_W0_CFG_SWITCH_SUP_RESET                BIT(2)
#define SYS_AXI_SUP_RESET_CTL_W0_CFG_AXI_SUP_RESET                   BIT(0)

#define SYS_AXI_SUP_RESET_CTL_W0_CFG_SWITCH_CORE_RESET_MASK          0x00000002
#define SYS_AXI_SUP_RESET_CTL_W0_CFG_SWITCH_SUP_RESET_MASK           0x00000004
#define SYS_AXI_SUP_RESET_CTL_W0_CFG_AXI_SUP_RESET_MASK              0x00000001

/* ################################################################################
 * # SysGpioResetCtl Definition
 */
#define SYS_GPIO_RESET_CTL_W0_CFG_GPIO_RESET                         BIT(0)

#define SYS_GPIO_RESET_CTL_W0_CFG_GPIO_RESET_MASK                    0x00000001

/* ################################################################################
 * # SysI2CResetCtl Definition
 */
#define SYS_I2_C_RESET_CTL_W0_CFG_I2_C0_RESET                        BIT(0)
#define SYS_I2_C_RESET_CTL_W0_CFG_I2_C1_RESET                        BIT(1)

#define SYS_I2_C_RESET_CTL_W0_CFG_I2_C0_RESET_MASK                   0x00000001
#define SYS_I2_C_RESET_CTL_W0_CFG_I2_C1_RESET_MASK                   0x00000002

/* ################################################################################
 * # SysMdioSocResetCtl Definition
 */
#define SYS_MDIO_SOC_RESET_CTL_W0_CFG_MDIO_SOC_RESET                 BIT(0)

#define SYS_MDIO_SOC_RESET_CTL_W0_CFG_MDIO_SOC_RESET_MASK            0x00000001

/* ################################################################################
 * # SysTimerResetCtl Definition
 */
#define SYS_TIMER_RESET_CTL_W0_CFG_TIMER_RESET                       BIT(0)

#define SYS_TIMER_RESET_CTL_W0_CFG_TIMER_RESET_MASK                  0x00000001

/* ################################################################################
 * # SysUartResetCtl Definition
 */
#define SYS_UART_RESET_CTL_W0_CFG_UART1_RESET                        BIT(1)
#define SYS_UART_RESET_CTL_W0_CFG_UART2_RESET                        BIT(2)
#define SYS_UART_RESET_CTL_W0_CFG_UART0_RESET                        BIT(0)

#define SYS_UART_RESET_CTL_W0_CFG_UART1_RESET_MASK                   0x00000002
#define SYS_UART_RESET_CTL_W0_CFG_UART2_RESET_MASK                   0x00000004
#define SYS_UART_RESET_CTL_W0_CFG_UART0_RESET_MASK                   0x00000001

/* ################################################################################
 * # SysTraceResetCtl Definition
 */
#define SYS_TRACE_RESET_CTL_W0_CFG_TRACE_RESET                       BIT(0)

#define SYS_TRACE_RESET_CTL_W0_CFG_TRACE_RESET_MASK                  0x00000001

/* ################################################################################
 * # SysDbg0ResetEnCtl Definition
 */
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU_L2                  BIT(4)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU1_CORE               BIT(3)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CST                     BIT(7)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU0_COLD               BIT(0)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_WDT0                    BIT(9)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_WDT1                    BIT(10)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU1_COLD               BIT(1)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU0_CORE               BIT(2)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU_MEM                 BIT(8)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_NIC                     BIT(11)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_JTAG_POT                BIT(6)
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU_APB                 BIT(5)

#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU_L2_MASK             0x00000010
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU1_CORE_MASK          0x00000008
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CST_MASK                0x00000080
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU0_COLD_MASK          0x00000001
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_WDT0_MASK               0x00000200
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_WDT1_MASK               0x00000400
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU1_COLD_MASK          0x00000002
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU0_CORE_MASK          0x00000004
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU_MEM_MASK            0x00000100
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_NIC_MASK                0x00000800
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_JTAG_POT_MASK           0x00000040
#define SYS_DBG0_RESET_EN_CTL_W0_DBG0_RST_EN_CPU_APB_MASK            0x00000020

/* ################################################################################
 * # SysDbg1ResetEnCtl Definition
 */
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU1_CORE               BIT(3)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU_APB                 BIT(5)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_JTAG_POT                BIT(6)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU0_COLD               BIT(0)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU_MEM                 BIT(8)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU1_COLD               BIT(1)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU0_CORE               BIT(2)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CST                     BIT(7)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_WDT0                    BIT(9)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU_L2                  BIT(4)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_WDT1                    BIT(10)
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_NIC                     BIT(11)

#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU1_CORE_MASK          0x00000008
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU_APB_MASK            0x00000020
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_JTAG_POT_MASK           0x00000040
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU0_COLD_MASK          0x00000001
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU_MEM_MASK            0x00000100
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU1_COLD_MASK          0x00000002
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU0_CORE_MASK          0x00000004
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CST_MASK                0x00000080
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_WDT0_MASK               0x00000200
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_CPU_L2_MASK             0x00000010
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_WDT1_MASK               0x00000400
#define SYS_DBG1_RESET_EN_CTL_W0_DBG1_RST_EN_NIC_MASK                0x00000800

/* ################################################################################
 * # SysWarm0ResetEnCtl Definition
 */
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU1_COLD             BIT(1)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_WDT0                  BIT(9)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU0_COLD             BIT(0)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU_L2                BIT(4)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_JTAG_POT              BIT(6)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CST                   BIT(7)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU_APB               BIT(5)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU0_CORE             BIT(2)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU_MEM               BIT(8)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_WDT1                  BIT(10)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_NIC                   BIT(11)
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU1_CORE             BIT(3)

#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU1_COLD_MASK        0x00000002
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_WDT0_MASK             0x00000200
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU0_COLD_MASK        0x00000001
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU_L2_MASK           0x00000010
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_JTAG_POT_MASK         0x00000040
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CST_MASK              0x00000080
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU_APB_MASK          0x00000020
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU0_CORE_MASK        0x00000004
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU_MEM_MASK          0x00000100
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_WDT1_MASK             0x00000400
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_NIC_MASK              0x00000800
#define SYS_WARM0_RESET_EN_CTL_W0_WARM0_RST_EN_CPU1_CORE_MASK        0x00000008

/* ################################################################################
 * # SysWarm1ResetEnCtl Definition
 */
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU_MEM               BIT(8)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU1_COLD             BIT(1)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_WDT1                  BIT(10)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU_APB               BIT(5)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CST                   BIT(7)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_NIC                   BIT(11)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU0_COLD             BIT(0)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU1_CORE             BIT(3)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU_L2                BIT(4)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_WDT0                  BIT(9)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_JTAG_POT              BIT(6)
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU0_CORE             BIT(2)

#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU_MEM_MASK          0x00000100
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU1_COLD_MASK        0x00000002
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_WDT1_MASK             0x00000400
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU_APB_MASK          0x00000020
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CST_MASK              0x00000080
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_NIC_MASK              0x00000800
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU0_COLD_MASK        0x00000001
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU1_CORE_MASK        0x00000008
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU_L2_MASK           0x00000010
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_WDT0_MASK             0x00000200
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_JTAG_POT_MASK         0x00000040
#define SYS_WARM1_RESET_EN_CTL_W0_WARM1_RST_EN_CPU0_CORE_MASK        0x00000004

/* ################################################################################
 * # SysWdt0ResetEnCtl Definition
 */
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU_L2                  BIT(4)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU1_COLD               BIT(1)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU_APB                 BIT(5)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_WDT1                    BIT(10)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CST                     BIT(7)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU0_CORE               BIT(2)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_WDT0                    BIT(9)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU_MEM                 BIT(8)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_JTAG_POT                BIT(6)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU1_CORE               BIT(3)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU0_COLD               BIT(0)
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_NIC                     BIT(11)

#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU_L2_MASK             0x00000010
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU1_COLD_MASK          0x00000002
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU_APB_MASK            0x00000020
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_WDT1_MASK               0x00000400
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CST_MASK                0x00000080
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU0_CORE_MASK          0x00000004
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_WDT0_MASK               0x00000200
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU_MEM_MASK            0x00000100
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_JTAG_POT_MASK           0x00000040
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU1_CORE_MASK          0x00000008
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_CPU0_COLD_MASK          0x00000001
#define SYS_WDT0_RESET_EN_CTL_W0_WDT0_RST_EN_NIC_MASK                0x00000800

/* ################################################################################
 * # SysWdt1ResetEnCtl Definition
 */
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU_L2                  BIT(4)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_WDT0                    BIT(9)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_WDT1                    BIT(10)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU1_CORE               BIT(3)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU_APB                 BIT(5)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CST                     BIT(7)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU0_COLD               BIT(0)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU_MEM                 BIT(8)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU1_COLD               BIT(1)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_JTAG_POT                BIT(6)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU0_CORE               BIT(2)
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_NIC                     BIT(11)

#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU_L2_MASK             0x00000010
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_WDT0_MASK               0x00000200
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_WDT1_MASK               0x00000400
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU1_CORE_MASK          0x00000008
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU_APB_MASK            0x00000020
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CST_MASK                0x00000080
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU0_COLD_MASK          0x00000001
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU_MEM_MASK            0x00000100
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU1_COLD_MASK          0x00000002
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_JTAG_POT_MASK           0x00000040
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_CPU0_CORE_MASK          0x00000004
#define SYS_WDT1_RESET_EN_CTL_W0_WDT1_RST_EN_NIC_MASK                0x00000800

/* ################################################################################
 * # SysCtlSysRev Definition
 */
#define SYS_CTL_RESERVED_W0_RESERVED                             	 BIT(4)
#define SYS_CTL_RESERVED_W0_SYS_REV                           		 BIT(0)

#define SYS_CTL_RESERVED_W0_RESERVED_MASK                            0xfffffff0
#define SYS_CTL_RESERVED_W0_SYS_REV_MASK                             0x0000000f

/* ################################################################################
 * # SysEnClkCfg Definition
 */
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_MSH                             BIT(2)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_CPU_DMA                         BIT(5)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_CPU_MEM                         BIT(8)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_CPU_MAC                         BIT(7)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_CST                             BIT(4)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_TZC                             BIT(3)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_MISC                            BIT(12)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_DDR                             BIT(0)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_SSP                             BIT(10)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_GIC                             BIT(6)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_USB                             BIT(1)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_AXI_SUP                         BIT(9)
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_QSPI                            BIT(11)

#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_MSH_MASK                        0x00000004
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_CPU_DMA_MASK                    0x00000020
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_CPU_MEM_MASK                    0x00000100
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_CPU_MAC_MASK                    0x00000080
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_CST_MASK                        0x00000010
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_TZC_MASK                        0x00000008
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_MISC_MASK                       0x00001000
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_DDR_MASK                        0x00000001
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_SSP_MASK                        0x00000400
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_GIC_MASK                        0x00000040
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_USB_MASK                        0x00000002
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_AXI_SUP_MASK                    0x00000200
#define SYS_EN_CLK_CFG_W0_CFG_EN_CLK_QSPI_MASK                       0x00000800

/* ################################################################################
 * # SysPllSocCfg0 Definition
 */
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_POST_DIV                         BIT(12)
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_MULT_INT                         BIT(20)
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_INT_FBK                          BIT(2)
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_DCO_BYPASS                       BIT(1)
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_PRE_DIV                          BIT(4)
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_RESET                            BIT(0)
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_PLL_PWD                          BIT(3)

#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_POST_DIV_MASK                    0x0003f000
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_MULT_INT_MASK                    0x0ff00000
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_INT_FBK_MASK                     0x00000004
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_DCO_BYPASS_MASK                  0x00000002
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_PRE_DIV_MASK                     0x000001f0
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_RESET_MASK                       0x00000001
#define SYS_PLL_SOC_CFG0_W0_PLL_SOC_PLL_PWD_MASK                     0x00000008

/* ################################################################################
 * # SysPllSocCfg1 Definition
 */
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_BYPASS                           BIT(24)
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_SLOCK                            BIT(16)
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_SGAIN                            BIT(20)
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_SIP                              BIT(0)
#define SYS_PLL_SOC_CFG1_W0_MON_PLL_SOC_LOCK                         BIT(28)
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_SIC                              BIT(8)

#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_BYPASS_MASK                      0x01000000
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_SLOCK_MASK                       0x00010000
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_SGAIN_MASK                       0x00700000
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_SIP_MASK                         0x0000001f
#define SYS_PLL_SOC_CFG1_W0_MON_PLL_SOC_LOCK_MASK                    0x10000000
#define SYS_PLL_SOC_CFG1_W0_PLL_SOC_SIC_MASK                         0x00001f00

/* ################################################################################
 * # SysPllDdrCfg0 Definition
 */
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_DCO_BYPASS                       BIT(1)
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_PRE_DIV                          BIT(4)
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_RESET                            BIT(0)
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_INT_FBK                          BIT(2)
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_PLL_PWD                          BIT(3)
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_MULT_INT                         BIT(20)
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_POST_DIV                         BIT(12)

#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_DCO_BYPASS_MASK                  0x00000002
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_PRE_DIV_MASK                     0x000001f0
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_RESET_MASK                       0x00000001
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_INT_FBK_MASK                     0x00000004
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_PLL_PWD_MASK                     0x00000008
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_MULT_INT_MASK                    0x0ff00000
#define SYS_PLL_DDR_CFG0_W0_PLL_DDR_POST_DIV_MASK                    0x0003f000

/* ################################################################################
 * # SysPllDdrCfg1 Definition
 */
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_SLOCK                            BIT(16)
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_BYPASS                           BIT(24)
#define SYS_PLL_DDR_CFG1_W0_MON_PLL_DDR_LOCK                         BIT(28)
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_SIC                              BIT(8)
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_SIP                              BIT(0)
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_SGAIN                            BIT(20)

#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_SLOCK_MASK                       0x00010000
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_BYPASS_MASK                      0x01000000
#define SYS_PLL_DDR_CFG1_W0_MON_PLL_DDR_LOCK_MASK                    0x10000000
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_SIC_MASK                         0x00001f00
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_SIP_MASK                         0x0000001f
#define SYS_PLL_DDR_CFG1_W0_PLL_DDR_SGAIN_MASK                       0x00700000

/* ################################################################################
 * # SysClkSelCfg Definition
 */
#define SYS_CLK_SEL_CFG_W0_RELEASE_DIV_CLK                           BIT(1)
#define SYS_CLK_SEL_CFG_W0_SUP_CLOCK_SEL                             BIT(0)

#define SYS_CLK_SEL_CFG_W0_RELEASE_DIV_CLK_MASK                      0x00000002
#define SYS_CLK_SEL_CFG_W0_SUP_CLOCK_SEL_MASK                        0x00000001

/* ################################################################################
 * # SysClkDivCfg Definition
 */
#define SYS_CLK_DIV_CFG_W0_CFG_DIV_AHB_CNT                           BIT(8)
#define SYS_CLK_DIV_CFG_W0_CFG_DIV_TRACE_CNT                         BIT(16)
#define SYS_CLK_DIV_CFG_W0_CFG_DIV_CST_CNT                           BIT(0)
#define SYS_CLK_DIV_CFG_W0_CFG_DIV_MSH_CNT                           BIT(24)

#define SYS_CLK_DIV_CFG_W0_CFG_DIV_AHB_CNT_MASK                      0x0000ff00
#define SYS_CLK_DIV_CFG_W0_CFG_DIV_TRACE_CNT_MASK                    0x00ff0000
#define SYS_CLK_DIV_CFG_W0_CFG_DIV_CST_CNT_MASK                      0x000000ff
#define SYS_CLK_DIV_CFG_W0_CFG_DIV_MSH_CNT_MASK                      0xff000000

/* ################################################################################
 * # SysClkPeriCfg Definition
 */
#define SYS_CLK_PERI_CFG_W0_CFG_DIV_USB_PHY_CNT                      BIT(16)
#define SYS_CLK_PERI_CFG_W0_CFG_DIV_SSP_CNT                          BIT(8)
#define SYS_CLK_PERI_CFG_W0_CFG_DIV_MSH_REF_CNT                      BIT(0)
#define SYS_CLK_PERI_CFG_W0_CFG_DIV_UART_CNT                         BIT(24)
#define SYS_CLK_PERI_CFG_W1_CFG_DIV_UART_CHG_EN                      BIT(15)
#define SYS_CLK_PERI_CFG_W1_CFG_DIV_MSH_TM_CNT                       BIT(0)
#define SYS_CLK_PERI_CFG_W1_CFG_DIV_MDIO_SOC_CNT                     BIT(16)

#define SYS_CLK_PERI_CFG_W0_CFG_DIV_USB_PHY_CNT_MASK                 0x00ff0000
#define SYS_CLK_PERI_CFG_W0_CFG_DIV_SSP_CNT_MASK                     0x0000ff00
#define SYS_CLK_PERI_CFG_W0_CFG_DIV_MSH_REF_CNT_MASK                 0x000000ff
#define SYS_CLK_PERI_CFG_W0_CFG_DIV_UART_CNT_MASK                    0xff000000
#define SYS_CLK_PERI_CFG_W1_CFG_DIV_UART_CHG_EN_MASK                 0x00008000
#define SYS_CLK_PERI_CFG_W1_CFG_DIV_MSH_TM_CNT_MASK                  0x000007ff
#define SYS_CLK_PERI_CFG_W1_CFG_DIV_MDIO_SOC_CNT_MASK                0x07ff0000

/* ################################################################################
 * # SysDbgCreditCtl Definition
 */
#define SYS_DBG_CREDIT_CTL_W0_DBG_CREDIT_CNT                         BIT(8)
#define SYS_DBG_CREDIT_CTL_W0_CFG_DBG_CREDIT_CNT                     BIT(0)
#define SYS_DBG_CREDIT_CTL_W0_DBG_CREDIT_OK                          BIT(12)
#define SYS_DBG_CREDIT_CTL_W0_CFG_DBG_CREDIT_THRD                    BIT(4)

#define SYS_DBG_CREDIT_CTL_W0_DBG_CREDIT_CNT_MASK                    0x00000700
#define SYS_DBG_CREDIT_CTL_W0_CFG_DBG_CREDIT_CNT_MASK                0x00000007
#define SYS_DBG_CREDIT_CTL_W0_DBG_CREDIT_OK_MASK                     0x00001000
#define SYS_DBG_CREDIT_CTL_W0_CFG_DBG_CREDIT_THRD_MASK               0x00000070

/* ################################################################################
 * # SysI2CMultiCtl Definition
 */
#define SYS_I2_C_MULTI_CTL_W0_CFG_I2_C_SLAVE_EN                      BIT(0)

#define SYS_I2_C_MULTI_CTL_W0_CFG_I2_C_SLAVE_EN_MASK                 0x00000001

/* ################################################################################
 * # BootStrapPin Definition
 */
#define BOOT_STRAP_PIN_W0_BOOT_STRAP_LOG                             BIT(0)
#define BOOT_STRAP_PIN_W0_CPU_SPEED_LOG                              BIT(4)

#define BOOT_STRAP_PIN_W0_BOOT_STRAP_LOG_MASK                        0x00000007
#define BOOT_STRAP_PIN_W0_CPU_SPEED_LOG_MASK                         0x00000010

/* ################################################################################
 * # SysCntValue Definition
 */
#define SYS_CNT_VALUE_W0_SYS_CNT_VALUE_READ_31_0                     BIT(0)
#define SYS_CNT_VALUE_W1_SYS_CNT_VALUE_READ_63_32                    BIT(0)

#define SYS_CNT_VALUE_W0_SYS_CNT_VALUE_READ_31_0_MASK                0x00000001
#define SYS_CNT_VALUE_W1_SYS_CNT_VALUE_READ_63_32_MASK               0x00000001

/* ################################################################################
 * # SysCntLog Definition
 */
#define SYS_CNT_LOG_W0_SYS_CNT_HALT_READ                             BIT(1)
#define SYS_CNT_LOG_W0_SYS_CNT_OVER_READ                             BIT(2)
#define SYS_CNT_LOG_W0_SYS_CNT_DIV_READ                              BIT(16)
#define SYS_CNT_LOG_W0_SYS_CNT_EN_READ                               BIT(0)

#define SYS_CNT_LOG_W0_SYS_CNT_HALT_READ_MASK                        0x00000002
#define SYS_CNT_LOG_W0_SYS_CNT_OVER_READ_MASK                        0x00000004
#define SYS_CNT_LOG_W0_SYS_CNT_DIV_READ_MASK                         0x03ff0000
#define SYS_CNT_LOG_W0_SYS_CNT_EN_READ_MASK                          0x00000001

/* ################################################################################
 * # SysCoreGicAddrBase Definition
 */
#define SYS_CORE_GIC_ADDR_BASE_W0_CFG_GIC_REG_BASE                   BIT(0)

#define SYS_CORE_GIC_ADDR_BASE_W0_CFG_GIC_REG_BASE_MASK              0x00ffffff

/* ################################################################################
 * # SysCorePmCfg Definition
 */
#define SYS_CORE_PM_CFG_W0_LOG_L2_Q_ACCEPTN                          BIT(1)
#define SYS_CORE_PM_CFG_W0_LOG_DBG_NO_PWR_DWN                        BIT(4)
#define SYS_CORE_PM_CFG_W0_LOG_L2_Q_DENY                             BIT(2)
#define SYS_CORE_PM_CFG_W0_CFG_L2_Q_REQN                             BIT(8)
#define SYS_CORE_PM_CFG_W0_LOG_L2_Q_ACTIVE                           BIT(0)

#define SYS_CORE_PM_CFG_W0_LOG_L2_Q_ACCEPTN_MASK                     0x00000002
#define SYS_CORE_PM_CFG_W0_LOG_DBG_NO_PWR_DWN_MASK                   0x00000030
#define SYS_CORE_PM_CFG_W0_LOG_L2_Q_DENY_MASK                        0x00000004
#define SYS_CORE_PM_CFG_W0_CFG_L2_Q_REQN_MASK                        0x00000100
#define SYS_CORE_PM_CFG_W0_LOG_L2_Q_ACTIVE_MASK                      0x00000001

/* ################################################################################
 * # SysCoreStatus Definition
 */
#define SYS_CORE_STATUS_W0_CORE_STANDBY_WFI_L2                       BIT(7)
#define SYS_CORE_STATUS_W0_JTAG_NSW_LOG                              BIT(8)
#define SYS_CORE_STATUS_W0_CFG_CORE1_EVENT_CLR                       BIT(17)
#define SYS_CORE_STATUS_W0_CORE_SMP_EN                               BIT(4)
#define SYS_CORE_STATUS_W0_JTAG_TOP_LOG                              BIT(9)
#define SYS_CORE_STATUS_W0_CORE_STANDBY_WFE                          BIT(2)
#define SYS_CORE_STATUS_W0_CORE_L2_FLUSH_DONE                        BIT(6)
#define SYS_CORE_STATUS_W0_CORE_STANDBY_WFI                          BIT(0)
#define SYS_CORE_STATUS_W0_CFG_CORE0_EVENT_CLR                       BIT(16)
#define SYS_CORE_STATUS_W0_CFG_CORE0_EVENT_RAW                       BIT(18)
#define SYS_CORE_STATUS_W0_CFG_CORE1_EVENT_RAW                       BIT(19)

#define SYS_CORE_STATUS_W0_CORE_STANDBY_WFI_L2_MASK                  0x00000080
#define SYS_CORE_STATUS_W0_JTAG_NSW_LOG_MASK                         0x00000100
#define SYS_CORE_STATUS_W0_CFG_CORE1_EVENT_CLR_MASK                  0x00020000
#define SYS_CORE_STATUS_W0_CORE_SMP_EN_MASK                          0x00000030
#define SYS_CORE_STATUS_W0_JTAG_TOP_LOG_MASK                         0x00000200
#define SYS_CORE_STATUS_W0_CORE_STANDBY_WFE_MASK                     0x0000000c
#define SYS_CORE_STATUS_W0_CORE_L2_FLUSH_DONE_MASK                   0x00000040
#define SYS_CORE_STATUS_W0_CORE_STANDBY_WFI_MASK                     0x00000003
#define SYS_CORE_STATUS_W0_CFG_CORE0_EVENT_CLR_MASK                  0x00010000
#define SYS_CORE_STATUS_W0_CFG_CORE0_EVENT_RAW_MASK                  0x00040000
#define SYS_CORE_STATUS_W0_CFG_CORE1_EVENT_RAW_MASK                  0x00080000

/* ################################################################################
 * # SysCorePmuEvent0 Definition
 */
#define SYS_CORE_PMU_EVENT0_W0_CORE0_PMU_EVENT                       BIT(0)

#define SYS_CORE_PMU_EVENT0_W0_CORE0_PMU_EVENT_MASK                  0x3fffffff

/* ################################################################################
 * # SysCorePmuEvent1 Definition
 */
#define SYS_CORE_PMU_EVENT1_W0_CORE1_PMU_EVENT                       BIT(0)

#define SYS_CORE_PMU_EVENT1_W0_CORE1_PMU_EVENT_MASK                  0x3fffffff

/* ################################################################################
 * # SysRstVecBar0 Definition
 */
#define SYS_RST_VEC_BAR0_W0_CFG_RV_BAR_ADDR0_31_0                    BIT(0)
#define SYS_RST_VEC_BAR0_W1_CFG_RV_BAR_ADDR0_39_32                   BIT(0)

#define SYS_RST_VEC_BAR0_W0_CFG_RV_BAR_ADDR0_31_0_MASK               0xffffffff
#define SYS_RST_VEC_BAR0_W1_CFG_RV_BAR_ADDR0_39_32_MASK              0x000000ff

/* ################################################################################
 * # SysRstVecBar1 Definition
 */
#define SYS_RST_VEC_BAR1_W0_CFG_RV_BAR_ADDR1_31_0                    BIT(0)
#define SYS_RST_VEC_BAR1_W1_CFG_RV_BAR_ADDR1_39_32                   BIT(0)

#define SYS_RST_VEC_BAR1_W0_CFG_RV_BAR_ADDR1_31_0_MASK               0xffffffff
#define SYS_RST_VEC_BAR1_W1_CFG_RV_BAR_ADDR1_39_32_MASK              0x000000ff

/* ################################################################################
 * # SysGicCfg Definition
 */
#define SYS_GIC_CFG_W0_CFG_GIC_LEGACY_IRQ_BAR                        BIT(4)
#define SYS_GIC_CFG_W0_CFG_GIC_WA_USER                               BIT(9)
#define SYS_GIC_CFG_W0_CFG_GIC_LEGACY_FIQ_BAR                        BIT(0)
#define SYS_GIC_CFG_W0_CFG_GIC_RA_USER                               BIT(8)

#define SYS_GIC_CFG_W0_CFG_GIC_LEGACY_IRQ_BAR_MASK                   0x00000030
#define SYS_GIC_CFG_W0_CFG_GIC_WA_USER_MASK                          0x00000200
#define SYS_GIC_CFG_W0_CFG_GIC_LEGACY_FIQ_BAR_MASK                   0x00000003
#define SYS_GIC_CFG_W0_CFG_GIC_RA_USER_MASK                          0x00000100

/* ################################################################################
 * # SysGicStatus Definition
 */
#define SYS_GIC_STATUS_W0_GIC_FIQ_OUT_LOG                            BIT(0)
#define SYS_GIC_STATUS_W0_GIC_IRQ_OUT_LOG                            BIT(2)

#define SYS_GIC_STATUS_W0_GIC_FIQ_OUT_LOG_MASK                       0x00000003
#define SYS_GIC_STATUS_W0_GIC_IRQ_OUT_LOG_MASK                       0x0000000c

/* ################################################################################
 * # SysMemCtl Definition
 */
#define SYS_MEM_CTL_W0_CFG_RAM_MUX_EN                                BIT(0)
#define SYS_MEM_CTL_W0_CFG_SYS_REMAP_EN                              BIT(2)
#define SYS_MEM_CTL_W0_CFG_SYS_DBG_EN                                BIT(4)
#define SYS_MEM_CTL_W0_CFG_RAM_REMAP                                 BIT(1)

#define SYS_MEM_CTL_W0_CFG_RAM_MUX_EN_MASK                           0x00000001
#define SYS_MEM_CTL_W0_CFG_SYS_REMAP_EN_MASK                         0x00000004
#define SYS_MEM_CTL_W0_CFG_SYS_DBG_EN_MASK                           0x00000010
#define SYS_MEM_CTL_W0_CFG_RAM_REMAP_MASK                            0x00000002

/* ################################################################################
 * # SysDmaMapCfg Definition
 */
#define SYS_DMA_MAP_CFG_W0_CFG_CPU_DMA_WR_MAP_LOW                    BIT(0)
#define SYS_DMA_MAP_CFG_W0_CFG_CPU_DMA_WR_MAP_HIGH                   BIT(16)
#define SYS_DMA_MAP_CFG_W0_CFG_CPU_DMA_RD_MAP_HIGH                   BIT(24)
#define SYS_DMA_MAP_CFG_W0_CFG_CPU_DMA_RD_MAP_LOW                    BIT(8)

#define SYS_DMA_MAP_CFG_W0_CFG_CPU_DMA_WR_MAP_LOW_MASK               0x0000000f
#define SYS_DMA_MAP_CFG_W0_CFG_CPU_DMA_WR_MAP_HIGH_MASK              0x000f0000
#define SYS_DMA_MAP_CFG_W0_CFG_CPU_DMA_RD_MAP_HIGH_MASK              0x0f000000
#define SYS_DMA_MAP_CFG_W0_CFG_CPU_DMA_RD_MAP_LOW_MASK               0x00000f00

/* ################################################################################
 * # SysDmaAbortCfg Definition
 */
#define SYS_DMA_ABORT_CFG_W0_DMA_ABORT_LEVEL_EN                      BIT(4)
#define SYS_DMA_ABORT_CFG_W0_DMA_ABORT_STATUS                        BIT(0)

#define SYS_DMA_ABORT_CFG_W0_DMA_ABORT_LEVEL_EN_MASK                 0x00000010
#define SYS_DMA_ABORT_CFG_W0_DMA_ABORT_STATUS_MASK                   0x00000001

/* ################################################################################
 * # SysCstCfg Definition
 */
#define SYS_CST_CFG_W0_CFG_CST_PIU_TP_CTL                            BIT(4)
#define SYS_CST_CFG_W0_CFG_CST_PIU_TP_MAX_DATA_SIZE                  BIT(8)
#define SYS_CST_CFG_W0_CFG_CST_INSTANCE_ID                           BIT(0)
#define SYS_CST_CFG_W0_CFG_CST_DEVICE_EN                             BIT(5)

#define SYS_CST_CFG_W0_CFG_CST_PIU_TP_CTL_MASK                       0x00000010
#define SYS_CST_CFG_W0_CFG_CST_PIU_TP_MAX_DATA_SIZE_MASK             0x00001f00
#define SYS_CST_CFG_W0_CFG_CST_INSTANCE_ID_MASK                      0x0000000f
#define SYS_CST_CFG_W0_CFG_CST_DEVICE_EN_MASK                        0x00000020

/* ################################################################################
 * # SysCstTargetIdCfg Definition
 */
#define SYS_CST_TARGET_ID_CFG_W0_CFG_CST_TARGET_ID                   BIT(0)

#define SYS_CST_TARGET_ID_CFG_W0_CFG_CST_TARGET_ID_MASK              0xffffffff

/* ################################################################################
 * # SysCstMemMapIdCfg Definition
 */
#define SYS_CST_MEM_MAP_ID_CFG_W0_CFG_CST_MEM_MAP_TARGET_ID          BIT(0)

#define SYS_CST_MEM_MAP_ID_CFG_W0_CFG_CST_MEM_MAP_TARGET_ID_MASK     0xffffffff

/* ################################################################################
 * # SysQspiBootCfg0 Definition
 */
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_EN_CPHA                      BIT(4)
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_HOLD                         BIT(6)
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_WP                           BIT(7)
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_CHIP_SEL                     BIT(8)
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_RX_CODE                      BIT(12)
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_EN                           BIT(0)
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_XIP_EN                            BIT(1)
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_EN_CPOL                      BIT(5)
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_CLK_DIV                      BIT(16)

#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_EN_CPHA_MASK                 0x00000010
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_HOLD_MASK                    0x00000040
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_WP_MASK                      0x00000080
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_CHIP_SEL_MASK                0x00000700
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_RX_CODE_MASK                 0x00007000
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_EN_MASK                      0x00000001
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_XIP_EN_MASK                       0x00000002
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_EN_CPOL_MASK                 0x00000020
#define SYS_QSPI_BOOT_CFG0_W0_QSPI_BOOT_CLK_DIV_MASK                 0x00ff0000

/* ################################################################################
 * # SysQspiBootCfg1 Definition
 */
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_CMD_CODE                     BIT(24)
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_CMD_MODE                     BIT(12)
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_CMD_CYCLE                    BIT(16)
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_ADDR_MODE                    BIT(8)
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_ADDR_CYCLE                   BIT(0)

#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_CMD_CODE_MASK                0xff000000
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_CMD_MODE_MASK                0x00007000
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_CMD_CYCLE_MASK               0x001f0000
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_ADDR_MODE_MASK               0x00000700
#define SYS_QSPI_BOOT_CFG1_W0_QSPI_BOOT_ADDR_CYCLE_MASK              0x0000003f

/* ################################################################################
 * # SysQspiBootCfg2 Definition
 */
#define SYS_QSPI_BOOT_CFG2_W0_QSPI_BOOT_DUMMY_MODE                   BIT(8)
#define SYS_QSPI_BOOT_CFG2_W0_QSPI_BOOT_DUMMY_CYCLE                  BIT(0)

#define SYS_QSPI_BOOT_CFG2_W0_QSPI_BOOT_DUMMY_MODE_MASK              0x00000700
#define SYS_QSPI_BOOT_CFG2_W0_QSPI_BOOT_DUMMY_CYCLE_MASK             0x0000003f

/* ################################################################################
 * # SysQspiBootCfg3 Definition
 */
#define SYS_QSPI_BOOT_CFG3_W0_QSPI_BOOT_DUMMY_CODE                   BIT(0)

#define SYS_QSPI_BOOT_CFG3_W0_QSPI_BOOT_DUMMY_CODE_MASK              0xffffffff

/* ################################################################################
 * # SysDdrCfg Definition
 */
#define SYS_DDR_CFG_W0_DDR_RESET_CK_E_LATCH_BAR                      BIT(0)
#define SYS_DDR_CFG_W0_DDR_FORCE_STA_CK_STP1                         BIT(3)
#define SYS_DDR_CFG_W0_DDR_PHY_IDDQ                                  BIT(4)
#define SYS_DDR_CFG_W0_DDR_VDD_ON                                    BIT(2)
#define SYS_DDR_CFG_W0_DDR_PWR_OFF_PHY                               BIT(1)
#define SYS_DDR_CFG_W0_DDR_INTF2_ADDR_TRANS_EN                       BIT(5)
#define SYS_DDR_CFG_W0_DDR_INTF3_ADDR_TRANS_EN                     	 BIT(6)

#define SYS_DDR_CFG_W0_DDR_RESET_CK_E_LATCH_BAR_MASK                 0x00000001
#define SYS_DDR_CFG_W0_DDR_FORCE_STA_CK_STP1_MASK                    0x00000008
#define SYS_DDR_CFG_W0_DDR_PHY_IDDQ_MASK                             0x00000010
#define SYS_DDR_CFG_W0_DDR_VDD_ON_MASK                               0x00000004
#define SYS_DDR_CFG_W0_DDR_PWR_OFF_PHY_MASK                          0x00000002
#define SYS_DDR_CFG_W0_DDR_INTF2_ADDR_TRANS_EN_MASK                  0x00000020
#define SYS_DDR_CFG_W0_DDR_INTF3_ADDR_TRANS_EN_MASK                  0x00000040

/* ################################################################################
 * # SysSpiSelCfg Definition
 */
#define SYS_SPI_SEL_CFG_W0_SSP_SLAVE_SEL                             BIT(0)
#define SYS_SPI_SEL_CFG_W0_SSP_CS_CFG_CTL                            BIT(4)

#define SYS_SPI_SEL_CFG_W0_SSP_SLAVE_SEL_MASK                        0x00000003
#define SYS_SPI_SEL_CFG_W0_SSP_CS_CFG_CTL_MASK                       0x000000f0

/* ################################################################################
 * # SysMdioSocCfg Definition
 */
#define SYS_MDIO_SOC_CFG_W0_CFG_EN_CLK_MDIO_SOC                      BIT(4)
#define SYS_MDIO_SOC_CFG_W0_CFG_INV_MDIO_SOC                         BIT(0)
#define SYS_MDIO_SOC_CFG_W0_CFG_EN_CLK_MDIO_SOC_REG                  BIT(5)

#define SYS_MDIO_SOC_CFG_W0_CFG_EN_CLK_MDIO_SOC_MASK                 0x00000010
#define SYS_MDIO_SOC_CFG_W0_CFG_INV_MDIO_SOC_MASK                    0x00000001
#define SYS_MDIO_SOC_CFG_W0_CFG_EN_CLK_MDIO_SOC_REG_MASK             0x00000020

/* ################################################################################
 * # SysWdt0Cnt Definition
 */
#define SYS_WDT0_CNT_W0_CFG_WDT0_DIV_CNT                             BIT(0)

#define SYS_WDT0_CNT_W0_CFG_WDT0_DIV_CNT_MASK                        0xffffffff

/* ################################################################################
 * # SysWdt0Rev Definition
 */
#define SYS_WDT0_REV_W0_CFG_WDT0_ECO_REV                             BIT(0)

#define SYS_WDT0_REV_W0_CFG_WDT0_ECO_REV_MASK                        0x0000000f

/* ################################################################################
 * # SysWdt1Cnt Definition
 */
#define SYS_WDT1_CNT_W0_CFG_WDT1_DIV_CNT                             BIT(0)

#define SYS_WDT1_CNT_W0_CFG_WDT1_DIV_CNT_MASK                        0xffffffff

/* ################################################################################
 * # SysWdt1Rev Definition
 */
#define SYS_WDT1_REV_W0_CFG_WDT1_ECO_REV                             BIT(0)

#define SYS_WDT1_REV_W0_CFG_WDT1_ECO_REV_MASK                        0x0000000f

/* ################################################################################
 * # SysMshCfg Definition
 */
#define SYS_MSH_CFG_W0_CFG_MSH_CARD_DETECT_IN_EN                     BIT(4)
#define SYS_MSH_CFG_W0_CFG_MSH_CARD_DETECT                           BIT(5)
#define SYS_MSH_CFG_W0_MSH_INTF_C_CLK_TX_DELAY                       BIT(16)
#define SYS_MSH_CFG_W0_MSH_INTF_C_CLK_TX_PHASE_SEL                   BIT(24)
#define SYS_MSH_CFG_W0_CFG_MSH_CARD_WRITE_PROT                       BIT(7)
#define SYS_MSH_CFG_W0_CFG_RESET_BAR_MSH_SEL                         BIT(12)
#define SYS_MSH_CFG_W0_MSH_INTF_TX_DLL_MASTER_BYPASS                 BIT(2)
#define SYS_MSH_CFG_W0_MSH_INTF_RX_DLL_MASTER_BYPASS                 BIT(1)
#define SYS_MSH_CFG_W0_MSH_INTF_C_CLK_TX_SEL                         BIT(25)
#define SYS_MSH_CFG_W0_MSH_INTF_AT_DLL_MASTER_BYPASS                 BIT(0)
#define SYS_MSH_CFG_W0_CFG_MSH_CARD_WRITE_PROT_IN_EN                 BIT(6)

#define SYS_MSH_CFG_W0_CFG_MSH_CARD_DETECT_IN_EN_MASK                0x00000010
#define SYS_MSH_CFG_W0_CFG_MSH_CARD_DETECT_MASK                      0x00000020
#define SYS_MSH_CFG_W0_MSH_INTF_C_CLK_TX_DELAY_MASK                  0x00ff0000
#define SYS_MSH_CFG_W0_MSH_INTF_C_CLK_TX_PHASE_SEL_MASK              0x01000000
#define SYS_MSH_CFG_W0_CFG_MSH_CARD_WRITE_PROT_MASK                  0x00000080
#define SYS_MSH_CFG_W0_CFG_RESET_BAR_MSH_SEL_MASK                    0x00001000
#define SYS_MSH_CFG_W0_MSH_INTF_TX_DLL_MASTER_BYPASS_MASK            0x00000004
#define SYS_MSH_CFG_W0_MSH_INTF_RX_DLL_MASTER_BYPASS_MASK            0x00000002
#define SYS_MSH_CFG_W0_MSH_INTF_C_CLK_TX_SEL_MASK                    0x02000000
#define SYS_MSH_CFG_W0_MSH_INTF_AT_DLL_MASTER_BYPASS_MASK            0x00000001
#define SYS_MSH_CFG_W0_CFG_MSH_CARD_WRITE_PROT_IN_EN_MASK            0x00000040

/* ################################################################################
 * # SysMshStatus Definition
 */
#define SYS_MSH_STATUS_W0_MON_MSH_C_RESET_DONE                       BIT(2)
#define SYS_MSH_STATUS_W0_MON_MSH_SD_CLK_LOCK                        BIT(6)
#define SYS_MSH_STATUS_W0_MON_MSH_SD_DATXFER_WIDTH                   BIT(0)
#define SYS_MSH_STATUS_W0_MON_MSH_AT_DLL_LOCK                        BIT(4)
#define SYS_MSH_STATUS_W0_MON_MSH_RX_DLL_LOCK                        BIT(5)

#define SYS_MSH_STATUS_W0_MON_MSH_C_RESET_DONE_MASK                  0x00000004
#define SYS_MSH_STATUS_W0_MON_MSH_SD_CLK_LOCK_MASK                   0x00000040
#define SYS_MSH_STATUS_W0_MON_MSH_SD_DATXFER_WIDTH_MASK              0x00000003
#define SYS_MSH_STATUS_W0_MON_MSH_AT_DLL_LOCK_MASK                   0x00000010
#define SYS_MSH_STATUS_W0_MON_MSH_RX_DLL_LOCK_MASK                   0x00000020

/* ################################################################################
 * # SysUsbCfg0 Definition
 */
#define SYS_USB_CFG0_W0_USB_PHY_TX_PRE_EMP_AMP_TUNE                  BIT(2)
#define SYS_USB_CFG0_W0_USB_PHY_FSEL                                 BIT(8)
#define SYS_USB_CFG0_W0_USB_PHY_COMMON_ONN                           BIT(0)
#define SYS_USB_CFG0_W0_USB_PHY_PLL_B_TUNE                           BIT(1)
#define SYS_USB_CFG0_W0_USB_PHY_COMP_DIS_TUNE                        BIT(4)
#define SYS_USB_CFG0_W0_USB_PHY_TX_RISE_TUNE                         BIT(20)
#define SYS_USB_CFG0_W0_USB_PHY_TX_HSXV_TUNE                         BIT(28)
#define SYS_USB_CFG0_W0_USB_PHY_VA_TEST_EN_B                         BIT(22)
#define SYS_USB_CFG0_W0_USB_PHY_TX_VREF_TUNE                         BIT(24)
#define SYS_USB_CFG0_W0_USB_PHY_TX_RES_TUNE                          BIT(30)
#define SYS_USB_CFG0_W0_USB_PHY_TX_FS_LS_TUNE                        BIT(16)
#define SYS_USB_CFG0_W0_USB_PHY_VREG_BYPASS                          BIT(11)
#define SYS_USB_CFG0_W0_USB_PHY_TX_PRE_EMP_PULSE_TUNE                BIT(7)
#define SYS_USB_CFG0_W0_USB_PHY_SQ_RX_TUNE                           BIT(12)

#define SYS_USB_CFG0_W0_USB_PHY_TX_PRE_EMP_AMP_TUNE_MASK             0x0000000c
#define SYS_USB_CFG0_W0_USB_PHY_FSEL_MASK                            0x00000700
#define SYS_USB_CFG0_W0_USB_PHY_COMMON_ONN_MASK                      0x00000001
#define SYS_USB_CFG0_W0_USB_PHY_PLL_B_TUNE_MASK                      0x00000002
#define SYS_USB_CFG0_W0_USB_PHY_COMP_DIS_TUNE_MASK                   0x00000070
#define SYS_USB_CFG0_W0_USB_PHY_TX_RISE_TUNE_MASK                    0x00300000
#define SYS_USB_CFG0_W0_USB_PHY_TX_HSXV_TUNE_MASK                    0x30000000
#define SYS_USB_CFG0_W0_USB_PHY_VA_TEST_EN_B_MASK                    0x00c00000
#define SYS_USB_CFG0_W0_USB_PHY_TX_VREF_TUNE_MASK                    0x0f000000
#define SYS_USB_CFG0_W0_USB_PHY_TX_RES_TUNE_MASK                     0xc0000000
#define SYS_USB_CFG0_W0_USB_PHY_TX_FS_LS_TUNE_MASK                   0x000f0000
#define SYS_USB_CFG0_W0_USB_PHY_VREG_BYPASS_MASK                     0x00000800
#define SYS_USB_CFG0_W0_USB_PHY_TX_PRE_EMP_PULSE_TUNE_MASK           0x00000080
#define SYS_USB_CFG0_W0_USB_PHY_SQ_RX_TUNE_MASK                      0x00007000

/* ################################################################################
 * # SysUsbCfg1 Definition
 */
#define SYS_USB_CFG1_W0_USB_INTF_AUTOPPD_ON_OVERCUR_EN               BIT(1)
#define SYS_USB_CFG1_W0_USB_INTF_FL_ADJ_VAL_HOST                     BIT(4)
#define SYS_USB_CFG1_W0_USB_INTF_WORD_IF                             BIT(16)
#define SYS_USB_CFG1_W0_USB_INTF_OHCI_CNT_SEL                        BIT(13)
#define SYS_USB_CFG1_W0_USB_INTF_OHCI_SUSP_LGCY                      BIT(14)
#define SYS_USB_CFG1_W0_USB_INTF_OHCI_CLKCKTRST_BAR                  BIT(12)
#define SYS_USB_CFG1_W0_USB_INTF_APP_START_CLK                       BIT(0)
#define SYS_USB_CFG1_W0_USB_PHY_LOOPBACK_EN                          BIT(17)
#define SYS_USB_CFG1_W0_USB_INTF_SIM_MODE                            BIT(15)
#define SYS_USB_CFG1_W0_USB_INTF_HUBSETUP_MIN                        BIT(2)

#define SYS_USB_CFG1_W0_USB_INTF_AUTOPPD_ON_OVERCUR_EN_MASK          0x00000002
#define SYS_USB_CFG1_W0_USB_INTF_FL_ADJ_VAL_HOST_MASK                0x000003f0
#define SYS_USB_CFG1_W0_USB_INTF_WORD_IF_MASK                        0x00010000
#define SYS_USB_CFG1_W0_USB_INTF_OHCI_CNT_SEL_MASK                   0x00002000
#define SYS_USB_CFG1_W0_USB_INTF_OHCI_SUSP_LGCY_MASK                 0x00004000
#define SYS_USB_CFG1_W0_USB_INTF_OHCI_CLKCKTRST_BAR_MASK             0x00001000
#define SYS_USB_CFG1_W0_USB_INTF_APP_START_CLK_MASK                  0x00000001
#define SYS_USB_CFG1_W0_USB_PHY_LOOPBACK_EN_MASK                     0x00020000
#define SYS_USB_CFG1_W0_USB_INTF_SIM_MODE_MASK                       0x00008000
#define SYS_USB_CFG1_W0_USB_INTF_HUBSETUP_MIN_MASK                   0x00000004

/* ################################################################################
 * # SysUsbCfg2 Definition
 */
#define SYS_USB_CFG2_W0_USB_PHY_PLL_P_TUNE                           BIT(8)
#define SYS_USB_CFG2_W0_USB_PHY_PLL_I_TUNE                           BIT(4)
#define SYS_USB_CFG2_W0_USB_PHY_TX_BIT_STUFF_EN0                     BIT(16)
#define SYS_USB_CFG2_W0_USB_PHY_TX_BIT_STUFF_EN_H0                   BIT(17)
#define SYS_USB_CFG2_W0_USB_PHY_OTG_TUNE                             BIT(0)
#define SYS_USB_CFG2_W0_USB_PHY_VDAT_REF_TUNE                        BIT(12)

#define SYS_USB_CFG2_W0_USB_PHY_PLL_P_TUNE_MASK                      0x00000f00
#define SYS_USB_CFG2_W0_USB_PHY_PLL_I_TUNE_MASK                      0x00000030
#define SYS_USB_CFG2_W0_USB_PHY_TX_BIT_STUFF_EN0_MASK                0x00010000
#define SYS_USB_CFG2_W0_USB_PHY_TX_BIT_STUFF_EN_H0_MASK              0x00020000
#define SYS_USB_CFG2_W0_USB_PHY_OTG_TUNE_MASK                        0x00000007
#define SYS_USB_CFG2_W0_USB_PHY_VDAT_REF_TUNE_MASK                   0x00003000

/* ################################################################################
 * # SysUsbStatus Definition
 */
#define SYS_USB_STATUS_W0_USB_INTF_EHCI_LPSMC_STATE                  BIT(0)
#define SYS_USB_STATUS_W0_USB_INTF_EHCI_XFER_CNT                     BIT(4)
#define SYS_USB_STATUS_W0_USB_INTF_OHCI_ISOC_OUT_MEM_READ            BIT(18)
#define SYS_USB_STATUS_W0_USB_INTF_OHCI_CCS                          BIT(16)
#define SYS_USB_STATUS_W0_USB_INTF_EHCI_XFER_PRDC                    BIT(15)
#define SYS_USB_STATUS_W0_USB_INTF_OHCI_GLOBALSUSPEND                BIT(17)
#define SYS_USB_STATUS_W0_USB_INTF_OHCI_RMTWKP                       BIT(19)

#define SYS_USB_STATUS_W0_USB_INTF_EHCI_LPSMC_STATE_MASK             0x0000000f
#define SYS_USB_STATUS_W0_USB_INTF_EHCI_XFER_CNT_MASK                0x00007ff0
#define SYS_USB_STATUS_W0_USB_INTF_OHCI_ISOC_OUT_MEM_READ_MASK       0x00040000
#define SYS_USB_STATUS_W0_USB_INTF_OHCI_CCS_MASK                     0x00010000
#define SYS_USB_STATUS_W0_USB_INTF_EHCI_XFER_PRDC_MASK               0x00008000
#define SYS_USB_STATUS_W0_USB_INTF_OHCI_GLOBALSUSPEND_MASK           0x00020000
#define SYS_USB_STATUS_W0_USB_INTF_OHCI_RMTWKP_MASK                  0x00080000

/* ################################################################################
 * # SysPcieBaseCfg Definition
 */
#define SYS_PCIE_BASE_CFG_W0_PCIE_BASE_CFG                           BIT(0)

#define SYS_PCIE_BASE_CFG_W0_PCIE_BASE_CFG_MASK                      0x000fffff

/* ################################################################################
 * # SysRegCfg Definition
 */
#define SYS_REG_CFG_W0_EN_CLK_GLOBAL_SYS                             BIT(0)
#define SYS_REG_CFG_W0_CFG_SOC_ACC_DIR_EN                            BIT(1)

#define SYS_REG_CFG_W0_EN_CLK_GLOBAL_SYS_MASK                        0x00000001
#define SYS_REG_CFG_W0_CFG_SOC_ACC_DIR_EN_MASK                       0x00000002

/* ################################################################################
 * # SysInitCtl Definition
 */
#define SYS_INIT_CTL_W0_CPU_MEM_INIT_DONE                            BIT(1)
#define SYS_INIT_CTL_W0_CFG_INIT_EN                                  BIT(0)
#define SYS_INIT_CTL_W1_CFG_INIT_START_PTR                           BIT(0)
#define SYS_INIT_CTL_W1_CFG_INIT_END_PTR                             BIT(16)

#define SYS_INIT_CTL_W0_CPU_MEM_INIT_DONE_MASK                       0x00000002
#define SYS_INIT_CTL_W0_CFG_INIT_EN_MASK                             0x00000001
#define SYS_INIT_CTL_W1_CFG_INIT_START_PTR_MASK                      0x00007fff
#define SYS_INIT_CTL_W1_CFG_INIT_END_PTR_MASK                        0x7fff0000

/* ################################################################################
 * # SysPllSupCfg0 Definition
 */
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_PLL_PWD                          BIT(3)
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_MULT_INT                         BIT(20)
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_RESET                            BIT(0)
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_INT_FBK                          BIT(2)
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_PRE_DIV                          BIT(4)
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_DCO_BYPASS                       BIT(1)
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_POST_DIV                         BIT(12)

#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_PLL_PWD_MASK                     0x00000008
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_MULT_INT_MASK                    0x0ff00000
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_RESET_MASK                       0x00000001
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_INT_FBK_MASK                     0x00000004
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_PRE_DIV_MASK                     0x000001f0
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_DCO_BYPASS_MASK                  0x00000002
#define SYS_PLL_SUP_CFG0_W0_PLL_SUP_POST_DIV_MASK                    0x0003f000

/* ################################################################################
 * # SysPllSupCfg1 Definition
 */
#define SYS_PLL_SUP_CFG1_W0_MON_PLL_SUP_LOCK                         BIT(28)
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_SGAIN                            BIT(20)
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_SIP                              BIT(0)
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_SIC                              BIT(8)
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_SLOCK                            BIT(16)
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_BYPASS                           BIT(24)

#define SYS_PLL_SUP_CFG1_W0_MON_PLL_SUP_LOCK_MASK                    0x10000000
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_SGAIN_MASK                       0x00700000
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_SIP_MASK                         0x0000001f
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_SIC_MASK                         0x00001f00
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_SLOCK_MASK                       0x00010000
#define SYS_PLL_SUP_CFG1_W0_PLL_SUP_BYPASS_MASK                      0x01000000

/* ################################################################################
 * # SysApbProcTimer Definition
 */
#define SYS_APB_PROC_TIMER_W0_CFG_APB_PROC_TIMER                     BIT(0)

#define SYS_APB_PROC_TIMER_W0_CFG_APB_PROC_TIMER_MASK                0x0000ffff

/* ################################################################################
 * # SysUsbTest Definition
 */
#define SYS_USB_TEST_W0_USB_PHY_TEST_DATA_IN                         BIT(8)
#define SYS_USB_TEST_W0_USB_PHY_TEST_DATA_OUT_SEL                    BIT(16)
#define SYS_USB_TEST_W0_USB_PHY_TEST_ADDR                            BIT(0)
#define SYS_USB_TEST_W1_USB_PHY_TEST_CLK                             BIT(16)
#define SYS_USB_TEST_W1_USB_PHY_TEST_DATA_OUT                        BIT(0)

#define SYS_USB_TEST_W0_USB_PHY_TEST_DATA_IN_MASK                    0x0000ff00
#define SYS_USB_TEST_W0_USB_PHY_TEST_DATA_OUT_SEL_MASK               0x00010000
#define SYS_USB_TEST_W0_USB_PHY_TEST_ADDR_MASK                       0x0000000f
#define SYS_USB_TEST_W1_USB_PHY_TEST_CLK_MASK                        0x00010000
#define SYS_USB_TEST_W1_USB_PHY_TEST_DATA_OUT_MASK                   0x0000000f

/* ################################################################################
 * # SysGpioMultiCtl Definition
 */
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO14_SEL                         BIT(28)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO5_SEL                          BIT(10)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO15_SEL                         BIT(30)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO7_SEL                          BIT(14)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO9_SEL                          BIT(18)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO13_SEL                         BIT(26)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO0_SEL                          BIT(0)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO1_SEL                          BIT(2)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO6_SEL                          BIT(12)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO11_SEL                         BIT(22)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO3_SEL                          BIT(6)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO10_SEL                         BIT(20)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO2_SEL                          BIT(4)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO8_SEL                          BIT(16)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO12_SEL                         BIT(24)
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO4_SEL                          BIT(8)

#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO14_SEL_MASK                    0x30000000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO5_SEL_MASK                     0x00000c00
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO15_SEL_MASK                    0xc0000000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO7_SEL_MASK                     0x0000c000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO9_SEL_MASK                     0x000c0000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO13_SEL_MASK                    0x0c000000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO0_SEL_MASK                     0x00000003
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO1_SEL_MASK                     0x0000000c
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO6_SEL_MASK                     0x00003000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO11_SEL_MASK                    0x00c00000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO3_SEL_MASK                     0x000000c0
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO10_SEL_MASK                    0x00300000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO2_SEL_MASK                     0x00000030
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO8_SEL_MASK                     0x00030000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO12_SEL_MASK                    0x03000000
#define SYS_GPIO_MULTI_CTL_W0_CFG_GPIO4_SEL_MASK                     0x00000300

/* ################################################################################
 * # SysGpioHsMultiCtl Definition
 */
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS13_SEL                   BIT(26)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS7_SEL                    BIT(14)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS9_SEL                    BIT(18)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS14_SEL                   BIT(28)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS11_SEL                   BIT(22)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS15_SEL                   BIT(30)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS5_SEL                    BIT(10)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS4_SEL                    BIT(8)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS10_SEL                   BIT(20)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS8_SEL                    BIT(16)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS12_SEL                   BIT(24)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS6_SEL                    BIT(12)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS3_SEL                    BIT(6)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS2_SEL                    BIT(4)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS0_SEL                    BIT(0)
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS1_SEL                    BIT(2)
#define SYS_GPIO_HS_MULTI_CTL_W1_CFG_GPIO_HS17_SEL                   BIT(2)
#define SYS_GPIO_HS_MULTI_CTL_W1_CFG_GPIO_HS16_SEL                   BIT(0)

#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS13_SEL_MASK              0x0c000000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS7_SEL_MASK               0x0000c000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS9_SEL_MASK               0x000c0000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS14_SEL_MASK              0x30000000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS11_SEL_MASK              0x00c00000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS15_SEL_MASK              0xc0000000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS5_SEL_MASK               0x00000c00
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS4_SEL_MASK               0x00000300
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS10_SEL_MASK              0x00300000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS8_SEL_MASK               0x00030000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS12_SEL_MASK              0x03000000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS6_SEL_MASK               0x00003000
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS3_SEL_MASK               0x000000c0
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS2_SEL_MASK               0x00000030
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS0_SEL_MASK               0x00000003
#define SYS_GPIO_HS_MULTI_CTL_W0_CFG_GPIO_HS1_SEL_MASK               0x0000000c
#define SYS_GPIO_HS_MULTI_CTL_W1_CFG_GPIO_HS17_SEL_MASK              0x0000000c
#define SYS_GPIO_HS_MULTI_CTL_W1_CFG_GPIO_HS16_SEL_MASK              0x00000003

/* ################################################################################
 * # SysPcieMemCtl Definition 
 */
#define SYS_PCIE_MEM_CTL_W0_CFG_AXI4_LITE_BYTE	                 	 BIT(4)
#define SYS_PCIE_MEM_CTL_W0_CFG_AXI4_LITE_MASK                  	 BIT(0)
#define SYS_PCIE_MEM_CTL_W0_CFG_AXI4_LITE_PROT                       BIT(8)
#define SYS_PCIE_MEM_CTL_W0_LOG_AXI4_LITE_RD_RESP                    BIT(16)
#define SYS_PCIE_MEM_CTL_W0_LOG_AXI4_LITE_WR_RESP                    BIT(20)

#define SYS_PCIE_MEM_CTL_W0_CFG_AXI4_LITE_BYTE_MASK                  0x00000030
#define SYS_PCIE_MEM_CTL_W0_CFG_AXI4_LITE_MASK_MASK                  0x0000000f
#define SYS_PCIE_MEM_CTL_W0_CFG_AXI4_LITE_PROT_MASK                  0x00000700
#define SYS_PCIE_MEM_CTL_W0_LOG_AXI4_LITE_RD_RESP_MASK               0x00030000
#define SYS_PCIE_MEM_CTL_W0_LOG_AXI4_LITE_WR_RESP_MASK               0x00300000

/* ################################################################################
 * # SysPcieStatus Definition
 */
#define SYS_PCIE_STATUS_W0_PCIE_LTSSM_LOG_31_0                       BIT(0)
#define SYS_PCIE_STATUS_W1_PCIE_LTSSM_LOG_63_32                      BIT(0)

#define SYS_PCIE_STATUS_W0_PCIE_LTSSM_LOG_31_0_MASK                  0x00000001
#define SYS_PCIE_STATUS_W1_PCIE_LTSSM_LOG_63_32_MASK                 0x00000001

/* ################################################################################
 * # SysMsixStatus Definition
 */
#define SYS_MSIX_STATUS_W0_MSIX_STATUS0                              BIT(0)
#define SYS_MSIX_STATUS_W1_MSIX_STATUS1                              BIT(0)
#define SYS_MSIX_STATUS_W2_MSIX_STATUS2                              BIT(0)
#define SYS_MSIX_STATUS_W3_MSIX_STATUS3                              BIT(0)
#define SYS_MSIX_STATUS_W4_MSIX_STATUS4                              BIT(0)
#define SYS_MSIX_STATUS_W5_MSIX_STATUS5                              BIT(0)
#define SYS_MSIX_STATUS_W6_MSIX_STATUS6                              BIT(0)
#define SYS_MSIX_STATUS_W7_MSIX_STATUS7                              BIT(0)

#define SYS_MSIX_STATUS_W0_MSIX_STATUS0_MASK                         0xffffffff
#define SYS_MSIX_STATUS_W1_MSIX_STATUS1_MASK                         0xffffffff
#define SYS_MSIX_STATUS_W2_MSIX_STATUS2_MASK                         0xffffffff
#define SYS_MSIX_STATUS_W3_MSIX_STATUS3_MASK                         0xffffffff
#define SYS_MSIX_STATUS_W4_MSIX_STATUS4_MASK                         0xffffffff
#define SYS_MSIX_STATUS_W5_MSIX_STATUS5_MASK                         0xffffffff
#define SYS_MSIX_STATUS_W6_MSIX_STATUS6_MASK                         0xffffffff
#define SYS_MSIX_STATUS_W7_MSIX_STATUS7_MASK                         0xffffffff

/* ################################################################################
 * # SysMsixMask Definition
 */
#define SYS_MSIX_MASK_W0_MSIX_MASK0                                  BIT(0)
#define SYS_MSIX_MASK_W1_MSIX_MASK1                                  BIT(0)
#define SYS_MSIX_MASK_W2_MSIX_MASK2                                  BIT(0)
#define SYS_MSIX_MASK_W3_MSIX_MASK3                                  BIT(0)
#define SYS_MSIX_MASK_W4_MSIX_MASK4                                  BIT(0)
#define SYS_MSIX_MASK_W5_MSIX_MASK5                                  BIT(0)
#define SYS_MSIX_MASK_W6_MSIX_MASK6                                  BIT(0)
#define SYS_MSIX_MASK_W7_MSIX_MASK7                                  BIT(0)

#define SYS_MSIX_MASK_W0_MSIX_MASK0_MASK                             0xffffffff
#define SYS_MSIX_MASK_W1_MSIX_MASK1_MASK                             0xffffffff
#define SYS_MSIX_MASK_W2_MSIX_MASK2_MASK                             0xffffffff
#define SYS_MSIX_MASK_W3_MSIX_MASK3_MASK                             0xffffffff
#define SYS_MSIX_MASK_W4_MSIX_MASK4_MASK                             0xffffffff
#define SYS_MSIX_MASK_W5_MSIX_MASK5_MASK                             0xffffffff
#define SYS_MSIX_MASK_W6_MSIX_MASK6_MASK                             0xffffffff
#define SYS_MSIX_MASK_W7_MSIX_MASK7_MASK                             0xffffffff

/* ################################################################################
 * # SysMsixAddr Definition
 */
#define SYS_MSIX_ADDR_W0_MSIX_ADDR                                   BIT(0)

#define SYS_MSIX_ADDR_W0_MSIX_ADDR_MASK                              0xffffffff

/* ################################################################################
 * # SysMsixVecCtl Definition
 */
#define SYS_MSIX_VEC_CTL_W0_MSIX_VEC_EN                              BIT(0)

#define SYS_MSIX_VEC_CTL_W0_MSIX_VEC_EN_MASK                         0x000000ff

/* ################################################################################
 * # SysMsixAddrEn Definition
 */
#define SYS_MSIX_ADDR_EN_W0_MSIX_ADDR_EN                             BIT(0)

#define SYS_MSIX_ADDR_EN_W0_MSIX_ADDR_EN_MASK                        0x00000001

/* ################################################################################
 * # SysMsixIntrLog Definition
 */
#define SYS_MSIX_INTR_LOG_W0_MSIX_INTR_LOG                           BIT(0)

#define SYS_MSIX_INTR_LOG_W0_MSIX_INTR_LOG_MASK                      0x000000ff

/* ################################################################################
 * # SysDebugCtl Definition
 */
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_PCIE                             BIT(4)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_SYS_APB                          BIT(8)
#define SYS_DEBUG_CTL_W0_DEBUG_EN_SYS_APB                            BIT(9)
#define SYS_DEBUG_CTL_W0_DEBUG_EN_SEC_APB                            BIT(13)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_MSH                              BIT(7)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_DDR1                             BIT(3)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_CPU                              BIT(0)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_DDR0                             BIT(2)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_SEC_APB                          BIT(12)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_MEM                              BIT(1)
#define SYS_DEBUG_CTL_W0_DEBUG_ONCE_SEC_APB                          BIT(14)
#define SYS_DEBUG_CTL_W0_DEBUG_ONCE_SYS_APB                          BIT(10)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_SUP                              BIT(6)
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_QSPI                             BIT(5)

#define SYS_DEBUG_CTL_W0_DEBUG_INIT_PCIE_MASK                        0x00000010
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_SYS_APB_MASK                     0x00000100
#define SYS_DEBUG_CTL_W0_DEBUG_EN_SYS_APB_MASK                       0x00000200
#define SYS_DEBUG_CTL_W0_DEBUG_EN_SEC_APB_MASK                       0x00002000
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_MSH_MASK                         0x00000080
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_DDR1_MASK                        0x00000008
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_CPU_MASK                         0x00000001
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_DDR0_MASK                        0x00000004
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_SEC_APB_MASK                     0x00001000
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_MEM_MASK                         0x00000002
#define SYS_DEBUG_CTL_W0_DEBUG_ONCE_SEC_APB_MASK                     0x00004000
#define SYS_DEBUG_CTL_W0_DEBUG_ONCE_SYS_APB_MASK                     0x00000400
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_SUP_MASK                         0x00000040
#define SYS_DEBUG_CTL_W0_DEBUG_INIT_QSPI_MASK                        0x00000020

/* ################################################################################
 * # SysMsixPending Definition
 */
#define SYS_MSIX_PENDING_W0_MSIX_PENDING0                            BIT(0)
#define SYS_MSIX_PENDING_W1_MSIX_PENDING1                            BIT(0)
#define SYS_MSIX_PENDING_W2_MSIX_PENDING2                            BIT(0)
#define SYS_MSIX_PENDING_W3_MSIX_PENDING3                            BIT(0)
#define SYS_MSIX_PENDING_W4_MSIX_PENDING4                            BIT(0)
#define SYS_MSIX_PENDING_W5_MSIX_PENDING5                            BIT(0)
#define SYS_MSIX_PENDING_W6_MSIX_PENDING6                            BIT(0)
#define SYS_MSIX_PENDING_W7_MSIX_PENDING7                            BIT(0)

#define SYS_MSIX_PENDING_W0_MSIX_PENDING0_MASK                       0xffffffff
#define SYS_MSIX_PENDING_W1_MSIX_PENDING1_MASK                       0xffffffff
#define SYS_MSIX_PENDING_W2_MSIX_PENDING2_MASK                       0xffffffff
#define SYS_MSIX_PENDING_W3_MSIX_PENDING3_MASK                       0xffffffff
#define SYS_MSIX_PENDING_W4_MSIX_PENDING4_MASK                       0xffffffff
#define SYS_MSIX_PENDING_W5_MSIX_PENDING5_MASK                       0xffffffff
#define SYS_MSIX_PENDING_W6_MSIX_PENDING6_MASK                       0xffffffff
#define SYS_MSIX_PENDING_W7_MSIX_PENDING7_MASK                       0xffffffff

/* ################################################################################
 * # SysPwmCtl Definition
 */
#define SYS_PWM_CTL_W0_CFG_PWM0_EN                                   BIT(31)
#define SYS_PWM_CTL_W0_CFG_PWM0_PERIOD_CYCLE                         BIT(0)
#define SYS_PWM_CTL_W1_CFG_PWM0_DUTY_CYCLE                           BIT(0)
#define SYS_PWM_CTL_W2_CFG_PWM1_EN                                   BIT(31)
#define SYS_PWM_CTL_W2_CFG_PWM1_PERIOD_CYCLE                         BIT(0)
#define SYS_PWM_CTL_W3_CFG_PWM1_DUTY_CYCLE                           BIT(0)
#define SYS_PWM_CTL_W4_CFG_PWM2_EN                                   BIT(31)
#define SYS_PWM_CTL_W4_CFG_PWM2_PERIOD_CYCLE                         BIT(0)
#define SYS_PWM_CTL_W5_CFG_PWM2_DUTY_CYCLE                           BIT(0)
#define SYS_PWM_CTL_W6_CFG_PWM3_EN                                   BIT(31)
#define SYS_PWM_CTL_W6_CFG_PWM3_PERIOD_CYCLE                         BIT(0)
#define SYS_PWM_CTL_W7_CFG_PWM3_DUTY_CYCLE                           BIT(0)

#define SYS_PWM_CTL_W0_CFG_PWM0_EN_MASK                              0x80000000
#define SYS_PWM_CTL_W0_CFG_PWM0_PERIOD_CYCLE_MASK                    0x00ffffff
#define SYS_PWM_CTL_W1_CFG_PWM0_DUTY_CYCLE_MASK                      0x00ffffff
#define SYS_PWM_CTL_W2_CFG_PWM1_EN_MASK                              0x80000000
#define SYS_PWM_CTL_W2_CFG_PWM1_PERIOD_CYCLE_MASK                    0x00ffffff
#define SYS_PWM_CTL_W3_CFG_PWM1_DUTY_CYCLE_MASK                      0x00ffffff
#define SYS_PWM_CTL_W4_CFG_PWM2_EN_MASK                              0x80000000
#define SYS_PWM_CTL_W4_CFG_PWM2_PERIOD_CYCLE_MASK                    0x00ffffff
#define SYS_PWM_CTL_W5_CFG_PWM2_DUTY_CYCLE_MASK                      0x00ffffff
#define SYS_PWM_CTL_W6_CFG_PWM3_EN_MASK                              0x80000000
#define SYS_PWM_CTL_W6_CFG_PWM3_PERIOD_CYCLE_MASK                    0x00ffffff
#define SYS_PWM_CTL_W7_CFG_PWM3_DUTY_CYCLE_MASK                      0x00ffffff

/* ################################################################################
 * # SysTachLog Definition
 */
#define SYS_TACH_LOG_W0_TACH0_PERIOD_CYCLE                           BIT(0)
#define SYS_TACH_LOG_W1_TACH0_DUTY_CYCLE                             BIT(0)
#define SYS_TACH_LOG_W2_TACH1_PERIOD_CYCLE                           BIT(0)
#define SYS_TACH_LOG_W3_TACH1_DUTY_CYCLE                             BIT(0)
#define SYS_TACH_LOG_W4_TACH2_PERIOD_CYCLE                           BIT(0)
#define SYS_TACH_LOG_W5_TACH2_DUTY_CYCLE                             BIT(0)
#define SYS_TACH_LOG_W6_TACH3_PERIOD_CYCLE                           BIT(0)
#define SYS_TACH_LOG_W7_TACH3_DUTY_CYCLE                             BIT(0)

#define SYS_TACH_LOG_W0_TACH0_PERIOD_CYCLE_MASK                      0x00ffffff
#define SYS_TACH_LOG_W1_TACH0_DUTY_CYCLE_MASK                        0x00ffffff
#define SYS_TACH_LOG_W2_TACH1_PERIOD_CYCLE_MASK                      0x00ffffff
#define SYS_TACH_LOG_W3_TACH1_DUTY_CYCLE_MASK                        0x00ffffff
#define SYS_TACH_LOG_W4_TACH2_PERIOD_CYCLE_MASK                      0x00ffffff
#define SYS_TACH_LOG_W5_TACH2_DUTY_CYCLE_MASK                        0x00ffffff
#define SYS_TACH_LOG_W6_TACH3_PERIOD_CYCLE_MASK                      0x00ffffff
#define SYS_TACH_LOG_W7_TACH3_DUTY_CYCLE_MASK                        0x00ffffff

/* ################################################################################
 * # MonAxiCpuCurInfo Definition
 */
#define MON_AXI_CPU_CUR_INFO_W0_MON_AXI_CPU_CUR_INFO_31_0            BIT(0)
#define MON_AXI_CPU_CUR_INFO_W1_MON_AXI_CPU_CUR_INFO_63_32           BIT(0)
#define MON_AXI_CPU_CUR_INFO_W2_MON_AXI_CPU_CUR_INFO_95_64           BIT(0)
#define MON_AXI_CPU_CUR_INFO_W3_MON_AXI_CPU_CUR_INFO_127_96          BIT(0)
#define MON_AXI_CPU_CUR_INFO_W4_MON_AXI_CPU_CUR_INFO_159_128         BIT(0)
#define MON_AXI_CPU_CUR_INFO_W5_MON_AXI_CPU_CUR_INFO_191_160         BIT(0)
#define MON_AXI_CPU_CUR_INFO_W6_MON_AXI_CPU_CUR_INFO_223_192         BIT(0)
#define MON_AXI_CPU_CUR_INFO_W7_MON_AXI_CPU_CUR_INFO_255_224         BIT(0)
#define MON_AXI_CPU_CUR_INFO_W8_MON_AXI_CPU_CUR_INFO_287_256         BIT(0)
#define MON_AXI_CPU_CUR_INFO_W9_MON_AXI_CPU_CUR_INFO_319_288         BIT(0)
#define MON_AXI_CPU_CUR_INFO_W10_MON_AXI_CPU_CUR_INFO_351_320        BIT(0)
#define MON_AXI_CPU_CUR_INFO_W11_MON_AXI_CPU_CUR_INFO_383_352        BIT(0)
#define MON_AXI_CPU_CUR_INFO_W12_MON_AXI_CPU_CUR_INFO_415_384        BIT(0)
#define MON_AXI_CPU_CUR_INFO_W13_MON_AXI_CPU_CUR_INFO_427_416        BIT(0)

#define MON_AXI_CPU_CUR_INFO_W0_MON_AXI_CPU_CUR_INFO_31_0_MASK       0x00000001
#define MON_AXI_CPU_CUR_INFO_W1_MON_AXI_CPU_CUR_INFO_63_32_MASK      0x00000001
#define MON_AXI_CPU_CUR_INFO_W2_MON_AXI_CPU_CUR_INFO_95_64_MASK      0x00000001
#define MON_AXI_CPU_CUR_INFO_W3_MON_AXI_CPU_CUR_INFO_127_96_MASK     0x00000001
#define MON_AXI_CPU_CUR_INFO_W4_MON_AXI_CPU_CUR_INFO_159_128_MASK    0x00000001
#define MON_AXI_CPU_CUR_INFO_W5_MON_AXI_CPU_CUR_INFO_191_160_MASK    0x00000001
#define MON_AXI_CPU_CUR_INFO_W6_MON_AXI_CPU_CUR_INFO_223_192_MASK    0x00000001
#define MON_AXI_CPU_CUR_INFO_W7_MON_AXI_CPU_CUR_INFO_255_224_MASK    0x00000001
#define MON_AXI_CPU_CUR_INFO_W8_MON_AXI_CPU_CUR_INFO_287_256_MASK    0x00000001
#define MON_AXI_CPU_CUR_INFO_W9_MON_AXI_CPU_CUR_INFO_319_288_MASK    0x00000001
#define MON_AXI_CPU_CUR_INFO_W10_MON_AXI_CPU_CUR_INFO_351_320_MASK   0x00000001
#define MON_AXI_CPU_CUR_INFO_W11_MON_AXI_CPU_CUR_INFO_383_352_MASK   0x00000001
#define MON_AXI_CPU_CUR_INFO_W12_MON_AXI_CPU_CUR_INFO_415_384_MASK   0x00000001
#define MON_AXI_CPU_CUR_INFO_W13_MON_AXI_CPU_CUR_INFO_427_416_MASK   0x00000001

/* ################################################################################
 * # MonAxiCpuLogInfo Definition
 */
#define MON_AXI_CPU_LOG_INFO_W0_MON_AXI_CPU_LOG_INFO_31_0            BIT(0)
#define MON_AXI_CPU_LOG_INFO_W1_MON_AXI_CPU_LOG_INFO_63_32           BIT(0)
#define MON_AXI_CPU_LOG_INFO_W2_MON_AXI_CPU_LOG_INFO_95_64           BIT(0)
#define MON_AXI_CPU_LOG_INFO_W3_MON_AXI_CPU_LOG_INFO_127_96          BIT(0)
#define MON_AXI_CPU_LOG_INFO_W4_MON_AXI_CPU_LOG_INFO_159_128         BIT(0)
#define MON_AXI_CPU_LOG_INFO_W5_MON_AXI_CPU_LOG_INFO_191_160         BIT(0)
#define MON_AXI_CPU_LOG_INFO_W6_MON_AXI_CPU_LOG_INFO_223_192         BIT(0)
#define MON_AXI_CPU_LOG_INFO_W7_MON_AXI_CPU_LOG_INFO_255_224         BIT(0)
#define MON_AXI_CPU_LOG_INFO_W8_MON_AXI_CPU_LOG_INFO_287_256         BIT(0)
#define MON_AXI_CPU_LOG_INFO_W9_MON_AXI_CPU_LOG_INFO_319_288         BIT(0)
#define MON_AXI_CPU_LOG_INFO_W10_MON_AXI_CPU_LOG_INFO_351_320        BIT(0)
#define MON_AXI_CPU_LOG_INFO_W11_MON_AXI_CPU_LOG_INFO_383_352        BIT(0)
#define MON_AXI_CPU_LOG_INFO_W12_MON_AXI_CPU_LOG_INFO_415_384        BIT(0)
#define MON_AXI_CPU_LOG_INFO_W13_MON_AXI_CPU_LOG_INFO_427_416        BIT(0)

#define MON_AXI_CPU_LOG_INFO_W0_MON_AXI_CPU_LOG_INFO_31_0_MASK       0x00000001
#define MON_AXI_CPU_LOG_INFO_W1_MON_AXI_CPU_LOG_INFO_63_32_MASK      0x00000001
#define MON_AXI_CPU_LOG_INFO_W2_MON_AXI_CPU_LOG_INFO_95_64_MASK      0x00000001
#define MON_AXI_CPU_LOG_INFO_W3_MON_AXI_CPU_LOG_INFO_127_96_MASK     0x00000001
#define MON_AXI_CPU_LOG_INFO_W4_MON_AXI_CPU_LOG_INFO_159_128_MASK    0x00000001
#define MON_AXI_CPU_LOG_INFO_W5_MON_AXI_CPU_LOG_INFO_191_160_MASK    0x00000001
#define MON_AXI_CPU_LOG_INFO_W6_MON_AXI_CPU_LOG_INFO_223_192_MASK    0x00000001
#define MON_AXI_CPU_LOG_INFO_W7_MON_AXI_CPU_LOG_INFO_255_224_MASK    0x00000001
#define MON_AXI_CPU_LOG_INFO_W8_MON_AXI_CPU_LOG_INFO_287_256_MASK    0x00000001
#define MON_AXI_CPU_LOG_INFO_W9_MON_AXI_CPU_LOG_INFO_319_288_MASK    0x00000001
#define MON_AXI_CPU_LOG_INFO_W10_MON_AXI_CPU_LOG_INFO_351_320_MASK   0x00000001
#define MON_AXI_CPU_LOG_INFO_W11_MON_AXI_CPU_LOG_INFO_383_352_MASK   0x00000001
#define MON_AXI_CPU_LOG_INFO_W12_MON_AXI_CPU_LOG_INFO_415_384_MASK   0x00000001
#define MON_AXI_CPU_LOG_INFO_W13_MON_AXI_CPU_LOG_INFO_427_416_MASK   0x00000001

/* ################################################################################
 * # MonAxiDdr0CurInfo Definition
 */
#define MON_AXI_DDR0_CUR_INFO_W0_MON_AXI_DDR0_CUR_INFO_31_0          BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W1_MON_AXI_DDR0_CUR_INFO_63_32         BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W2_MON_AXI_DDR0_CUR_INFO_95_64         BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W3_MON_AXI_DDR0_CUR_INFO_127_96        BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W4_MON_AXI_DDR0_CUR_INFO_159_128       BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W5_MON_AXI_DDR0_CUR_INFO_191_160       BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W6_MON_AXI_DDR0_CUR_INFO_223_192       BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W7_MON_AXI_DDR0_CUR_INFO_255_224       BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W8_MON_AXI_DDR0_CUR_INFO_287_256       BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W9_MON_AXI_DDR0_CUR_INFO_319_288       BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W10_MON_AXI_DDR0_CUR_INFO_351_320      BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W11_MON_AXI_DDR0_CUR_INFO_383_352      BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W12_MON_AXI_DDR0_CUR_INFO_415_384      BIT(0)
#define MON_AXI_DDR0_CUR_INFO_W13_MON_AXI_DDR0_CUR_INFO_427_416      BIT(0)

#define MON_AXI_DDR0_CUR_INFO_W0_MON_AXI_DDR0_CUR_INFO_31_0_MASK     0x00000001
#define MON_AXI_DDR0_CUR_INFO_W1_MON_AXI_DDR0_CUR_INFO_63_32_MASK    0x00000001
#define MON_AXI_DDR0_CUR_INFO_W2_MON_AXI_DDR0_CUR_INFO_95_64_MASK    0x00000001
#define MON_AXI_DDR0_CUR_INFO_W3_MON_AXI_DDR0_CUR_INFO_127_96_MASK   0x00000001
#define MON_AXI_DDR0_CUR_INFO_W4_MON_AXI_DDR0_CUR_INFO_159_128_MASK  0x00000001
#define MON_AXI_DDR0_CUR_INFO_W5_MON_AXI_DDR0_CUR_INFO_191_160_MASK  0x00000001
#define MON_AXI_DDR0_CUR_INFO_W6_MON_AXI_DDR0_CUR_INFO_223_192_MASK  0x00000001
#define MON_AXI_DDR0_CUR_INFO_W7_MON_AXI_DDR0_CUR_INFO_255_224_MASK  0x00000001
#define MON_AXI_DDR0_CUR_INFO_W8_MON_AXI_DDR0_CUR_INFO_287_256_MASK  0x00000001
#define MON_AXI_DDR0_CUR_INFO_W9_MON_AXI_DDR0_CUR_INFO_319_288_MASK  0x00000001
#define MON_AXI_DDR0_CUR_INFO_W10_MON_AXI_DDR0_CUR_INFO_351_320_MASK 0x00000001
#define MON_AXI_DDR0_CUR_INFO_W11_MON_AXI_DDR0_CUR_INFO_383_352_MASK 0x00000001
#define MON_AXI_DDR0_CUR_INFO_W12_MON_AXI_DDR0_CUR_INFO_415_384_MASK 0x00000001
#define MON_AXI_DDR0_CUR_INFO_W13_MON_AXI_DDR0_CUR_INFO_427_416_MASK 0x00000001

/* ################################################################################
 * # MonAxiDdr0LogInfo Definition
 */
#define MON_AXI_DDR0_LOG_INFO_W0_MON_AXI_DDR0_LOG_INFO_31_0          BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W1_MON_AXI_DDR0_LOG_INFO_63_32         BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W2_MON_AXI_DDR0_LOG_INFO_95_64         BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W3_MON_AXI_DDR0_LOG_INFO_127_96        BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W4_MON_AXI_DDR0_LOG_INFO_159_128       BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W5_MON_AXI_DDR0_LOG_INFO_191_160       BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W6_MON_AXI_DDR0_LOG_INFO_223_192       BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W7_MON_AXI_DDR0_LOG_INFO_255_224       BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W8_MON_AXI_DDR0_LOG_INFO_287_256       BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W9_MON_AXI_DDR0_LOG_INFO_319_288       BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W10_MON_AXI_DDR0_LOG_INFO_351_320      BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W11_MON_AXI_DDR0_LOG_INFO_383_352      BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W12_MON_AXI_DDR0_LOG_INFO_415_384      BIT(0)
#define MON_AXI_DDR0_LOG_INFO_W13_MON_AXI_DDR0_LOG_INFO_427_416      BIT(0)

#define MON_AXI_DDR0_LOG_INFO_W0_MON_AXI_DDR0_LOG_INFO_31_0_MASK     0x00000001
#define MON_AXI_DDR0_LOG_INFO_W1_MON_AXI_DDR0_LOG_INFO_63_32_MASK    0x00000001
#define MON_AXI_DDR0_LOG_INFO_W2_MON_AXI_DDR0_LOG_INFO_95_64_MASK    0x00000001
#define MON_AXI_DDR0_LOG_INFO_W3_MON_AXI_DDR0_LOG_INFO_127_96_MASK   0x00000001
#define MON_AXI_DDR0_LOG_INFO_W4_MON_AXI_DDR0_LOG_INFO_159_128_MASK  0x00000001
#define MON_AXI_DDR0_LOG_INFO_W5_MON_AXI_DDR0_LOG_INFO_191_160_MASK  0x00000001
#define MON_AXI_DDR0_LOG_INFO_W6_MON_AXI_DDR0_LOG_INFO_223_192_MASK  0x00000001
#define MON_AXI_DDR0_LOG_INFO_W7_MON_AXI_DDR0_LOG_INFO_255_224_MASK  0x00000001
#define MON_AXI_DDR0_LOG_INFO_W8_MON_AXI_DDR0_LOG_INFO_287_256_MASK  0x00000001
#define MON_AXI_DDR0_LOG_INFO_W9_MON_AXI_DDR0_LOG_INFO_319_288_MASK  0x00000001
#define MON_AXI_DDR0_LOG_INFO_W10_MON_AXI_DDR0_LOG_INFO_351_320_MASK 0x00000001
#define MON_AXI_DDR0_LOG_INFO_W11_MON_AXI_DDR0_LOG_INFO_383_352_MASK 0x00000001
#define MON_AXI_DDR0_LOG_INFO_W12_MON_AXI_DDR0_LOG_INFO_415_384_MASK 0x00000001
#define MON_AXI_DDR0_LOG_INFO_W13_MON_AXI_DDR0_LOG_INFO_427_416_MASK 0x00000001

/* ################################################################################
 * # MonAxiDdr1CurInfo Definition
 */
#define MON_AXI_DDR1_CUR_INFO_W0_MON_AXI_DDR1_CUR_INFO_31_0          BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W1_MON_AXI_DDR1_CUR_INFO_63_32         BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W2_MON_AXI_DDR1_CUR_INFO_95_64         BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W3_MON_AXI_DDR1_CUR_INFO_127_96        BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W4_MON_AXI_DDR1_CUR_INFO_159_128       BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W5_MON_AXI_DDR1_CUR_INFO_191_160       BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W6_MON_AXI_DDR1_CUR_INFO_223_192       BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W7_MON_AXI_DDR1_CUR_INFO_255_224       BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W8_MON_AXI_DDR1_CUR_INFO_287_256       BIT(0)
#define MON_AXI_DDR1_CUR_INFO_W9_MON_AXI_DDR1_CUR_INFO_299_288       BIT(0)

#define MON_AXI_DDR1_CUR_INFO_W0_MON_AXI_DDR1_CUR_INFO_31_0_MASK     0x00000001
#define MON_AXI_DDR1_CUR_INFO_W1_MON_AXI_DDR1_CUR_INFO_63_32_MASK    0x00000001
#define MON_AXI_DDR1_CUR_INFO_W2_MON_AXI_DDR1_CUR_INFO_95_64_MASK    0x00000001
#define MON_AXI_DDR1_CUR_INFO_W3_MON_AXI_DDR1_CUR_INFO_127_96_MASK   0x00000001
#define MON_AXI_DDR1_CUR_INFO_W4_MON_AXI_DDR1_CUR_INFO_159_128_MASK  0x00000001
#define MON_AXI_DDR1_CUR_INFO_W5_MON_AXI_DDR1_CUR_INFO_191_160_MASK  0x00000001
#define MON_AXI_DDR1_CUR_INFO_W6_MON_AXI_DDR1_CUR_INFO_223_192_MASK  0x00000001
#define MON_AXI_DDR1_CUR_INFO_W7_MON_AXI_DDR1_CUR_INFO_255_224_MASK  0x00000001
#define MON_AXI_DDR1_CUR_INFO_W8_MON_AXI_DDR1_CUR_INFO_287_256_MASK  0x00000001
#define MON_AXI_DDR1_CUR_INFO_W9_MON_AXI_DDR1_CUR_INFO_299_288_MASK  0x00000001

/* ################################################################################
 * # MonAxiDdr1LogInfo Definition
 */
#define MON_AXI_DDR1_LOG_INFO_W0_MON_AXI_DDR1_LOG_INFO_31_0          BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W1_MON_AXI_DDR1_LOG_INFO_63_32         BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W2_MON_AXI_DDR1_LOG_INFO_95_64         BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W3_MON_AXI_DDR1_LOG_INFO_127_96        BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W4_MON_AXI_DDR1_LOG_INFO_159_128       BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W5_MON_AXI_DDR1_LOG_INFO_191_160       BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W6_MON_AXI_DDR1_LOG_INFO_223_192       BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W7_MON_AXI_DDR1_LOG_INFO_255_224       BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W8_MON_AXI_DDR1_LOG_INFO_287_256       BIT(0)
#define MON_AXI_DDR1_LOG_INFO_W9_MON_AXI_DDR1_LOG_INFO_299_288       BIT(0)

#define MON_AXI_DDR1_LOG_INFO_W0_MON_AXI_DDR1_LOG_INFO_31_0_MASK     0x00000001
#define MON_AXI_DDR1_LOG_INFO_W1_MON_AXI_DDR1_LOG_INFO_63_32_MASK    0x00000001
#define MON_AXI_DDR1_LOG_INFO_W2_MON_AXI_DDR1_LOG_INFO_95_64_MASK    0x00000001
#define MON_AXI_DDR1_LOG_INFO_W3_MON_AXI_DDR1_LOG_INFO_127_96_MASK   0x00000001
#define MON_AXI_DDR1_LOG_INFO_W4_MON_AXI_DDR1_LOG_INFO_159_128_MASK  0x00000001
#define MON_AXI_DDR1_LOG_INFO_W5_MON_AXI_DDR1_LOG_INFO_191_160_MASK  0x00000001
#define MON_AXI_DDR1_LOG_INFO_W6_MON_AXI_DDR1_LOG_INFO_223_192_MASK  0x00000001
#define MON_AXI_DDR1_LOG_INFO_W7_MON_AXI_DDR1_LOG_INFO_255_224_MASK  0x00000001
#define MON_AXI_DDR1_LOG_INFO_W8_MON_AXI_DDR1_LOG_INFO_287_256_MASK  0x00000001
#define MON_AXI_DDR1_LOG_INFO_W9_MON_AXI_DDR1_LOG_INFO_299_288_MASK  0x00000001

/* ################################################################################
 * # MonAxiMemCurInfo Definition
 */
#define MON_AXI_MEM_CUR_INFO_W0_MON_AXI_MEM_CUR_INFO_31_0            BIT(0)
#define MON_AXI_MEM_CUR_INFO_W1_MON_AXI_MEM_CUR_INFO_63_32           BIT(0)
#define MON_AXI_MEM_CUR_INFO_W2_MON_AXI_MEM_CUR_INFO_95_64           BIT(0)
#define MON_AXI_MEM_CUR_INFO_W3_MON_AXI_MEM_CUR_INFO_127_96          BIT(0)
#define MON_AXI_MEM_CUR_INFO_W4_MON_AXI_MEM_CUR_INFO_159_128         BIT(0)
#define MON_AXI_MEM_CUR_INFO_W5_MON_AXI_MEM_CUR_INFO_191_160         BIT(0)
#define MON_AXI_MEM_CUR_INFO_W6_MON_AXI_MEM_CUR_INFO_223_192         BIT(0)
#define MON_AXI_MEM_CUR_INFO_W7_MON_AXI_MEM_CUR_INFO_255_224         BIT(0)
#define MON_AXI_MEM_CUR_INFO_W8_MON_AXI_MEM_CUR_INFO_287_256         BIT(0)
#define MON_AXI_MEM_CUR_INFO_W9_MON_AXI_MEM_CUR_INFO_303_288         BIT(0)

#define MON_AXI_MEM_CUR_INFO_W0_MON_AXI_MEM_CUR_INFO_31_0_MASK       0x00000001
#define MON_AXI_MEM_CUR_INFO_W1_MON_AXI_MEM_CUR_INFO_63_32_MASK      0x00000001
#define MON_AXI_MEM_CUR_INFO_W2_MON_AXI_MEM_CUR_INFO_95_64_MASK      0x00000001
#define MON_AXI_MEM_CUR_INFO_W3_MON_AXI_MEM_CUR_INFO_127_96_MASK     0x00000001
#define MON_AXI_MEM_CUR_INFO_W4_MON_AXI_MEM_CUR_INFO_159_128_MASK    0x00000001
#define MON_AXI_MEM_CUR_INFO_W5_MON_AXI_MEM_CUR_INFO_191_160_MASK    0x00000001
#define MON_AXI_MEM_CUR_INFO_W6_MON_AXI_MEM_CUR_INFO_223_192_MASK    0x00000001
#define MON_AXI_MEM_CUR_INFO_W7_MON_AXI_MEM_CUR_INFO_255_224_MASK    0x00000001
#define MON_AXI_MEM_CUR_INFO_W8_MON_AXI_MEM_CUR_INFO_287_256_MASK    0x00000001
#define MON_AXI_MEM_CUR_INFO_W9_MON_AXI_MEM_CUR_INFO_303_288_MASK    0x00000001

/* ################################################################################
 * # MonAxiMemLogInfo Definition
 */
#define MON_AXI_MEM_LOG_INFO_W0_MON_AXI_MEM_LOG_INFO_31_0            BIT(0)
#define MON_AXI_MEM_LOG_INFO_W1_MON_AXI_MEM_LOG_INFO_63_32           BIT(0)
#define MON_AXI_MEM_LOG_INFO_W2_MON_AXI_MEM_LOG_INFO_95_64           BIT(0)
#define MON_AXI_MEM_LOG_INFO_W3_MON_AXI_MEM_LOG_INFO_127_96          BIT(0)
#define MON_AXI_MEM_LOG_INFO_W4_MON_AXI_MEM_LOG_INFO_159_128         BIT(0)
#define MON_AXI_MEM_LOG_INFO_W5_MON_AXI_MEM_LOG_INFO_191_160         BIT(0)
#define MON_AXI_MEM_LOG_INFO_W6_MON_AXI_MEM_LOG_INFO_223_192         BIT(0)
#define MON_AXI_MEM_LOG_INFO_W7_MON_AXI_MEM_LOG_INFO_255_224         BIT(0)
#define MON_AXI_MEM_LOG_INFO_W8_MON_AXI_MEM_LOG_INFO_287_256         BIT(0)
#define MON_AXI_MEM_LOG_INFO_W9_MON_AXI_MEM_LOG_INFO_303_288         BIT(0)

#define MON_AXI_MEM_LOG_INFO_W0_MON_AXI_MEM_LOG_INFO_31_0_MASK       0x00000001
#define MON_AXI_MEM_LOG_INFO_W1_MON_AXI_MEM_LOG_INFO_63_32_MASK      0x00000001
#define MON_AXI_MEM_LOG_INFO_W2_MON_AXI_MEM_LOG_INFO_95_64_MASK      0x00000001
#define MON_AXI_MEM_LOG_INFO_W3_MON_AXI_MEM_LOG_INFO_127_96_MASK     0x00000001
#define MON_AXI_MEM_LOG_INFO_W4_MON_AXI_MEM_LOG_INFO_159_128_MASK    0x00000001
#define MON_AXI_MEM_LOG_INFO_W5_MON_AXI_MEM_LOG_INFO_191_160_MASK    0x00000001
#define MON_AXI_MEM_LOG_INFO_W6_MON_AXI_MEM_LOG_INFO_223_192_MASK    0x00000001
#define MON_AXI_MEM_LOG_INFO_W7_MON_AXI_MEM_LOG_INFO_255_224_MASK    0x00000001
#define MON_AXI_MEM_LOG_INFO_W8_MON_AXI_MEM_LOG_INFO_287_256_MASK    0x00000001
#define MON_AXI_MEM_LOG_INFO_W9_MON_AXI_MEM_LOG_INFO_303_288_MASK    0x00000001

/* ################################################################################
 * # MonAxiMshCurInfo Definition
 */
#define MON_AXI_MSH_CUR_INFO_W0_MON_AXI_MSH_CUR_INFO_31_0            BIT(0)
#define MON_AXI_MSH_CUR_INFO_W1_MON_AXI_MSH_CUR_INFO_63_32           BIT(0)
#define MON_AXI_MSH_CUR_INFO_W2_MON_AXI_MSH_CUR_INFO_95_64           BIT(0)
#define MON_AXI_MSH_CUR_INFO_W3_MON_AXI_MSH_CUR_INFO_127_96          BIT(0)
#define MON_AXI_MSH_CUR_INFO_W4_MON_AXI_MSH_CUR_INFO_159_128         BIT(0)
#define MON_AXI_MSH_CUR_INFO_W5_MON_AXI_MSH_CUR_INFO_191_160         BIT(0)
#define MON_AXI_MSH_CUR_INFO_W6_MON_AXI_MSH_CUR_INFO_223_192         BIT(0)
#define MON_AXI_MSH_CUR_INFO_W7_MON_AXI_MSH_CUR_INFO_255_224         BIT(0)
#define MON_AXI_MSH_CUR_INFO_W8_MON_AXI_MSH_CUR_INFO_275_256         BIT(0)

#define MON_AXI_MSH_CUR_INFO_W0_MON_AXI_MSH_CUR_INFO_31_0_MASK       0x00000001
#define MON_AXI_MSH_CUR_INFO_W1_MON_AXI_MSH_CUR_INFO_63_32_MASK      0x00000001
#define MON_AXI_MSH_CUR_INFO_W2_MON_AXI_MSH_CUR_INFO_95_64_MASK      0x00000001
#define MON_AXI_MSH_CUR_INFO_W3_MON_AXI_MSH_CUR_INFO_127_96_MASK     0x00000001
#define MON_AXI_MSH_CUR_INFO_W4_MON_AXI_MSH_CUR_INFO_159_128_MASK    0x00000001
#define MON_AXI_MSH_CUR_INFO_W5_MON_AXI_MSH_CUR_INFO_191_160_MASK    0x00000001
#define MON_AXI_MSH_CUR_INFO_W6_MON_AXI_MSH_CUR_INFO_223_192_MASK    0x00000001
#define MON_AXI_MSH_CUR_INFO_W7_MON_AXI_MSH_CUR_INFO_255_224_MASK    0x00000001
#define MON_AXI_MSH_CUR_INFO_W8_MON_AXI_MSH_CUR_INFO_275_256_MASK    0x00000001

/* ################################################################################
 * # MonAxiMshLogInfo Definition
 */
#define MON_AXI_MSH_LOG_INFO_W0_MON_AXI_MSH_LOG_INFO_31_0            BIT(0)
#define MON_AXI_MSH_LOG_INFO_W1_MON_AXI_MSH_LOG_INFO_63_32           BIT(0)
#define MON_AXI_MSH_LOG_INFO_W2_MON_AXI_MSH_LOG_INFO_95_64           BIT(0)
#define MON_AXI_MSH_LOG_INFO_W3_MON_AXI_MSH_LOG_INFO_127_96          BIT(0)
#define MON_AXI_MSH_LOG_INFO_W4_MON_AXI_MSH_LOG_INFO_159_128         BIT(0)
#define MON_AXI_MSH_LOG_INFO_W5_MON_AXI_MSH_LOG_INFO_191_160         BIT(0)
#define MON_AXI_MSH_LOG_INFO_W6_MON_AXI_MSH_LOG_INFO_223_192         BIT(0)
#define MON_AXI_MSH_LOG_INFO_W7_MON_AXI_MSH_LOG_INFO_255_224         BIT(0)
#define MON_AXI_MSH_LOG_INFO_W8_MON_AXI_MSH_LOG_INFO_275_256         BIT(0)
#define MON_AXI_MSH_LOG_INFO_W8_MON_AXI_MSH_LOG_WD_ID                BIT(24)

#define MON_AXI_MSH_LOG_INFO_W0_MON_AXI_MSH_LOG_INFO_31_0_MASK       0x00000001
#define MON_AXI_MSH_LOG_INFO_W1_MON_AXI_MSH_LOG_INFO_63_32_MASK      0x00000001
#define MON_AXI_MSH_LOG_INFO_W2_MON_AXI_MSH_LOG_INFO_95_64_MASK      0x00000001
#define MON_AXI_MSH_LOG_INFO_W3_MON_AXI_MSH_LOG_INFO_127_96_MASK     0x00000001
#define MON_AXI_MSH_LOG_INFO_W4_MON_AXI_MSH_LOG_INFO_159_128_MASK    0x00000001
#define MON_AXI_MSH_LOG_INFO_W5_MON_AXI_MSH_LOG_INFO_191_160_MASK    0x00000001
#define MON_AXI_MSH_LOG_INFO_W6_MON_AXI_MSH_LOG_INFO_223_192_MASK    0x00000001
#define MON_AXI_MSH_LOG_INFO_W7_MON_AXI_MSH_LOG_INFO_255_224_MASK    0x00000001
#define MON_AXI_MSH_LOG_INFO_W8_MON_AXI_MSH_LOG_INFO_275_256_MASK    0x00000001
#define MON_AXI_MSH_LOG_INFO_W8_MON_AXI_MSH_LOG_WD_ID_MASK           0x0f000000

/* ################################################################################
 * # MonAxiPcieCurInfo Definition
 */
#define MON_AXI_PCIE_CUR_INFO_W0_MON_AXI_PCIE_CUR_INFO_31_0          BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W1_MON_AXI_PCIE_CUR_INFO_63_32         BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W2_MON_AXI_PCIE_CUR_INFO_95_64         BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W3_MON_AXI_PCIE_CUR_INFO_127_96        BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W4_MON_AXI_PCIE_CUR_INFO_159_128       BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W5_MON_AXI_PCIE_CUR_INFO_191_160       BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W6_MON_AXI_PCIE_CUR_INFO_223_192       BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W7_MON_AXI_PCIE_CUR_INFO_255_224       BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W8_MON_AXI_PCIE_CUR_INFO_287_256       BIT(0)
#define MON_AXI_PCIE_CUR_INFO_W9_MON_AXI_PCIE_CUR_INFO_291_288       BIT(0)

#define MON_AXI_PCIE_CUR_INFO_W0_MON_AXI_PCIE_CUR_INFO_31_0_MASK     0x00000001
#define MON_AXI_PCIE_CUR_INFO_W1_MON_AXI_PCIE_CUR_INFO_63_32_MASK    0x00000001
#define MON_AXI_PCIE_CUR_INFO_W2_MON_AXI_PCIE_CUR_INFO_95_64_MASK    0x00000001
#define MON_AXI_PCIE_CUR_INFO_W3_MON_AXI_PCIE_CUR_INFO_127_96_MASK   0x00000001
#define MON_AXI_PCIE_CUR_INFO_W4_MON_AXI_PCIE_CUR_INFO_159_128_MASK  0x00000001
#define MON_AXI_PCIE_CUR_INFO_W5_MON_AXI_PCIE_CUR_INFO_191_160_MASK  0x00000001
#define MON_AXI_PCIE_CUR_INFO_W6_MON_AXI_PCIE_CUR_INFO_223_192_MASK  0x00000001
#define MON_AXI_PCIE_CUR_INFO_W7_MON_AXI_PCIE_CUR_INFO_255_224_MASK  0x00000001
#define MON_AXI_PCIE_CUR_INFO_W8_MON_AXI_PCIE_CUR_INFO_287_256_MASK  0x00000001
#define MON_AXI_PCIE_CUR_INFO_W9_MON_AXI_PCIE_CUR_INFO_291_288_MASK  0x00000001

/* ################################################################################
 * # MonAxiPcieLogInfo Definition
 */
#define MON_AXI_PCIE_LOG_INFO_W0_MON_AXI_PCIE_LOG_INFO_31_0          BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W1_MON_AXI_PCIE_LOG_INFO_63_32         BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W2_MON_AXI_PCIE_LOG_INFO_95_64         BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W3_MON_AXI_PCIE_LOG_INFO_127_96        BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W4_MON_AXI_PCIE_LOG_INFO_159_128       BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W5_MON_AXI_PCIE_LOG_INFO_191_160       BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W6_MON_AXI_PCIE_LOG_INFO_223_192       BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W7_MON_AXI_PCIE_LOG_INFO_255_224       BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W8_MON_AXI_PCIE_LOG_INFO_287_256       BIT(0)
#define MON_AXI_PCIE_LOG_INFO_W9_MON_AXI_PCIE_LOG_INFO_291_288       BIT(0)

#define MON_AXI_PCIE_LOG_INFO_W0_MON_AXI_PCIE_LOG_INFO_31_0_MASK     0x00000001
#define MON_AXI_PCIE_LOG_INFO_W1_MON_AXI_PCIE_LOG_INFO_63_32_MASK    0x00000001
#define MON_AXI_PCIE_LOG_INFO_W2_MON_AXI_PCIE_LOG_INFO_95_64_MASK    0x00000001
#define MON_AXI_PCIE_LOG_INFO_W3_MON_AXI_PCIE_LOG_INFO_127_96_MASK   0x00000001
#define MON_AXI_PCIE_LOG_INFO_W4_MON_AXI_PCIE_LOG_INFO_159_128_MASK  0x00000001
#define MON_AXI_PCIE_LOG_INFO_W5_MON_AXI_PCIE_LOG_INFO_191_160_MASK  0x00000001
#define MON_AXI_PCIE_LOG_INFO_W6_MON_AXI_PCIE_LOG_INFO_223_192_MASK  0x00000001
#define MON_AXI_PCIE_LOG_INFO_W7_MON_AXI_PCIE_LOG_INFO_255_224_MASK  0x00000001
#define MON_AXI_PCIE_LOG_INFO_W8_MON_AXI_PCIE_LOG_INFO_287_256_MASK  0x00000001
#define MON_AXI_PCIE_LOG_INFO_W9_MON_AXI_PCIE_LOG_INFO_291_288_MASK  0x00000001

/* ################################################################################
 * # MonAxiQspiCurInfo Definition
 */
#define MON_AXI_QSPI_CUR_INFO_W0_MON_AXI_QSPI_CUR_INFO_31_0          BIT(0)
#define MON_AXI_QSPI_CUR_INFO_W1_MON_AXI_QSPI_CUR_INFO_63_32         BIT(0)
#define MON_AXI_QSPI_CUR_INFO_W2_MON_AXI_QSPI_CUR_INFO_95_64         BIT(0)
#define MON_AXI_QSPI_CUR_INFO_W3_MON_AXI_QSPI_CUR_INFO_127_96        BIT(0)
#define MON_AXI_QSPI_CUR_INFO_W4_MON_AXI_QSPI_CUR_INFO_159_128       BIT(0)
#define MON_AXI_QSPI_CUR_INFO_W5_MON_AXI_QSPI_CUR_INFO_191_160       BIT(0)
#define MON_AXI_QSPI_CUR_INFO_W6_MON_AXI_QSPI_CUR_INFO_223_192       BIT(0)

#define MON_AXI_QSPI_CUR_INFO_W0_MON_AXI_QSPI_CUR_INFO_31_0_MASK     0x00000001
#define MON_AXI_QSPI_CUR_INFO_W1_MON_AXI_QSPI_CUR_INFO_63_32_MASK    0x00000001
#define MON_AXI_QSPI_CUR_INFO_W2_MON_AXI_QSPI_CUR_INFO_95_64_MASK    0x00000001
#define MON_AXI_QSPI_CUR_INFO_W3_MON_AXI_QSPI_CUR_INFO_127_96_MASK   0x00000001
#define MON_AXI_QSPI_CUR_INFO_W4_MON_AXI_QSPI_CUR_INFO_159_128_MASK  0x00000001
#define MON_AXI_QSPI_CUR_INFO_W5_MON_AXI_QSPI_CUR_INFO_191_160_MASK  0x00000001
#define MON_AXI_QSPI_CUR_INFO_W6_MON_AXI_QSPI_CUR_INFO_223_192_MASK  0x00000001

/* ################################################################################
 * # MonAxiQspiLogInfo Definition
 */
#define MON_AXI_QSPI_LOG_INFO_W0_MON_AXI_QSPI_LOG_INFO_31_0          BIT(0)
#define MON_AXI_QSPI_LOG_INFO_W1_MON_AXI_QSPI_LOG_INFO_63_32         BIT(0)
#define MON_AXI_QSPI_LOG_INFO_W2_MON_AXI_QSPI_LOG_INFO_95_64         BIT(0)
#define MON_AXI_QSPI_LOG_INFO_W3_MON_AXI_QSPI_LOG_INFO_127_96        BIT(0)
#define MON_AXI_QSPI_LOG_INFO_W4_MON_AXI_QSPI_LOG_INFO_159_128       BIT(0)
#define MON_AXI_QSPI_LOG_INFO_W5_MON_AXI_QSPI_LOG_INFO_191_160       BIT(0)
#define MON_AXI_QSPI_LOG_INFO_W6_MON_AXI_QSPI_LOG_INFO_223_192       BIT(0)
#define MON_AXI_QSPI_LOG_INFO_W7_MON_AXI_QSPI_LOG_WD_ID              BIT(0)

#define MON_AXI_QSPI_LOG_INFO_W0_MON_AXI_QSPI_LOG_INFO_31_0_MASK     0x00000001
#define MON_AXI_QSPI_LOG_INFO_W1_MON_AXI_QSPI_LOG_INFO_63_32_MASK    0x00000001
#define MON_AXI_QSPI_LOG_INFO_W2_MON_AXI_QSPI_LOG_INFO_95_64_MASK    0x00000001
#define MON_AXI_QSPI_LOG_INFO_W3_MON_AXI_QSPI_LOG_INFO_127_96_MASK   0x00000001
#define MON_AXI_QSPI_LOG_INFO_W4_MON_AXI_QSPI_LOG_INFO_159_128_MASK  0x00000001
#define MON_AXI_QSPI_LOG_INFO_W5_MON_AXI_QSPI_LOG_INFO_191_160_MASK  0x00000001
#define MON_AXI_QSPI_LOG_INFO_W6_MON_AXI_QSPI_LOG_INFO_223_192_MASK  0x00000001
#define MON_AXI_QSPI_LOG_INFO_W7_MON_AXI_QSPI_LOG_WD_ID_MASK         0x000003ff

/* ################################################################################
 * # MonAxiSupCurInfo Definition
 */
#define MON_AXI_SUP_CUR_INFO_W0_MON_AXI_SUP_CUR_INFO_31_0            BIT(0)
#define MON_AXI_SUP_CUR_INFO_W1_MON_AXI_SUP_CUR_INFO_63_32           BIT(0)
#define MON_AXI_SUP_CUR_INFO_W2_MON_AXI_SUP_CUR_INFO_95_64           BIT(0)
#define MON_AXI_SUP_CUR_INFO_W3_MON_AXI_SUP_CUR_INFO_127_96          BIT(0)
#define MON_AXI_SUP_CUR_INFO_W4_MON_AXI_SUP_CUR_INFO_159_128         BIT(0)
#define MON_AXI_SUP_CUR_INFO_W5_MON_AXI_SUP_CUR_INFO_191_160         BIT(0)
#define MON_AXI_SUP_CUR_INFO_W6_MON_AXI_SUP_CUR_INFO_223_192         BIT(0)
#define MON_AXI_SUP_CUR_INFO_W7_MON_AXI_SUP_CUR_INFO_231_224         BIT(0)

#define MON_AXI_SUP_CUR_INFO_W0_MON_AXI_SUP_CUR_INFO_31_0_MASK       0x00000001
#define MON_AXI_SUP_CUR_INFO_W1_MON_AXI_SUP_CUR_INFO_63_32_MASK      0x00000001
#define MON_AXI_SUP_CUR_INFO_W2_MON_AXI_SUP_CUR_INFO_95_64_MASK      0x00000001
#define MON_AXI_SUP_CUR_INFO_W3_MON_AXI_SUP_CUR_INFO_127_96_MASK     0x00000001
#define MON_AXI_SUP_CUR_INFO_W4_MON_AXI_SUP_CUR_INFO_159_128_MASK    0x00000001
#define MON_AXI_SUP_CUR_INFO_W5_MON_AXI_SUP_CUR_INFO_191_160_MASK    0x00000001
#define MON_AXI_SUP_CUR_INFO_W6_MON_AXI_SUP_CUR_INFO_223_192_MASK    0x00000001
#define MON_AXI_SUP_CUR_INFO_W7_MON_AXI_SUP_CUR_INFO_231_224_MASK    0x00000001

/* ################################################################################
 * # MonAxiSupLogInfo Definition
 */
#define MON_AXI_SUP_LOG_INFO_W0_MON_AXI_SUP_LOG_INFO_31_0            BIT(0)
#define MON_AXI_SUP_LOG_INFO_W1_MON_AXI_SUP_LOG_INFO_63_32           BIT(0)
#define MON_AXI_SUP_LOG_INFO_W2_MON_AXI_SUP_LOG_INFO_95_64           BIT(0)
#define MON_AXI_SUP_LOG_INFO_W3_MON_AXI_SUP_LOG_INFO_127_96          BIT(0)
#define MON_AXI_SUP_LOG_INFO_W4_MON_AXI_SUP_LOG_INFO_159_128         BIT(0)
#define MON_AXI_SUP_LOG_INFO_W5_MON_AXI_SUP_LOG_INFO_191_160         BIT(0)
#define MON_AXI_SUP_LOG_INFO_W6_MON_AXI_SUP_LOG_INFO_223_192         BIT(0)
#define MON_AXI_SUP_LOG_INFO_W7_MON_AXI_SUP_LOG_INFO_231_224         BIT(0)

#define MON_AXI_SUP_LOG_INFO_W0_MON_AXI_SUP_LOG_INFO_31_0_MASK       0x00000001
#define MON_AXI_SUP_LOG_INFO_W1_MON_AXI_SUP_LOG_INFO_63_32_MASK      0x00000001
#define MON_AXI_SUP_LOG_INFO_W2_MON_AXI_SUP_LOG_INFO_95_64_MASK      0x00000001
#define MON_AXI_SUP_LOG_INFO_W3_MON_AXI_SUP_LOG_INFO_127_96_MASK     0x00000001
#define MON_AXI_SUP_LOG_INFO_W4_MON_AXI_SUP_LOG_INFO_159_128_MASK    0x00000001
#define MON_AXI_SUP_LOG_INFO_W5_MON_AXI_SUP_LOG_INFO_191_160_MASK    0x00000001
#define MON_AXI_SUP_LOG_INFO_W6_MON_AXI_SUP_LOG_INFO_223_192_MASK    0x00000001
#define MON_AXI_SUP_LOG_INFO_W7_MON_AXI_SUP_LOG_INFO_231_224_MASK    0x00000001

/* ################################################################################
 * # MonSysApbCurInfo Definition
 */
#define MON_SYS_APB_CUR_INFO_W0_MON_SYS_APB_CUR_ADDR                 BIT(0)
#define MON_SYS_APB_CUR_INFO_W1_MON_SYS_APB_CUR_WR_DATA              BIT(0)
#define MON_SYS_APB_CUR_INFO_W2_MON_SYS_APB_CUR_RD_DATA              BIT(0)
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_ENABLE               BIT(3)
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_WRITE                BIT(2)
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_READY                BIT(1)
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_SEL                  BIT(4)
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_SLV_ERR              BIT(0)

#define MON_SYS_APB_CUR_INFO_W0_MON_SYS_APB_CUR_ADDR_MASK            0xffffffff
#define MON_SYS_APB_CUR_INFO_W1_MON_SYS_APB_CUR_WR_DATA_MASK         0xffffffff
#define MON_SYS_APB_CUR_INFO_W2_MON_SYS_APB_CUR_RD_DATA_MASK         0xffffffff
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_ENABLE_MASK          0x00000008
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_WRITE_MASK           0x00000004
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_READY_MASK           0x00000002
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_SEL_MASK             0x00000010
#define MON_SYS_APB_CUR_INFO_W3_MON_SYS_APB_CUR_SLV_ERR_MASK         0x00000001

/* ################################################################################
 * # MonSysApbLogInfo Definition
 */
#define MON_SYS_APB_LOG_INFO_W0_MON_SYS_APB_LOG_ADDR                 BIT(0)
#define MON_SYS_APB_LOG_INFO_W1_MON_SYS_APB_LOG_WR_DATA              BIT(0)
#define MON_SYS_APB_LOG_INFO_W2_MON_SYS_APB_LOG_RD_DATA              BIT(0)
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_SEL                  BIT(4)
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_ENABLE               BIT(3)
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_READY                BIT(1)
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_ERR_CNT                  BIT(8)
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_SLV_ERR              BIT(0)
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_WRITE                BIT(2)

#define MON_SYS_APB_LOG_INFO_W0_MON_SYS_APB_LOG_ADDR_MASK            0xffffffff
#define MON_SYS_APB_LOG_INFO_W1_MON_SYS_APB_LOG_WR_DATA_MASK         0xffffffff
#define MON_SYS_APB_LOG_INFO_W2_MON_SYS_APB_LOG_RD_DATA_MASK         0xffffffff
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_SEL_MASK             0x00000010
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_ENABLE_MASK          0x00000008
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_READY_MASK           0x00000002
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_ERR_CNT_MASK             0x0000ff00
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_SLV_ERR_MASK         0x00000001
#define MON_SYS_APB_LOG_INFO_W3_MON_SYS_APB_LOG_WRITE_MASK           0x00000004

/* ################################################################################
 * # MonSecApbCurInfo Definition
 */
#define MON_SEC_APB_CUR_INFO_W0_MON_SEC_APB_CUR_ADDR                 BIT(0)
#define MON_SEC_APB_CUR_INFO_W1_MON_SEC_APB_CUR_WR_DATA              BIT(0)
#define MON_SEC_APB_CUR_INFO_W2_MON_SEC_APB_CUR_RD_DATA              BIT(0)
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_WRITE                BIT(2)
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_READY                BIT(1)
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_SLV_ERR              BIT(0)
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_ENABLE               BIT(3)
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_SEL                  BIT(4)

#define MON_SEC_APB_CUR_INFO_W0_MON_SEC_APB_CUR_ADDR_MASK            0xffffffff
#define MON_SEC_APB_CUR_INFO_W1_MON_SEC_APB_CUR_WR_DATA_MASK         0xffffffff
#define MON_SEC_APB_CUR_INFO_W2_MON_SEC_APB_CUR_RD_DATA_MASK         0xffffffff
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_WRITE_MASK           0x00000004
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_READY_MASK           0x00000002
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_SLV_ERR_MASK         0x00000001
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_ENABLE_MASK          0x00000008
#define MON_SEC_APB_CUR_INFO_W3_MON_SEC_APB_CUR_SEL_MASK             0x00000010

/* ################################################################################
 * # MonSecApbLogInfo Definition
 */
#define MON_SEC_APB_LOG_INFO_W0_MON_SEC_APB_LOG_ADDR                 BIT(0)
#define MON_SEC_APB_LOG_INFO_W1_MON_SEC_APB_LOG_WR_DATA              BIT(0)
#define MON_SEC_APB_LOG_INFO_W2_MON_SEC_APB_LOG_RD_DATA              BIT(0)
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_SLV_ERR              BIT(0)
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_WRITE                BIT(2)
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_READY                BIT(1)
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_ENABLE               BIT(3)
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_SEL                  BIT(4)
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_ERR_CNT                  BIT(8)

#define MON_SEC_APB_LOG_INFO_W0_MON_SEC_APB_LOG_ADDR_MASK            0xffffffff
#define MON_SEC_APB_LOG_INFO_W1_MON_SEC_APB_LOG_WR_DATA_MASK         0xffffffff
#define MON_SEC_APB_LOG_INFO_W2_MON_SEC_APB_LOG_RD_DATA_MASK         0xffffffff
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_SLV_ERR_MASK         0x00000001
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_WRITE_MASK           0x00000004
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_READY_MASK           0x00000002
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_ENABLE_MASK          0x00000008
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_LOG_SEL_MASK             0x00000010
#define MON_SEC_APB_LOG_INFO_W3_MON_SEC_APB_ERR_CNT_MASK             0x0000ff00

/* ################################################################################
 * # DebugCpuCnt Definition
 */
#define DEBUG_CPU_CNT_W0_MON_CPU_WA_LOG_CNT                          BIT(8)
#define DEBUG_CPU_CNT_W0_MON_CPU_RA_LOG_CNT                          BIT(0)
#define DEBUG_CPU_CNT_W0_MON_CPU_WD_LOG_CNT                          BIT(16)
#define DEBUG_CPU_CNT_W1_MON_CPU_WR_OUT_CNT                          BIT(8)
#define DEBUG_CPU_CNT_W1_MON_CPU_RD_OUT_CNT                          BIT(0)
#define DEBUG_CPU_CNT_W1_MON_CPU_WR_RESP_ERROR_CNT                   BIT(24)
#define DEBUG_CPU_CNT_W1_MON_CPU_RD_RESP_ERROR_CNT                   BIT(16)

#define DEBUG_CPU_CNT_W0_MON_CPU_WA_LOG_CNT_MASK                     0x0000ff00
#define DEBUG_CPU_CNT_W0_MON_CPU_RA_LOG_CNT_MASK                     0x000000ff
#define DEBUG_CPU_CNT_W0_MON_CPU_WD_LOG_CNT_MASK                     0x00ff0000
#define DEBUG_CPU_CNT_W1_MON_CPU_WR_OUT_CNT_MASK                     0x00001f00
#define DEBUG_CPU_CNT_W1_MON_CPU_RD_OUT_CNT_MASK                     0x0000001f
#define DEBUG_CPU_CNT_W1_MON_CPU_WR_RESP_ERROR_CNT_MASK              0xff000000
#define DEBUG_CPU_CNT_W1_MON_CPU_RD_RESP_ERROR_CNT_MASK              0x00ff0000

/* ################################################################################
 * # DebugMemCnt Definition
 */
#define DEBUG_MEM_CNT_W0_MON_MEM_WA_LOG_CNT                          BIT(8)
#define DEBUG_MEM_CNT_W0_MON_MEM_RA_LOG_CNT                          BIT(0)
#define DEBUG_MEM_CNT_W0_MON_MEM_WD_LOG_CNT                          BIT(16)
#define DEBUG_MEM_CNT_W1_MON_MEM_WR_OUT_CNT                          BIT(8)
#define DEBUG_MEM_CNT_W1_MON_MEM_RD_OUT_CNT                          BIT(0)

#define DEBUG_MEM_CNT_W0_MON_MEM_WA_LOG_CNT_MASK                     0x0000ff00
#define DEBUG_MEM_CNT_W0_MON_MEM_RA_LOG_CNT_MASK                     0x000000ff
#define DEBUG_MEM_CNT_W0_MON_MEM_WD_LOG_CNT_MASK                     0x00ff0000
#define DEBUG_MEM_CNT_W1_MON_MEM_WR_OUT_CNT_MASK                     0x00001f00
#define DEBUG_MEM_CNT_W1_MON_MEM_RD_OUT_CNT_MASK                     0x0000001f

/* ################################################################################
 * # DebugDdrCnt Definition
 */
#define DEBUG_DDR_CNT_W0_MON_DDR0_WA_LOG_CNT                         BIT(8)
#define DEBUG_DDR_CNT_W0_MON_DDR0_RA_LOG_CNT                         BIT(0)
#define DEBUG_DDR_CNT_W0_MON_DDR0_WD_LOG_CNT                         BIT(16)
#define DEBUG_DDR_CNT_W1_MON_DDR0_RD_OUT_CNT                         BIT(0)
#define DEBUG_DDR_CNT_W1_MON_DDR0_WR_RESP_ERROR_CNT                  BIT(24)
#define DEBUG_DDR_CNT_W1_MON_DDR0_RD_RESP_ERROR_CNT                  BIT(16)
#define DEBUG_DDR_CNT_W1_MON_DDR0_WR_OUT_CNT                         BIT(8)
#define DEBUG_DDR_CNT_W2_MON_DDR1_RA_LOG_CNT                         BIT(0)
#define DEBUG_DDR_CNT_W2_MON_DDR1_WD_LOG_CNT                         BIT(16)
#define DEBUG_DDR_CNT_W2_MON_DDR1_WA_LOG_CNT                         BIT(8)
#define DEBUG_DDR_CNT_W3_MON_DDR1_WR_OUT_CNT                         BIT(8)
#define DEBUG_DDR_CNT_W3_MON_DDR1_RD_RESP_ERROR_CNT                  BIT(16)
#define DEBUG_DDR_CNT_W3_MON_DDR1_RD_OUT_CNT                         BIT(0)
#define DEBUG_DDR_CNT_W3_MON_DDR1_WR_RESP_ERROR_CNT                  BIT(24)

#define DEBUG_DDR_CNT_W0_MON_DDR0_WA_LOG_CNT_MASK                    0x0000ff00
#define DEBUG_DDR_CNT_W0_MON_DDR0_RA_LOG_CNT_MASK                    0x000000ff
#define DEBUG_DDR_CNT_W0_MON_DDR0_WD_LOG_CNT_MASK                    0x00ff0000
#define DEBUG_DDR_CNT_W1_MON_DDR0_RD_OUT_CNT_MASK                    0x0000001f
#define DEBUG_DDR_CNT_W1_MON_DDR0_WR_RESP_ERROR_CNT_MASK             0xff000000
#define DEBUG_DDR_CNT_W1_MON_DDR0_RD_RESP_ERROR_CNT_MASK             0x00ff0000
#define DEBUG_DDR_CNT_W1_MON_DDR0_WR_OUT_CNT_MASK                    0x00001f00
#define DEBUG_DDR_CNT_W2_MON_DDR1_RA_LOG_CNT_MASK                    0x000000ff
#define DEBUG_DDR_CNT_W2_MON_DDR1_WD_LOG_CNT_MASK                    0x00ff0000
#define DEBUG_DDR_CNT_W2_MON_DDR1_WA_LOG_CNT_MASK                    0x0000ff00
#define DEBUG_DDR_CNT_W3_MON_DDR1_WR_OUT_CNT_MASK                    0x00001f00
#define DEBUG_DDR_CNT_W3_MON_DDR1_RD_RESP_ERROR_CNT_MASK             0x00ff0000
#define DEBUG_DDR_CNT_W3_MON_DDR1_RD_OUT_CNT_MASK                    0x0000001f
#define DEBUG_DDR_CNT_W3_MON_DDR1_WR_RESP_ERROR_CNT_MASK             0xff000000

/* ################################################################################
 * # DebugMshCnt Definition
 */
#define DEBUG_MSH_CNT_W0_MON_MSH_WD_LOG_CNT                          BIT(16)
#define DEBUG_MSH_CNT_W0_MON_MSH_RA_LOG_CNT                          BIT(0)
#define DEBUG_MSH_CNT_W0_MON_MSH_WA_LOG_CNT                          BIT(8)
#define DEBUG_MSH_CNT_W1_MON_MSH_RD_OUT_CNT                          BIT(0)
#define DEBUG_MSH_CNT_W1_MON_MSH_WR_OUT_CNT                          BIT(8)

#define DEBUG_MSH_CNT_W0_MON_MSH_WD_LOG_CNT_MASK                     0x00ff0000
#define DEBUG_MSH_CNT_W0_MON_MSH_RA_LOG_CNT_MASK                     0x000000ff
#define DEBUG_MSH_CNT_W0_MON_MSH_WA_LOG_CNT_MASK                     0x0000ff00
#define DEBUG_MSH_CNT_W1_MON_MSH_RD_OUT_CNT_MASK                     0x0000001f
#define DEBUG_MSH_CNT_W1_MON_MSH_WR_OUT_CNT_MASK                     0x00001f00

/* ################################################################################
 * # DebugPcieCnt Definition
 */
#define DEBUG_PCIE_CNT_W0_MON_PCIE_WD_LOG_CNT                        BIT(16)
#define DEBUG_PCIE_CNT_W0_MON_PCIE_RA_LOG_CNT                        BIT(0)
#define DEBUG_PCIE_CNT_W0_MON_PCIE_WA_LOG_CNT                        BIT(8)
#define DEBUG_PCIE_CNT_W1_MON_PCIE_WR_OUT_CNT                        BIT(8)
#define DEBUG_PCIE_CNT_W1_MON_PCIE_RD_OUT_CNT                        BIT(0)
#define DEBUG_PCIE_CNT_W1_MON_PCIE_WR_RESP_ERROR_CNT                 BIT(24)
#define DEBUG_PCIE_CNT_W1_MON_PCIE_RD_RESP_ERROR_CNT                 BIT(16)

#define DEBUG_PCIE_CNT_W0_MON_PCIE_WD_LOG_CNT_MASK                   0x00ff0000
#define DEBUG_PCIE_CNT_W0_MON_PCIE_RA_LOG_CNT_MASK                   0x000000ff
#define DEBUG_PCIE_CNT_W0_MON_PCIE_WA_LOG_CNT_MASK                   0x0000ff00
#define DEBUG_PCIE_CNT_W1_MON_PCIE_WR_OUT_CNT_MASK                   0x00001f00
#define DEBUG_PCIE_CNT_W1_MON_PCIE_RD_OUT_CNT_MASK                   0x0000001f
#define DEBUG_PCIE_CNT_W1_MON_PCIE_WR_RESP_ERROR_CNT_MASK            0xff000000
#define DEBUG_PCIE_CNT_W1_MON_PCIE_RD_RESP_ERROR_CNT_MASK            0x00ff0000

/* ################################################################################
 * # DebugQspiCnt Definition
 */
#define DEBUG_QSPI_CNT_W0_MON_QSPI_WD_LOG_CNT                        BIT(16)
#define DEBUG_QSPI_CNT_W0_MON_QSPI_RA_LOG_CNT                        BIT(0)
#define DEBUG_QSPI_CNT_W0_MON_QSPI_WA_LOG_CNT                        BIT(8)
#define DEBUG_QSPI_CNT_W1_MON_QSPI_RD_OUT_CNT                        BIT(0)
#define DEBUG_QSPI_CNT_W1_MON_QSPI_WR_OUT_CNT                        BIT(8)

#define DEBUG_QSPI_CNT_W0_MON_QSPI_WD_LOG_CNT_MASK                   0x00ff0000
#define DEBUG_QSPI_CNT_W0_MON_QSPI_RA_LOG_CNT_MASK                   0x000000ff
#define DEBUG_QSPI_CNT_W0_MON_QSPI_WA_LOG_CNT_MASK                   0x0000ff00
#define DEBUG_QSPI_CNT_W1_MON_QSPI_RD_OUT_CNT_MASK                   0x0000001f
#define DEBUG_QSPI_CNT_W1_MON_QSPI_WR_OUT_CNT_MASK                   0x00001f00

/* ################################################################################
 * # DebugSupCnt Definition
 */
#define DEBUG_SUP_CNT_W0_MON_SUP_WD_LOG_CNT                          BIT(16)
#define DEBUG_SUP_CNT_W0_MON_SUP_RA_LOG_CNT                          BIT(0)
#define DEBUG_SUP_CNT_W0_MON_SUP_WA_LOG_CNT                          BIT(8)
#define DEBUG_SUP_CNT_W1_MON_SUP_WR_OUT_CNT                          BIT(8)
#define DEBUG_SUP_CNT_W1_MON_SUP_RD_RESP_ERROR_CNT                   BIT(16)
#define DEBUG_SUP_CNT_W1_MON_SUP_WR_RESP_ERROR_CNT                   BIT(24)
#define DEBUG_SUP_CNT_W1_MON_SUP_RD_OUT_CNT                          BIT(0)

#define DEBUG_SUP_CNT_W0_MON_SUP_WD_LOG_CNT_MASK                     0x00ff0000
#define DEBUG_SUP_CNT_W0_MON_SUP_RA_LOG_CNT_MASK                     0x000000ff
#define DEBUG_SUP_CNT_W0_MON_SUP_WA_LOG_CNT_MASK                     0x0000ff00
#define DEBUG_SUP_CNT_W1_MON_SUP_WR_OUT_CNT_MASK                     0x00001f00
#define DEBUG_SUP_CNT_W1_MON_SUP_RD_RESP_ERROR_CNT_MASK              0x00ff0000
#define DEBUG_SUP_CNT_W1_MON_SUP_WR_RESP_ERROR_CNT_MASK              0xff000000
#define DEBUG_SUP_CNT_W1_MON_SUP_RD_OUT_CNT_MASK                     0x0000001f

/* ################################################################################
 * # SysSpiVecLog Definition
 */
#define SYS_SPI_VEC_LOG_W0_SPI_VEC_LOG_31_0                          BIT(0)
#define SYS_SPI_VEC_LOG_W1_SPI_VEC_LOG_63_32                         BIT(0)
#define SYS_SPI_VEC_LOG_W2_SPI_VEC_LOG_95_64                         BIT(0)
#define SYS_SPI_VEC_LOG_W3_SPI_VEC_LOG_127_96                        BIT(0)
#define SYS_SPI_VEC_LOG_W4_SPI_VEC_LOG_159_128                       BIT(0)

#define SYS_SPI_VEC_LOG_W0_SPI_VEC_LOG_31_0_MASK                     0x00000001
#define SYS_SPI_VEC_LOG_W1_SPI_VEC_LOG_63_32_MASK                    0x00000001
#define SYS_SPI_VEC_LOG_W2_SPI_VEC_LOG_95_64_MASK                    0x00000001
#define SYS_SPI_VEC_LOG_W3_SPI_VEC_LOG_127_96_MASK                   0x00000001
#define SYS_SPI_VEC_LOG_W4_SPI_VEC_LOG_159_128_MASK                  0x00000001

/* ################################################################################
 * # DebugAhbRespCnt Definition
 */
#define DEBUG_AHB_RESP_CNT_W0_MON_MSH_CFG_RESP_ERROR_CNT             BIT(8)
#define DEBUG_AHB_RESP_CNT_W0_MON_DDR_CFG_RESP_ERROR_CNT             BIT(0)
#define DEBUG_AHB_RESP_CNT_W0_MON_USB_CFG_RESP_ERROR_CNT             BIT(16)

#define DEBUG_AHB_RESP_CNT_W0_MON_MSH_CFG_RESP_ERROR_CNT_MASK        0x0000ff00
#define DEBUG_AHB_RESP_CNT_W0_MON_DDR_CFG_RESP_ERROR_CNT_MASK        0x000000ff
#define DEBUG_AHB_RESP_CNT_W0_MON_USB_CFG_RESP_ERROR_CNT_MASK        0x00ff0000

/* ################################################################################
 * # DebugGicRespCnt Definition
 */
#define DEBUG_GIC_RESP_CNT_W0_MON_GIC_RD_RESP_ERROR_CNT              BIT(0)
#define DEBUG_GIC_RESP_CNT_W0_MON_GIC_WR_RESP_ERROR_CNT              BIT(8)

#define DEBUG_GIC_RESP_CNT_W0_MON_GIC_RD_RESP_ERROR_CNT_MASK         0x000000ff
#define DEBUG_GIC_RESP_CNT_W0_MON_GIC_WR_RESP_ERROR_CNT_MASK         0x0000ff00

/* ################################################################################ 
 * # SysDdrEccCtl Definition 
 */
#define SYS_DDR_ECC_CTL_W0_DDR_ECC_CLEAR_EN                      	 BIT(0)
#define SYS_DDR_ECC_CTL_W0_DDR_ECC_ONE_BIT_ERR_CNT                   BIT(8)
#define SYS_DDR_ECC_CTL_W0_DDR_ECC_TWO_BIT_ERR_CNT               	 BIT(12)

#define SYS_DDR_ECC_CTL_W0_DDR_ECC_CLEAR_EN_MASK                     0x00000001
#define SYS_DDR_ECC_CTL_W0_DDR_ECC_ONE_BIT_ERR_CNT_MASK              0x00000f00
#define SYS_DDR_ECC_CTL_W0_DDR_ECC_TWO_BIT_ERR_CNT_MASK              0x0000f000

/* ################################################################################ 
 * # SysDdrInitStartAddr Definition
 */
#define SYS_DDR_INIT_START_ADDR_W0_CFG_INIT_START_ADDR0          	 BIT(0)
#define SYS_DDR_INIT_START_ADDR_W1_CFG_INIT_START_ADDR1          	 BIT(0)

#define SYS_DDR_INIT_START_ADDR_W0_CFG_INIT_START_ADDR0_MASK         0xffffffff
#define SYS_DDR_INIT_START_ADDR_W1_CFG_INIT_START_ADDR1_MASK         0x0000000f

/* ################################################################################ 
 * # SysDdrInitLastAddr Definition 
 */
#define SYS_DDR_INIT_LAST_ADDR_W0_CFG_INIT_LAST_ADDR0           	 BIT(0)
#define SYS_DDR_INIT_LAST_ADDR_W1_CFG_INIT_LAST_ADDR1            	 BIT(0)

#define SYS_DDR_INIT_LAST_ADDR_W0_CFG_INIT_LAST_ADDR0_MASK           0xffffffff
#define SYS_DDR_INIT_LAST_ADDR_W1_CFG_INIT_LAST_ADDR1_MASK           0x0000000f

/* ################################################################################ 
 * # SysDdrInitData Definition
 */
#define SYS_DDR_INIT_DATA_W0_CFG_INIT_WR_DATA0                   	 BIT(0)
#define SYS_DDR_INIT_DATA_W1_CFG_INIT_WR_DATA1                       BIT(0)

#define SYS_DDR_INIT_DATA_W0_CFG_INIT_WR_DATA0_MASK                  0xffffffff
#define SYS_DDR_INIT_DATA_W1_CFG_INIT_WR_DATA1_MASK                  0xffffffff

/* ################################################################################ 
 * # SysDdrInitMode Definition */
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_CRITICAL                   BIT(0)
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_LEN                    	 BIT(8)
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_QOS                        BIT(4)
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_STRB                       BIT(16)
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_THRD                   	 BIT(24)

#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_CRITICAL_MASK              0x00000001
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_LEN_MASK                   0x0000ff00
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_QOS_MASK                   0x000000f0
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_STRB_MASK                  0x00ff0000
#define SYS_DDR_INIT_MODE_W0_CFG_INIT_AXI_THRD_MASK                  0x0f000000

/* ################################################################################ 
 * # SysDdrInitCtl Definition 
 */
#define SYS_DDR_INIT_CTL_W0_CFG_DDR_INIT_EN                          BIT(0)
#define SYS_DDR_INIT_CTL_W0_LOG_DDR_INIT_DONE                  	     BIT(4)
#define SYS_DDR_INIT_CTL_W0_LOG_DDR_INIT_RESP                        BIT(8)

#define SYS_DDR_INIT_CTL_W0_CFG_DDR_INIT_EN_MASK                     0x00000001
#define SYS_DDR_INIT_CTL_W0_LOG_DDR_INIT_DONE_MASK                   0x00000010
#define SYS_DDR_INIT_CTL_W0_LOG_DDR_INIT_RESP_MASK                   0x00000300

/* ################################################################################
 * # DebugMemPtrCfg Definition
 */
#define DEBUG_MEM_PTR_CFG_W0_CFG_DBG_START_PTR                       BIT(0)
#define DEBUG_MEM_PTR_CFG_W0_CFG_DBG_END_PTR                         BIT(16)

#define DEBUG_MEM_PTR_CFG_W0_CFG_DBG_START_PTR_MASK                  0x00003fff
#define DEBUG_MEM_PTR_CFG_W0_CFG_DBG_END_PTR_MASK                    0x3fff0000

/* ################################################################################
 * # Grant0IntMask Definition
 */
#define GRANT0_INT_MASK_W0_GRANT0_INT_MASK                           BIT(0)

#define GRANT0_INT_MASK_W0_GRANT0_INT_MASK_MASK                      0xffffffff

/* ################################################################################
 * # Grant0IntCtl Definition
 */
#define GRANT0_INT_CTL_W0_GRANT0_INT_CTL                             BIT(0)

#define GRANT0_INT_CTL_W0_GRANT0_INT_CTL_MASK                        0xffffffff

/* ################################################################################
 * # Grant1IntMask Definition
 */
#define GRANT1_INT_MASK_W0_GRANT1_INT_MASK                           BIT(0)

#define GRANT1_INT_MASK_W0_GRANT1_INT_MASK_MASK                      0xffffffff

/* ################################################################################
 * # Grant1IntCtl Definition
 */
#define GRANT1_INT_CTL_W0_GRANT1_INT_CTL                             BIT(0)

#define GRANT1_INT_CTL_W0_GRANT1_INT_CTL_MASK                        0xffffffff

/* ################################################################################
 * # Grant2IntMask Definition
 */
#define GRANT2_INT_MASK_W0_GRANT2_INT_MASK                           BIT(0)

#define GRANT2_INT_MASK_W0_GRANT2_INT_MASK_MASK                      0xffffffff

/* ################################################################################
 * # Grant2IntCtl Definition
 */
#define GRANT2_INT_CTL_W0_GRANT2_INT_CTL                             BIT(0)

#define GRANT2_INT_CTL_W0_GRANT2_INT_CTL_MASK                        0xffffffff

/* ################################################################################
 * # Grant3IntMask Definition
 */
#define GRANT3_INT_MASK_W0_GRANT3_INT_MASK                           BIT(0)

#define GRANT3_INT_MASK_W0_GRANT3_INT_MASK_MASK                      0xffffffff

/* ################################################################################
 * # Grant3IntCtl Definition
 */
#define GRANT3_INT_CTL_W0_GRANT3_INT_CTL                             BIT(0)

#define GRANT3_INT_CTL_W0_GRANT3_INT_CTL_MASK                        0xffffffff

/* ################################################################################
 * # SysIntrIntCtl Definition
 */
#define SYS_INTR_INT_CTL_W0_SYS_INTR_INT_CTL_31_0                    BIT(0)
#define SYS_INTR_INT_CTL_W1_SYS_INTR_INT_CTL_63_32                   BIT(0)

#define SYS_INTR_INT_CTL_W0_SYS_INTR_INT_CTL_31_0_MASK               0x00000001
#define SYS_INTR_INT_CTL_W1_SYS_INTR_INT_CTL_63_32_MASK              0x00000001

/* ################################################################################
 * # SupIntrIntCtl Definition
 */
#define SUP_INTR_INT_CTL_W0_SUP_INTR_INT_CTL_31_0                    BIT(0)
#define SUP_INTR_INT_CTL_W1_SUP_INTR_INT_CTL_63_32                   BIT(0)

#define SUP_INTR_INT_CTL_W0_SUP_INTR_INT_CTL_31_0_MASK               0x00000001
#define SUP_INTR_INT_CTL_W1_SUP_INTR_INT_CTL_63_32_MASK              0x00000001

/* ################################################################################
 * # SysMiscInfo0 Definition
 */
#define SYS_MISC_INFO0_W0_SYS_MISC_INFO0                             BIT(0)

#define SYS_MISC_INFO0_W0_SYS_MISC_INFO0_MASK                        0xffffffff

/* ################################################################################
 * # SysMiscInfo1 Definition
 */
#define SYS_MISC_INFO1_W0_SYS_MISC_INFO1                             BIT(0)

#define SYS_MISC_INFO1_W0_SYS_MISC_INFO1_MASK                        0xffffffff

/* ################################################################################
 * # SysMiscInfo2 Definition
 */
#define SYS_MISC_INFO2_W0_SYS_MISC_INFO2                             BIT(0)

#define SYS_MISC_INFO2_W0_SYS_MISC_INFO2_MASK                        0xffffffff

/* ################################################################################
 * # SysMiscInfo3 Definition
 */
#define SYS_MISC_INFO3_W0_SYS_MISC_INFO3                             BIT(0)

#define SYS_MISC_INFO3_W0_SYS_MISC_INFO3_MASK                        0xffffffff

/* ################################################################################
 * # CommonInfo0 Definition
 */
#define COMMON_INFO0_W0_COMMON_INFO0                                 BIT(0)

#define COMMON_INFO0_W0_COMMON_INFO0_MASK                            0xffffffff

/* ################################################################################
 * # CommonInfo1 Definition
 */
#define COMMON_INFO1_W0_COMMON_INFO1                                 BIT(0)

#define COMMON_INFO1_W0_COMMON_INFO1_MASK                            0xffffffff

/* ################################################################################
 * # CommonInfo2 Definition
 */
#define COMMON_INFO2_W0_COMMON_INFO2                                 BIT(0)

#define COMMON_INFO2_W0_COMMON_INFO2_MASK                            0xffffffff

/* ################################################################################
 * # CommonInfo3 Definition
 */
#define COMMON_INFO3_W0_COMMON_INFO3                                 BIT(0)

#define COMMON_INFO3_W0_COMMON_INFO3_MASK                            0xffffffff

/* ################################################################################
 * # Grant0ExtMask Definition
 */
#define GRANT0_EXT_MASK_W0_GRANT0_EXT_MASK                           BIT(0)

#define GRANT0_EXT_MASK_W0_GRANT0_EXT_MASK_MASK                      0xffffffff

/* ################################################################################
 * # Grant0ExtCtl Definition
 */
#define GRANT0_EXT_CTL_W0_GRANT0_EXT_CTL                             BIT(0)

#define GRANT0_EXT_CTL_W0_GRANT0_EXT_CTL_MASK                        0xffffffff

/* ################################################################################
 * # Grant1ExtMask Definition
 */
#define GRANT1_EXT_MASK_W0_GRANT1_EXT_MASK                           BIT(0)

#define GRANT1_EXT_MASK_W0_GRANT1_EXT_MASK_MASK                      0xffffffff

/* ################################################################################
 * # Grant1ExtCtl Definition
 */
#define GRANT1_EXT_CTL_W0_GRANT1_EXT_CTL                             BIT(0)

#define GRANT1_EXT_CTL_W0_GRANT1_EXT_CTL_MASK                        0xffffffff

/* ################################################################################
 * # Grant2ExtMask Definition
 */
#define GRANT2_EXT_MASK_W0_GRANT2_EXT_MASK                           BIT(0)

#define GRANT2_EXT_MASK_W0_GRANT2_EXT_MASK_MASK                      0xffffffff

/* ################################################################################
 * # Grant2ExtCtl Definition
 */
#define GRANT2_EXT_CTL_W0_GRANT2_EXT_CTL                             BIT(0)

#define GRANT2_EXT_CTL_W0_GRANT2_EXT_CTL_MASK                        0xffffffff

/* ################################################################################
 * # Grant3ExtMask Definition
 */
#define GRANT3_EXT_MASK_W0_GRANT3_EXT_MASK                           BIT(0)

#define GRANT3_EXT_MASK_W0_GRANT3_EXT_MASK_MASK                      0xffffffff

/* ################################################################################
 * # Grant3ExtCtl Definition
 */
#define GRANT3_EXT_CTL_W0_GRANT3_EXT_CTL                             BIT(0)

#define GRANT3_EXT_CTL_W0_GRANT3_EXT_CTL_MASK                        0xffffffff

/* ################################################################################
 * # SysIntrExtCtl Definition
 */
#define SYS_INTR_EXT_CTL_W0_SYS_INTR_EXT_CTL_31_0                    BIT(0)
#define SYS_INTR_EXT_CTL_W1_SYS_INTR_EXT_CTL_63_32                   BIT(0)

#define SYS_INTR_EXT_CTL_W0_SYS_INTR_EXT_CTL_31_0_MASK               0x00000001
#define SYS_INTR_EXT_CTL_W1_SYS_INTR_EXT_CTL_63_32_MASK              0x00000001

/* ################################################################################
 * # SupIntrExtCtl Definition
 */
#define SUP_INTR_EXT_CTL_W0_SUP_INTR_EXT_CTL_31_0                    BIT(0)
#define SUP_INTR_EXT_CTL_W1_SUP_INTR_EXT_CTL_63_32                   BIT(0)

#define SUP_INTR_EXT_CTL_W0_SUP_INTR_EXT_CTL_31_0_MASK               0x00000001
#define SUP_INTR_EXT_CTL_W1_SUP_INTR_EXT_CTL_63_32_MASK              0x00000001

/* ################################################################################
 * # SupMiscInfo0 Definition
 */
#define SUP_MISC_INFO0_W0_SUP_MISC_INFO0                             BIT(0)

#define SUP_MISC_INFO0_W0_SUP_MISC_INFO0_MASK                        0xffffffff

/* ################################################################################
 * # SupMiscInfo1 Definition
 */
#define SUP_MISC_INFO1_W0_SUP_MISC_INFO1                             BIT(0)

#define SUP_MISC_INFO1_W0_SUP_MISC_INFO1_MASK                        0xffffffff

/* ################################################################################
 * # SupMiscInfo2 Definition
 */
#define SUP_MISC_INFO2_W0_SUP_MISC_INFO2                             BIT(0)

#define SUP_MISC_INFO2_W0_SUP_MISC_INFO2_MASK                        0xffffffff

/* ################################################################################
 * # SupMiscInfo3 Definition
 */
#define SUP_MISC_INFO3_W0_SUP_MISC_INFO3                             BIT(0)

#define SUP_MISC_INFO3_W0_SUP_MISC_INFO3_MASK                        0xffffffff

/* ################################################################################ 
 * # SysBusDbgEn Definition
 */
#define SYS_BUS_DBG_EN_W0_CFG_DBG_EN_VEC0                            BIT(0)
#define SYS_BUS_DBG_EN_W1_CFG_DBG_EN_VEC1                            BIT(0)

#define SYS_BUS_DBG_EN_W0_CFG_DBG_EN_VEC0_MASK                       0xffffffff
#define SYS_BUS_DBG_EN_W1_CFG_DBG_EN_VEC1_MASK                       0x001fffff

/* ################################################################################ 
 * # SysApbErrLog Definition 
 */
#define SYS_APB_ERR_LOG_W0_APB_SYS_ERR_LOG                           BIT(0)

#define SYS_APB_ERR_LOG_W0_APB_SYS_ERR_LOG_MASK                      0x00000fff

/* ################################################################################ 
 * # MshClkPadSchmitEn Definition 
 */
#define MSH_CLK_PAD_SCHMIT_EN_W0_MSH_CLK_PAD_SCHMIT_EN               BIT(0)

#define MSH_CLK_PAD_SCHMIT_EN_W0_MSH_CLK_PAD_SCHMIT_EN_MASK          0x00000001

/* ################################################################################ 
 * # SysI2C0DebugStatus Definition 
 */
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_ADDR10_BIT               BIT(6)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_ADDR_PROC                BIT(0)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_DATA_PROC                BIT(1)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_MASTER_ACT               BIT(7)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_MASTER_STATE             BIT(8)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_RD_PROC                  BIT(2)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_SLAVE_ACT                BIT(20)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_SLAVE_STATE              BIT(16)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_START_GEN                BIT(4)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_STOP_GEN                 BIT(5)
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_WR_PROC                  BIT(3)

#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_ADDR10_BIT_MASK          0x00000040
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_ADDR_PROC_MASK           0x00000001
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_DATA_PROC_MASK           0x00000002
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_MASTER_ACT_MASK          0x00000080
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_MASTER_STATE_MASK        0x00001f00
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_RD_PROC_MASK             0x00000004
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_SLAVE_ACT_MASK           0x00100000
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_SLAVE_STATE_MASK         0x000f0000
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_START_GEN_MASK           0x00000010
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_STOP_GEN_MASK            0x00000020
#define SYS_I2_C0_DEBUG_STATUS_W0_DBG_I2_C0_WR_PROC_MASK             0x00000008

/* ################################################################################
 * # SysI2C1DebugStatus Definition 
 */
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_ADDR10_BIT               BIT(6)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_ADDR_PROC                BIT(0)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_DATA_PROC                BIT(1)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_MASTER_ACT               BIT(7)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_MASTER_STATE        	 BIT(8)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_RD_PROC                  BIT(2)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_SLAVE_ACT                BIT(20)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_SLAVE_STATE              BIT(16)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_START_GEN                BIT(4)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_STOP_GEN                 BIT(5)
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_WR_PROC                  BIT(3)

#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_ADDR10_BIT_MASK          0x00000040
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_ADDR_PROC_MASK           0x00000001
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_DATA_PROC_MASK           0x00000002
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_MASTER_ACT_MASK          0x00000080
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_MASTER_STATE_MASK        0x00001f00
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_RD_PROC_MASK             0x00000004
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_SLAVE_ACT_MASK           0x00100000
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_SLAVE_STATE_MASK         0x000f0000
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_START_GEN_MASK           0x00000010
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_STOP_GEN_MASK            0x00000020
#define SYS_I2_C1_DEBUG_STATUS_W0_DBG_I2_C1_WR_PROC_MASK             0x00000008

struct SysCtl_mems {
	u32 SysApbMem0[3];	/* 0x0000fe00 */
	u32 SysApbMem0_rsv3;
	u32 SysApbMem1[3];	/* 0x0000fe10 */
	u32 SysApbMem1_rsv3;
	u32 SysApbMem2[3];	/* 0x0000fe20 */
	u32 SysApbMem2_rsv3;
	u32 SysApbMem3[3];	/* 0x0000fe30 */
	u32 SysApbMem3_rsv3;
	u32 SysApbMem4[3];	/* 0x0000fe40 */
	u32 SysApbMem4_rsv3;
	u32 SysApbMem5[3];	/* 0x0000fe50 */
	u32 SysApbMem5_rsv3;
	u32 SysApbMem6[3];	/* 0x0000fe60 */
	u32 SysApbMem6_rsv3;
	u32 SysApbMem7[3];	/* 0x0000fe70 */
	u32 SysApbMem7_rsv3;
	u32 SysApbMem8[3];	/* 0x0000fe80 */
	u32 SysApbMem8_rsv3;
	u32 SysApbMem9[3];	/* 0x0000fe90 */
	u32 SysApbMem9_rsv3;
	u32 SysApbMem10[3];	/* 0x0000fea0 */
	u32 SysApbMem10_rsv3;
	u32 SysApbMem11[3];	/* 0x0000feb0 */
	u32 SysApbMem11_rsv3;
	u32 SysApbMem12[3];	/* 0x0000fec0 */
	u32 SysApbMem12_rsv3;
	u32 SysApbMem13[3];	/* 0x0000fed0 */
	u32 SysApbMem13_rsv3;
	u32 SysApbMem14[3];	/* 0x0000fee0 */
	u32 SysApbMem14_rsv3;
	u32 SysApbMem15[3];	/* 0x0000fef0 */
	u32 SysApbMem15_rsv3;
	u32 SecApbMem0[3];	/* 0x0000ff00 */
	u32 SecApbMem0_rsv3;
	u32 SecApbMem1[3];	/* 0x0000ff10 */
	u32 SecApbMem1_rsv3;
	u32 SecApbMem2[3];	/* 0x0000ff20 */
	u32 SecApbMem2_rsv3;
	u32 SecApbMem3[3];	/* 0x0000ff30 */
	u32 SecApbMem3_rsv3;
	u32 SecApbMem4[3];	/* 0x0000ff40 */
	u32 SecApbMem4_rsv3;
	u32 SecApbMem5[3];	/* 0x0000ff50 */
	u32 SecApbMem5_rsv3;
	u32 SecApbMem6[3];	/* 0x0000ff60 */
	u32 SecApbMem6_rsv3;
	u32 SecApbMem7[3];	/* 0x0000ff70 */
	u32 SecApbMem7_rsv3;
	u32 SecApbMem8[3];	/* 0x0000ff80 */
	u32 SecApbMem8_rsv3;
	u32 SecApbMem9[3];	/* 0x0000ff90 */
	u32 SecApbMem9_rsv3;
	u32 SecApbMem10[3];	/* 0x0000ffa0 */
	u32 SecApbMem10_rsv3;
	u32 SecApbMem11[3];	/* 0x0000ffb0 */
	u32 SecApbMem11_rsv3;
	u32 SecApbMem12[3];	/* 0x0000ffc0 */
	u32 SecApbMem12_rsv3;
	u32 SecApbMem13[3];	/* 0x0000ffd0 */
	u32 SecApbMem13_rsv3;
	u32 SecApbMem14[3];	/* 0x0000ffe0 */
	u32 SecApbMem14_rsv3;
	u32 SecApbMem15[3];	/* 0x0000fff0 */
	u32 SecApbMem15_rsv3;
};

/* ################################################################################
 * # SysApbMem Definition
 */
#define SYS_APB_MEM_W0_ADDR                                          BIT(0)
#define SYS_APB_MEM_W1_DATA                                          BIT(0)
#define SYS_APB_MEM_W2_WR_EN                                         BIT(0)
#define SYS_APB_MEM_W2_SLV_ERR                                       BIT(1)

#define SYS_APB_MEM_W0_ADDR_MASK                                     0xffffffff
#define SYS_APB_MEM_W1_DATA_MASK                                     0xffffffff
#define SYS_APB_MEM_W2_WR_EN_MASK                                    0x00000001
#define SYS_APB_MEM_W2_SLV_ERR_MASK                                  0x00000002

/* ################################################################################
 * # SecApbMem Definition
 */
#define SEC_APB_MEM_W0_ADDR                                          BIT(0)
#define SEC_APB_MEM_W1_DATA                                          BIT(0)
#define SEC_APB_MEM_W2_WR_EN                                         BIT(0)
#define SEC_APB_MEM_W2_SLV_ERR                                       BIT(1)

#define SEC_APB_MEM_W0_ADDR_MASK                                     0xffffffff
#define SEC_APB_MEM_W1_DATA_MASK                                     0xffffffff
#define SEC_APB_MEM_W2_WR_EN_MASK                                    0x00000001
#define SEC_APB_MEM_W2_SLV_ERR_MASK                                  0x00000002

/* ################################################################################ 
 * # SysPcieMem Definition 
 */
#define SYS_PCIE_MEM_W0_DATA_BIT                                     BIT(0)

#define SYS_PCIE_MEM_W0_DATA_MASK                                    0xffffffff

/* Bootmode setting values */
#define ROM_QSPI_MODE		0x00000000
#define ROM_EMMC_MODE		0x00000001
#define ROM_SD_MODE		0x00000002
#define ROM_UART_MODE		0x00000003
#define NON_ROM_MODE		0x00000004
#define JTAG_MODE		0x00000005
#define ROM_QSPI_DEBUG_MODE	0x00000007

#define BOOT_MODES_MASK		0x00000007

#define sysctl_base ((struct SysCtl_regs *)CONFIG_SYS_CTC5236_SYSCTL_BASE)
#endif

#endif /*__CTC5236_SYSCTL_H__*/
