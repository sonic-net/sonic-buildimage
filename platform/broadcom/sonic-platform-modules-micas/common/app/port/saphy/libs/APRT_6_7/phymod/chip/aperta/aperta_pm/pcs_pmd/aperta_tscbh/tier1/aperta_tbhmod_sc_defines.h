#ifndef aperta_tefmod_sc_defines_H_
#define aperta_tefmod_sc_defines_H_


#define T_PMA_BITMUX_MODE_1to1   0
#define T_PMA_BITMUX_MODE_2to1   1
#define T_PMA_BITMUX_MODE_5to1   2

#define T_SCR_MODE_BYPASS        0
#define T_SCR_MODE_CL49          1
#define T_SCR_MODE_40G_2_LANE    2
#define T_SCR_MODE_100G          3
#define T_SCR_MODE_20G           4
#define T_SCR_MODE_40G_4_LANE    5

#define R_DESCR_MODE_BYPASS      0
#define R_DESCR_MODE_CL49        1
#define R_DESCR_MODE_CL82        2

#define R_DEC_TL_MODE_BYPASS     0
#define R_DEC_TL_MODE_CL49       1
#define R_DEC_TL_MODE_CL82       2

#define R_DEC_FSM_MODE_BYPASS    0
#define R_DEC_FSM_MODE_CL49      1
#define R_DEC_FSM_MODE_CL82      2

#define BS_DIST_MODE_5_LANE_TDM          0
#define BS_DIST_MODE_2_LANE_TDM_2_VLANE  1
#define BS_DIST_MODE_2_LANE_TDM_1_VLANE  2
#define BS_DIST_MODE_NO_TDM              3

#define BS_BTMX_MODE_1to1        0
#define BS_BTMX_MODE_2to1        1
#define BS_BTMX_MODE_5to1        2

#define IDLE_DELETION_MODE_BYPASS       0
#define IDLE_DELETION_MODE_40G          1
#define IDLE_DELETION_MODE_100G         2
#define IDLE_DELETION_MODE_20G          3

#define T_FIFO_MODE_BYPASS       0
#define T_FIFO_MODE_40G          1
#define T_FIFO_MODE_100G         2
#define T_FIFO_MODE_20G          3

#define T_ENC_MODE_BYPASS        0
#define T_ENC_MODE_CL49          1
#define T_ENC_MODE_CL82          2

#define R_DESKEW_MODE_BYPASS     0
#define R_DESKEW_MODE_20G        1
#define R_DESKEW_MODE_40G_4_LANE 2
#define R_DESKEW_MODE_40G_2_LANE 3
#define R_DESKEW_MODE_100G       4
#define R_DESKEW_MODE_CL49       5
#define R_DESKEW_MODE_CL91       6

#define AM_MODE_20G              1
#define AM_MODE_40G              2
#define AM_MODE_100G             3

#define NUM_LANES_01                            0
#define NUM_LANES_02                            1
#define NUM_LANES_04                            2


#define PORT_MODE_FOUR                          0
#define PORT_MODE_THREE_3_2_COMBO               1
#define PORT_MODE_THREE_1_0_COMBO               2
#define PORT_MODE_TWO                           3
#define PORT_MODE_ONE                           4

#define OS_MODE_1                               0
#define OS_MODE_2                               1
#define OS_MODE_4                               2
#define OS_MODE_21p25                           4
#define OS_MODE_16p5                            8
#define OS_MODE_20p625                          12

#define REG_ACCESS_TYPE_MDIO                    0
#define REG_ACCESS_TYPE_UC                      1
#define REG_ACCESS_TYPE_EXT                     2


#define BS_CL82_SYNC_MODE 0
#define BS_CL49_SYNC_MODE 1
#define BLKSIZE           66
#define DSC_DATA_WID      40

#endif

