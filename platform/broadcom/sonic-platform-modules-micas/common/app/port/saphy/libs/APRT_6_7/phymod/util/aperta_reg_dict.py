CHIPID_REGS_NAME_ADDR = [
                         # ING_COMMON
                         #('ING_RX_PORTID',        '0x4900E000'),
                         #('ING_TX_PORTID',        '0x4900E100'),

                         # EGR_COMMON
                         #('EGR_RX_PORTID',        '0x4900F000'),
                         #('EGR_TX_PORTID',        '0x4900F100'),

                         # EGR_P2M
                         ('EGR_P2M_CTRL',        '0x4900D001'),
                         ('EGR_P2M_FIFO_CTRL',   '0x4900D002'),
                         ('EGR_P2M_SOP_CNTR',    '0x4900D003'),
                         ('EGR_P2M_EOP_CNTR',    '0x4900D004'),
                         ('EGR_P2M_STATUS',      '0x4900D008'),
                         ('EGR_P2M_STATUS2',     '0x4900D009'),

                         # EGR_P2MRC
                         ('EGR_P2MRC_CTRL',      '0x22000000'),

                         ('EGR_P2MRC_BPC',       '0x22000004'),

                         # EGR_FC
                         ('EGR_FC_BKPON_THR',    '0x4900B000'),
                         ('EGR_FC_BKPOFF_THR',   '0x4900B001'),
                         ('EGR_FC_CTRL',         '0x4900B002'),
                         ('EGR_FC_STATUS',       '0x4900B003'),
                         ('EGR_FC_BUFFIL',       '0x4900B004'),
                         ('EGR_FC_MAXBUFFIL',    '0x4900B005'),
                         ('EGR_FC_DBG_STATUS',   '0x4900B006'),
                         ('EGR_FC_CTRLGEN_0_3',  '0x4900B800'),
                         ('EGR_FC_CTRLGEN_4_7',  '0x4900B801'),


                         # EGR_FCRC
                         ('EGR_FCRC_CTRL',       '0x22001000'),
                         ('EGR_FCRC_BPC',        '0x22001004'),

                         # PTP STBYP
                         ('PTP_TXRX_STBYP',      '0x49007007'),

                         # EGR_SF
                         ('EGR_SF_BKPON_THR',    '0x49009000'),
                         ('EGR_SF_BKPOFF_THR',   '0x49009001'),
                         ('EGR_SF_CTRL',         '0x49009004'),
                         ('EGR_SF_STATUS',       '0x49009005'),
                         ('EGR_SF_BUFFIL',       '0x49009006'),
                         ('EGR_SF_MAXBUFFIL',    '0x49009007'),
                         ('EGR_SF_PKT_DRPD',     '0x49009008'),
                         ('EGR_SF_DBG_STATUS',   '0x49009009'),
                         ('EGR_SF_CTRLGEN_0_7',  '0x49009800'),

                         # EGR_M2P

                         ('EGR_M2P_CTRL',        '0x4900D021'),
                         ('EGR_M2P_FXLTN_CFG',   '0x4900D022'),
                         ('EGR_M2P_FIFO_CTRL',   '0x4900D023'),
                         ('EGR_M2P_MINLAT_STS',  '0x4900D024'),
                         ('EGR_M2P_MAXLAT_STS',  '0x4900D025'),
                         ('EGR_M2P_CRDT_CNTR',   '0x4900D026'),
                         ('EGR_M2P_STATUS',      '0x4900D027'),
                         ('EGR_M2P_STATUS2',     '0x4900D028'),

                         # ING_P2M

                         ('ING_P2M_CTRL',        '0x4900C001'),
                         ('ING_P2M_FIFO_CTRL',   '0x4900C002'),
                         ('ING_P2M_SOP_CNTR',    '0x4900C003'),
                         ('ING_P2M_EOP_CNTR',    '0x4900C004'),
                         ('ING_P2M_STATUS',      '0x4900C008'),
                         ('ING_P2M_STAUS2',      '0x4900C009'),

                         # ING_P2MRC
                         ('ING_P2MRC_CTRL',      '0x21000000'),
                         ('ING_P2MRC_BPC',       '0x21000004'),

                         # ING PTP STBYP
                         ('ING_TXRX_STBYP',      '0x49007007'),

                         # ING_FC
                         ('ING_FC_BKPON_THR',    '0x4900A000'),
                         ('ING_FC_BKPOFF_THR',   '0x4900A001'),
                         ('ING_FC_CTRL',         '0x4900A002'),
                         ('ING_FC_STATUS',       '0x4900A003'),
                         ('ING_FC_BUFFIL',       '0x4900A004'),
                         ('ING_FC_MAXBUFFIL',    '0x4900A006'),
                         ('ING_FC_GOOD_FRMS',    '0x4900A005'),
                         ('ING_FC_PKT_DRPD',     '0x4900A007'),
                         ('ING_FC_DBG_STATUS',   '0x4900A008'),
                         ('ING_FC_CTRLGEN0_3',   '0x4900A800'),
                         ('ING_FC_CTRLGEN4_7',   '0x4900A801'),

                         # ING_M2P
                         ('ING_M2P_CTRL',        '0x4900C021'),
                         ('ING_M2P_FXLTN_CFG',   '0x4900C022'),
                         ('ING_M2P_FIFO_CTRL',   '0x4900C023'),
                         ('ING_M2P_MINLAT_STS',  '0x4900C024'),
                         ('ING_M2P_MAXLAT_STS',  '0x4900C025'),
                         ('ING_M2P_CRDT_CNTR',   '0x4900C026'),
                         ('ING_M2P_STATUS',      '0x4900C027'),
                         ('ING_M2P_STATUS2',     '0x4900C028'),
                        ] #CHIPID_REGS_NAME_ADDR

TSCBH_REGS_NAME_ADDR = [
                        ('TSCBH_MAIN0_SETUP',                                '0x18029000'),
                        ('TSCBH_MAIN0_DEVICEINPKG5',                         '0x18029002'),
                        ('TSCBH_MAIN0_TICK_CTRL_1',                          '0x18029003'),
                        ('TSCBH_MAIN0_TICK_CTRL_0',  	                     '0x18029004'),
                        ('TSCBH_MAIN0_SERDESID',            	             '0x18029008'),
                        ('TSCBH_MAIN0_ECC_DISABLE_CTRL',     	             '0x18029009'),
                        ('TSCBH_MAIN0_ECC_1B_ERR_INTERRUPT_EN',     	     '0x1802900A'),
                        ('TSCBH_MAIN0_ECC_2B_ERR_INTERRUPT_EN',              '0x1802900B'),
                        ('TSCBH_MAIN0_ECC_CORRUPT_CTRL_0',                   '0x1802900C'),
                        ('TSCBH_MAIN0_ECC_CORRUPT_CTRL_1',                   '0x1802900D'),
                        ('TSCBH_MAIN0_TM_CTRL',                              '0x1802900E'),
                        ('TSCBH_PMD_X1_CTRL',                                '0x18029010'),
                        ('TSCBH_PMD_X1_PM_TIMER_OFFSET',                     '0x18029011'),
                        ('TSCBH_PMD_X1_FCLK_PERIOD',                         '0x18029012'),
                        ('TSCBH_PKTGEN0_PKTGENCTRL1',                        '0x18029030'),
                        ('TSCBH_PKTGEN0_PRTPCTRL',                           '0x18029033'),
                        ('TSCBH_PKTGEN0_PCS_SEEDA0',                         '0x18029037'),
                        ('TSCBH_PKTGEN0_PCS_SEEDA1',                         '0x18029038'),
                        ('TSCBH_PKTGEN0_PCS_SEEDA2',                         '0x18029039'),
                        ('TSCBH_PKTGEN0_PCS_SEEDA3',                         '0x1802903A'),
                        ('TSCBH_PKTGEN0_PCS_SEEDB0',                         '0x1802903B'),
                        ('TSCBH_PKTGEN0_PCS_SEEDB1',                         '0x1802903C'),
                        ('TSCBH_PKTGEN0_PCS_SEEDB2',                         '0x1802903D'),
                        ('TSCBH_PKTGEN0_PCS_SEEDB3',                         '0x1802903E'),
                        ('TSCBH_PKTGEN1_ERRMASK4',                           '0x18029040'),
                        ('TSCBH_PKTGEN1_ERRMASK3',                           '0x18029041'),
                        ('TSCBH_PKTGEN1_ERRMASK2',                           '0x18029042'),
                        ('TSCBH_PKTGEN1_ERRMASK1',                           '0x18029043'),
                        ('TSCBH_PKTGEN1_ERRMASK0',                           '0x18029044'),
                        ('TSCBH_PKTGEN1_GLASTEST_CTRL',                      '0x18029045'),
                        ('TSCBH_PKTGEN1_GLASTEST_EXP_0',                     '0x18029046'),
                        ('TSCBH_PKTGEN1_GLASTEST_EXP_1',                     '0x18029047'),
                        ('TSCBH_PKTGEN1_GLASTEST_EXP_2',                     '0x18029048'),
                        ('TSCBH_PKTGEN1_GLASTEST_ACTDATA_0',                 '0x18029049'),
                        ('TSCBH_PKTGEN1_GLASTEST_ACTDATA_1',                 '0x1802904A'),
                        ('TSCBH_PKTGEN1_GLASTEST_ACTDATA_2',                 '0x1802904B'),
                        ('TSCBH_PKTGEN1_GLASTEST_ACTADJ',                    '0x1802904C'),
                        ('TSCBH_TX_CTRL0_TX_LANE_SWAP',                      '0x18029200'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_CTRL',                    '0x18029201'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_DATA_0',                  '0x18029202'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_DATA_1',                  '0x18029203'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_DATA_2',                  '0x18029204'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_DATA_3',                  '0x18029205'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_DATA_4',                  '0x18029206'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_DATA_5',                  '0x18029207'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_DATA_6',                  '0x18029208'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_DATA_7',                  '0x18029209'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_ADJ_0_1',                 '0x1802920A'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_ADJ_2_3',                 '0x1802920B'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_ADJ_4_5',                 '0x1802920C'),
                        ('TSCBH_TX_CTRL0_GLAS_TPMA_ADJ_6_7',                 '0x1802920D'),
                        ('TSCBH_RX_X1_CTRL0_RS_FEC_CFG',                     '0x18029223'),
                        ('TSCBH_RX_X1_CTRL0_RX_LANE_SWAP',                   '0x18029225'),
                        ('TSCBH_RX_X1_ECC_STS_BASE_R_FEC',                   '0x18029230'),
                        ('TSCBH_RX_X1_ECC_STS_RSFEC_RBUF_MPP',               '0x18029231'),
                        ('TSCBH_RX_X1_ECC_STS_RSFEC_RS400G_MPP',             '0x18029233'),
                        ('TSCBH_RX_X1_ECC_STS_DESKEW',                       '0x18029235'),
                        ('TSCBH_RX_X1_ECC_STS_SPD_TBL',                      '0x18029236'),
                        ('TSCBH_RX_X1_ECC_STS_AM_TBL',                       '0x18029237'),
                        ('TSCBH_RX_X1_ECC_STS_UM_TBL',                       '0x18029238'),
                        ('TSCBH_RX_X1_ECC_STS_TX_1588_MPP',                  '0x18029239'),
                        ('TSCBH_RX_X1_ECC_STS_TX_1588_400G',                 '0x1802923B'),
                        ('TSCBH_RX_X1_ECC_STS_RX_1588_MPP',                  '0x1802923C'),
                        ('TSCBH_RX_X1_ECC_STS_RX_1588_400G',                 '0x1802923E'),
                        ('TSCBH_AN_X1_T_CL73_BREAK_LINK',                    '0x18029250'),
                        ('TSCBH_AN_X1_T_CL73_ERR',                           '0x18029251'),
                        ('TSCBH_AN_X1_T_IGNORE_LINK_TIMER',                  '0x18029254'),
                        ('TSCBH_AN_X1_T_LF_INHIBIT_TIMER_CL72',       	     '0x18029255'),
                        ('TSCBH_AN_X1_T_LF_INHIBIT_TIMER_NOT_CL72',   	     '0x18029256'),
                        ('TSCBH_AN_X1_T_DME_PG_TIMER_TYPE',                  '0x18029257'),
                        ('TSCBH_AN_X1_T_IGNORE_LINK_TIMER_PAM4',             '0x18029259'),
                        ('TSCBH_AN_X1_T_LF_INHIBIT_TIMER_CL72_PAM4',   	     '0x1802925A'),
                        ('TSCBH_AN_X1_T_LF_INHIBIT_TIMER_NOT_CL72_PAM4',     '0x1802925B'),
                        ('TSCBH_SC_X1_CTRL_PIPELINE_RESET_COUNT',            '0x18029262'),
                        ('TSCBH_SC_X1_CTRL_TX_RESET_COUNT',                  '0x18029263'),
                        ('TSCBH_SC_X1_CTRL_STS',                             '0x18029264'),
                        ('TSCBH_AN_X1_S_MGT_GLB_INT',                        '0x180292C0'),
                        ('TSCBH_AN_X1_S_MGT_GLB_MASK',                       '0x180292C1'),
                        ('TSCBH_PMD_X4_CTRL',                                '0x1802C010'),
                        ('TSCBH_PMD_X4_MODE',                                '0x1802C011'),
                        ('TSCBH_PMD_X4_STS',                                 '0x1802C012'),
                        ('TSCBH_PMD_X4_LATCH_STS',                           '0x1802C013'),
                        ('TSCBH_PMD_X4_OVERRIDE',                            '0x1802C014'),
                        ('TSCBH_PMD_X4_UI_VALUE_HI',                         '0x1802C015'),
                        ('TSCBH_PMD_X4_UI_VALUE_LO',                         '0x1802C016'),
                        ('TSCBH_PMD_X4_RX_FIXED_LATENCY',                    '0x1802C017'),
                        ('TSCBH_PMD_X4_TX_FIXED_LATENCY',                    '0x1802C018'),
                        ('TSCBH_SC_X4_CTRL_CTRL',                            '0x1802C050'),
                        ('TSCBH_SC_X4_CTRL_STS',                             '0x1802C051'),
                        ('TSCBH_SC_X4_CTRL_DEBUG',                           '0x1802C054'),
                        ('TSCBH_SC_X4_CTRL_SPARE0',                          '0x1802C05D'),
                        ('TSCBH_SC_X4_CTRL_SW_SPARE1',                       '0x1802C05E'),
                        ('TSCBH_SC_X4_FINAL_CFG_STS_RESOLV_SPD',             '0x1802C070'),
                        ('TSCBH_SC_X4_FINAL_CFG_STS_FEC_STS',                '0x1802C078'),
                        ('TSCBH_TX_X4_CTRL0_RS_SYMBOL',                      '0x1802C110'),
                        ('TSCBH_TX_X4_CTRL0_MISC',                           '0x1802C111'),
                        ('TSCBH_TX_X4_CTRL0_TX_TS_CTRL',                     '0x1802C112'),
                        ('TSCBH_TX_X4_CTRL0_ERR_CTRL',                       '0x1802C113'),
                        ('TSCBH_TX_X4_STS0_ENCODE_STS_1',                    '0x1802C121'),
                        ('TSCBH_TX_X4_STS0_PCS_STS_LATCHED',                 '0x1802C123'),
                        ('TSCBH_TX_X4_STS0_TX_1588_TIMESTAMP_STS',           '0x1802C124'),
                        ('TSCBH_TX_X4_STS0_TX_1588_TIMESTAMP_HI',            '0x1802C125'),
                        ('TSCBH_TX_X4_STS0_TX_1588_TIMESTAMP_MID',           '0x1802C126'),
                        ('TSCBH_TX_X4_STS0_TX_1588_TIMESTAMP_LO',            '0x1802C127'),
                        ('TSCBH_TX_X4_STS0_TX_TS_SEQ_ID',                    '0x1802C128'),
                        ('TSCBH_RX_CTRL0_RS_FEC_TIMER',                      '0x1802C130'),
                        ('TSCBH_RX_CTRL0_RS_FEC_RX_CTRL_0',                  '0x1802C131'),
                        ('TSCBH_RX_CTRL0_DECODE_CTRL_0',                     '0x1802C132'),
                        ('TSCBH_RX_CTRL0_PMA_CTRL_0',                        '0x1802C133'),
                        ('TSCBH_RX_CTRL0_RX_TS_CTRL',                        '0x1802C134'),
                        ('TSCBH_RX_FEC_CTRL_FEC_0',                          '0x1802C140'),
                        ('TSCBH_RX_FEC_CTRL_FEC_1',                          '0x1802C141'),
                        ('TSCBH_RX_FEC_CTRL_FEC_2',                          '0x1802C142'),
                        ('TSCBH_RX_STS0_BLKSYNC_STS',                        '0x1802C150'),
                        ('TSCBH_RX_STS0_BLKSYNC_DBG0',                       '0x1802C151'),
                        ('TSCBH_RX_STS0_BLKSYNC_DBG1',                       '0x1802C152'),
                        ('TSCBH_RX_STS0_BLKSYNC_DBG2',                       '0x1802C153'),
                        ('TSCBH_RX_STS0_BLOCK_LOCK_LAT_STS',                 '0x1802C154'),
                        ('TSCBH_RX_STS0_AM_LOCK_LAT_STS',                    '0x1802C155'),
                        ('TSCBH_RX_STS0_BIPCOUNT_0',                         '0x1802C157'),
                        ('TSCBH_RX_STS0_BIPCOUNT_1',                         '0x1802C158'),
                        ('TSCBH_RX_STS0_BIPCOUNT_2',                         '0x1802C159'),
                        ('TSCBH_RX_STS0_PSLL_TO_VL_MAPPING_0',               '0x1802C15A'),
                        ('TSCBH_RX_STS0_PSLL_TO_VL_MAPPING_1',               '0x1802C15B'),
                        ('TSCBH_RX_STS0_RS_SYMBOL',                          '0x1802C15C'),
                        ('TSCBH_RX_STS1_PCS_LAT_STS_1',                      '0x1802C160'),
                        ('TSCBH_RX_STS1_RL_MODE_HW_STS',                     '0x1802C161'),
                        ('TSCBH_RX_STS1_RL_MODE_SW_CTRL',                    '0x1802C162'),
                        ('TSCBH_RX_STS1_DECODE_STS_1',                       '0x1802C163'),
                        ('TSCBH_RX_STS1_DECODE_STS_3',                       '0x1802C165'),
                        ('TSCBH_RX_STS1_RX_LAT_STS',                         '0x1802C167'),
                        ('TSCBH_RX_STS1_BER_LO',                             '0x1802C168'),
                        ('TSCBH_RX_STS1_BER_HO',                             '0x1802C169'),
                        ('TSCBH_RX_STS1_ERRED_BLOCKS_HO',                    '0x1802C16A'),
                        ('TSCBH_RX_STS1_CL49_SCRIDLE_TERR',                  '0x1802C16B'),
                        ('TSCBH_RX_STS2_CL82_RX_AM_LAT_STS_PSLL_0',    	     '0x1802C170'),
                        ('TSCBH_RX_STS2_CL82_RX_AM_LAT_STS_PSLL_1',    	     '0x1802C171'),
                        ('TSCBH_RX_STS2_CL82_RX_AM_LAT_STS_PSLL_2',    	     '0x1802C172'),
                        ('TSCBH_RX_STS2_CL82_RX_AM_LAT_STS_PSLL_3',    	     '0x1802C173'),
                        ('TSCBH_RX_STS2_CL82_RX_AM_LAT_STS_PSLL_4',    	     '0x1802C174'),
                        ('TSCBH_RX_STS2_RS_FEC_SYNC_STS',                    '0x1802C17A'),
                        ('TSCBH_RX_STS2_FEC_SYNC_FSM_STATE',                 '0x1802C17B'),
                        ('TSCBH_RX_STS3_FEC_DBG_ERRL_0',                     '0x1802C180'),
                        ('TSCBH_RX_STS3_FEC_DBG_ERRAH_0',                    '0x1802C185'),
                        ('TSCBH_RX_STS3_FEC_BURST_ERR_STSL_0',               '0x1802C18A'),
                        ('TSCBH_RX_STS4_FEC_BURST_ERR_STSH_0',               '0x1802C190'),
                        ('TSCBH_RX_STS4_FEC_CORRBLKSL_0',                    '0x1802C191'),
                        ('TSCBH_RX_STS4_FEC_CORRBLKSH_0',                    '0x1802C192'),
                        ('TSCBH_RX_STS4_PRTPERRCOUNTER',                     '0x1802C193'),
                        ('TSCBH_RX_STS4_PRTPSTS',                            '0x1802C194'),
                        ('TSCBH_RX_STS5_FEC_UNCORRBLKSL_0',                  '0x1802C1A0'),
                        ('TSCBH_RX_STS5_FEC_UNCORRBLKSH_0',                  '0x1802C1A1'),
                        ('TSCBH_RX_STS5_SKEW_OFFSETS_0',                     '0x1802C1A2'),
                        ('TSCBH_RX_STS5_SKEW_OFFSETS_1',                     '0x1802C1A3'),
                        ('TSCBH_RX_STS5_SKEW_OFFSETS_2',                     '0x1802C1A4'),
                        ('TSCBH_RX_STS5_SKEW_OFFSETS_3',                     '0x1802C1A5'),
                        ('TSCBH_RX_STS5_SKEW_OFFSETS_4',                     '0x1802C1A6'),
                        ('TSCBH_RX_STS5_AM_TS_INFO_0',                       '0x1802C1A7'),
                        ('TSCBH_RX_STS5_AM_TS_INFO_1',                       '0x1802C1A8'),
                        ('TSCBH_RX_STS5_AM_TS_INFO_2',                       '0x1802C1A9'),
                        ('TSCBH_RX_STS5_AM_TS_INFO_3',                       '0x1802C1AA'),
                        ('TSCBH_RX_STS5_AM_TS_INFO_4',                       '0x1802C1AB'),
                        ('TSCBH_RX_TEST_STS0_CL82_SCRIDLE_TERR',             '0x1802C1B0'),
                        ('TSCBH_AN_ABI_CL73_CFG',                            '0x1802C1C0'),
                        ('TSCBH_AN_ABI_LD_UP1_ABI_0',                	     '0x1802C1C1'),
                        ('TSCBH_AN_ABI_LD_UP1_ABI_1',                	     '0x1802C1C2'),
                        ('TSCBH_AN_ABI_LD_BASE_ABI_0',               	     '0x1802C1C3'),
                        ('TSCBH_AN_ABI_LD_BASE_ABI_1',               	     '0x1802C1C4'),
                        ('TSCBH_AN_ABI_LD_BAM_ABILITIES',              	     '0x1802C1C5'),
                        ('TSCBH_AN_ABI_CL73_CTRLS',                          '0x1802C1C6'),
                        ('TSCBH_AN_ABI_LD_BASE_ABI_2',                       '0x1802C1C7'),
                        ('TSCBH_AN_ABI_LD_BASE_ABI_3',                       '0x1802C1C8'),
                        ('TSCBH_AN_ABI_SW_AN_BASE_PG_0',                     '0x1802C1CB'),
                        ('TSCBH_AN_ABI_SW_AN_BASE_PG_1',                     '0x1802C1CC'),
                        ('TSCBH_AN_ABI_SW_AN_BASE_PG_2',                     '0x1802C1CD'),
                        ('TSCBH_AN_STS_R_CL73_STS',                          '0x1802C1D0'),
                        ('TSCBH_AN_STS_PXNG_STS',                            '0x1802C1D1'),
                        ('TSCBH_AN_STS_PSEQ_STS',                            '0x1802C1D2'),
                        ('TSCBH_AN_STS_LP_BASE1',                            '0x1802C1D3'),
                        ('TSCBH_AN_STS_LP_BASE2',                            '0x1802C1D4'),
                        ('TSCBH_AN_STS_LP_BASE3',                            '0x1802C1D5'),
                        ('TSCBH_AN_STS_LP_MP5_LOWER',                        '0x1802C1D6'),
                        ('TSCBH_AN_STS_LP_MP5_MIDDLE',                       '0x1802C1D7'),
                        ('TSCBH_AN_STS_LP_MP5_UPPER',                        '0x1802C1D8'),
                        ('TSCBH_AN_STS_LP_U_LOWER',                          '0x1802C1D9'),
                        ('TSCBH_AN_STS_LP_U_MIDDLE',                         '0x1802C1DA'),
                        ('TSCBH_AN_STS_LP_U_UPPER',                          '0x1802C1DB'),
                        ('TSCBH_AN_STS_RES_ERR',                             '0x1802C1DC'),
                        ('TSCBH_AN_S_MGT_LD_PG_2',                           '0x1802C1E0'),
                        ('TSCBH_AN_S_MGT_LD_PG_1',                           '0x1802C1E1'),
                        ('TSCBH_AN_S_MGT_LD_PG_0',                           '0x1802C1E2'),
                        ('TSCBH_AN_S_MGT_LP_PG_2',                           '0x1802C1E3'),
                        ('TSCBH_AN_S_MGT_LP_PG_1',                           '0x1802C1E4'),
                        ('TSCBH_AN_S_MGT_LP_PG_0',                           '0x1802C1E5'),
                        ('TSCBH_AN_S_MGT_SW_CTRL_STS',                       '0x1802C1E6'),
                        ('TSCBH_AN_S_MGT_LD_CTRL',                           '0x1802C1E7'),
                        ('TSCBH_AN_S_MGT_AN_ABIRESTS',		             '0x1802C1E8'),
                        ('TSCBH_AN_S_MGT_AN_MISC_STS',			     '0x1802C1E9'),
                        ('TSCBH_AN_S_MGT_TLA_SEQ_STS',		   	     '0x1802C1EA'),
                        ('TSCBH_AN_S_MGT_INT',				     '0x1802C1EB'),
                        ('TSCBH_AN_S_MGT_INT_EN',			     '0x1802C1EC'),
                        ('TSCBH_AN_S_MGT_WAIT_ACK_COMP',		     '0x1802C1ED'),
                        ('TSCBH_SYNCE_X4_INT_DIV',			     '0x1802C211'),
                        ('TSCBH_SYNCE_X4_FRAC_DIV',			     '0x1802C212'),
                        ('TSCBH_ILKN_X4_CTRL0_ILKN_CTRL0',		     '0x1802C330'),
                        ('TSCBH_ILKN_X4_STS0_ILKN_STS0',		     '0x1802C340'),
                        ('TSCBH_RX_RFEC_S0_RS_FEC_RXP_STS',		     '0x1802C351'),
                        ('TSCBH_RX_RFEC_S0_CORR_CNTR_0',		     '0x1802C352'),
                        ('TSCBH_RX_RFEC_S0_CORR_CNTR_1',		     '0x1802C353'),
                        ('TSCBH_RX_RFEC_S0_UNCORR_CNTR_0',		     '0x1802C354'),
                        ('TSCBH_RX_RFEC_S0_UNCORR_CNTR_1',		     '0x1802C355'),
                        ('TSCBH_RX_RFEC_S0_BERR_CNTR_0',		     '0x1802C356'),
                        ('TSCBH_RX_RFEC_S0_BERR_CNTR_1',		     '0x1802C357'),
                        ('TSCBH_RX_RFEC_S1ER_CNTR_L_0',                      '0x1802C360'),
                        ('TSCBH_RX_RFEC_S1ER_CNTR_U_0',	                     '0x1802C361'),
                        ('TSCBH_RX_RFEC_S1ER_CNTR_L_1',                      '0x1802C362'),
                        ('TSCBH_RX_RFEC_S1ER_CNTR_U_1',	                     '0x1802C363'),
                        ('TSCBH_RX_RFEC_S1ER_CNTR_L_2',                      '0x1802C364'),
                        ('TSCBH_RX_RFEC_S1ER_CNTR_U_2',	                     '0x1802C365'),
                        ('TSCBH_RX_RFEC_S1ER_CNTR_L_3',                      '0x1802C366'),
                        ('TSCBH_RX_RFEC_S1ER_CNTR_U_3',	                     '0x1802C367'),
                       ] #TSCBH_REGS_NAME_ADDR

PM_TSC_BHAWK_NAME_ADDR = [
                        ('AMS_PLL_COM_PLL_STS',             '0x1800D11C'),
                        ('PLL_CAL_COM_CTL_7',               '0x1800D147'),
                        ('PLL_CAL_COM_CTL_STS_0',           '0x1800D148'),
                       ] #PM_TSC_BHAWK_NAME_ADDR

CDMAC_REGS_NAME_ADDR = [
                        ('CDMAC_CTRL',                         '0x1400010b'),
                        ('CDMAC_MODE',                         '0x1400010c'),
                        ('CDMAC_TX_CTRL',                      '0x1500010d'),
                        ('CDMAC_TX_MAC_SA',                    '0x1600010e'),
                        ('CDMAC_RX_CTRL',                      '0x1400010f'),
                        ('CDMAC_RX_MAC_SA',                    '0x16000110'),
                        ('CDMAC_RXMAXSIZE',                    '0x14000111'),
                        ('CDMAC_RX_VLAN_TAG',                  '0x14000112'),
                        ('CDMAC_RXLSS_CTRL',                   '0x14000113'),
                        ('CDMAC_RXLSS_STS',                    '0x14000114'),
                        ('CDMAC_PAUSE_CTRL',                   '0x16000115'),
                        ('CDMAC_PFC_CTRL',                     '0x14000116'),
                        ('CDMAC_PFCTYPE',                      '0x14000117'),
                        ('CDMAC_PFC_OPCODE',                   '0x14000118'),
                        ('CDMAC_PFC_DA',                       '0x16000119'),
                        ('CDMAC_FIFO_STS',                     '0x1400011a'),
                        ('CDMAC_LAG_FAILOVER_STATUS',          '0x1400011b'),
                        ('CDMAC_TXFIFOCELL_CNT',               '0x1400011c'),
                        ('CDMAC_TXFIFOCELLREQ_CNT',            '0x1400011d'),
                        ('CDMAC_INTREN',                       '0x1400011e'),
                        ('CDMAC_INTR_STS',                     '0x1400011f'),
                        ('CDMAC_RSVMASK',                      '0x14000120'),
                        ('CDMAC_MIB_COUNTER_CTRL',             '0x14000121'),
                        ('CDMAC_SPARE',                        '0x14000122'),
                        ('CDMAC_VERSION_ID',                   '0x14020100'),
                        ('CDMAC_LINK_INTR_CTL',                '0x15020101'),
                        ('CDMAC_LINK_INTR_STS',                '0x15020102'),
                        ('CDMAC_LINK_ECC_CTL',                 '0x14020103'),
                        ('CDMAC_LINK_ECC_STS',                 '0x14020104'),
                        ('CDMAC_LINK_MEM_CTL',                 '0x14020105'),
                        ('CDMAC_MIB_CNT_MODE',                 '0x1402010A'),
                       ] #CDMAC_REGS_NAME_ADDR

CDPORT_REGS_NAME_ADDR = [
                        ('CDPORT_LAG_FAILOVER_CONFIG',         '0x10000000'),
                        ('CDPORT_FAULT_LINK_STATUS',           '0x10000001'),
                        ('CDPORT_SW_FLOW_CONTROL',             '0x10000002'),
                        ('CDPORT_FLOW_CONTROL_CONFIG',         '0x10000003'),
                        ('CDPORT_LED_CONTROL',                 '0x10000004'),
                        ('CDPORT_SPARE0_REG',                  '0x11000005'),
                        ('CDPORT_SPARE1_REG',                  '0x11000006'),
                        ('CDPORT_SPARE2_REG',                  '0x11000007'),
                        ('CDPORT_SPARE3_REG',                  '0x11000008'),
                        ('CDPORT_MODEREG',                     '0x10020009'),
                        ('CDPORT_MAC_CTRL',                    '0x1002000a'),
                        ('CDPORT_TSCPLLCK_STS',                '0x1002000b'),
                        ('CDPORT_XGXS0_CTRL',                  '0x1002000c'),
                        ('CDPORT_XGXS0_LN0_STS',               '0x1002000d'),
                        ('CDPORT_XGXS0_LN1_STS',               '0x1002000e'),
                        ('CDPORT_XGXS0_LN2_STS',               '0x1002000f'),
                        ('CDPORT_XGXS0_LN3_STS',               '0x10020010'),
                        ('CDPORT_XGXS0_LN4_STS',               '0x10020011'),
                        ('CDPORT_XGXS0_LN5_STS',               '0x10020012'),
                        ('CDPORT_XGXS0_LN6_STS',               '0x10020013'),
                        ('CDPORT_XGXS0_LN7_STS',               '0x10020014'),
                        ('CDPORT_INTR_STS',                    '0x11020017'),
                        ('CDPORT_INTREN',                      '0x11020018'),
                        ('CDPORT_TSCINTR_STS',                 '0x1202001f'),
                       ] #CDPORT_REGS_NAME_ADDR

CDMIB_REGS_NAME_ADDR = [
                        ('CDMIB_R2047',                       'CDMIB[0.7]'),
                        ('CDMIB_RMGV',                        'CDMIB[0.6]'),
                        ('CDMIB_R1518',                       'CDMIB[0.5]'),
                        ('CDMIB_R1023',                       'CDMIB[0.4]'),
                        ('CDMIB_R511',                        'CDMIB[0.3]'),
                        ('CDMIB_R255',                        'CDMIB[0.2]'),
                        ('CDMIB_R127',                        'CDMIB[0.1]'),
                        ('CDMIB_R64',                         'CDMIB[0.0]'),

                        ('CDMIB_RPROG3',                      'CDMIB[1.7]'),
                        ('CDMIB_RPROG2',                      'CDMIB[1.6]'),
                        ('CDMIB_RPROG1',                      'CDMIB[1.5]'),
                        ('CDMIB_RPROG0',                      'CDMIB[1.4]'),
                        ('CDMIB_RBCA',                        'CDMIB[1.3]'),
                        ('CDMIB_R16383',                      'CDMIB[1.2]'),
                        ('CDMIB_R9216',                       'CDMIB[1.1]'),
                        ('CDMIB_R4095',                       'CDMIB[1.0]'),

                        ('CDMIB_RXCF',                        'CDMIB[2.7]'),
                        ('CDMIB_RXPP',                        'CDMIB[2.6]'),
                        ('CDMIB_RXPF',                        'CDMIB[2.5]'),
                        ('CDMIB_RMCA',                        'CDMIB[2.4]'),
                        ('CDMIB_RUCA',                        'CDMIB[2.2]'),
                        ('CDMIB_RPOK',                        'CDMIB[2.1]'),
                        ('CDMIB_RPKT',                        'CDMIB[2.0]'),

                        ('CDMIB_RDVLN',                       'CDMIB[3.7]'),
                        ('CDMIB_RVLN',                        'CDMIB[3.6]'),
                        ('CDMIB_ROVR',                        'CDMIB[3.5]'),
                        ('CDMIB_RMTUE',                       'CDMIB[3.4]'),
                        ('CDMIB_RJBR',                        'CDMIB[3.3]'),
                        ('CDMIB_RFLR',                        'CDMIB[3.2]'),
                        ('CDMIB_RERPKT',                      'CDMIB[3.1]'),
                        ('CDMIB_RFCS',                        'CDMIB[3.0]'),

                        ('CDMIB_RPFCOFF1',                    'CDMIB[4.7]'),
                        ('CDMIB_RPFC1',                       'CDMIB[4.6]'),
                        ('CDMIB_RPFCOFF0',                    'CDMIB[4.5]'),
                        ('CDMIB_RPFC0',                       'CDMIB[4.4]'),
                        ('CDMIB_RPRM',                        'CDMIB[4.3]'),
                        ('CDMIB_RXWSA',                       'CDMIB[4.2]'),
                        ('CDMIB_RXUDA',                       'CDMIB[4.1]'),
                        ('CDMIB_RXUO',                        'CDMIB[4.0]'),

                        ('CDMIB_RPFCOFF5',                    'CDMIB[5.7]'),
                        ('CDMIB_RPFC5',                       'CDMIB[5.6]'),
                        ('CDMIB_RPFCOFF4',                    'CDMIB[5.5]'),
                        ('CDMIB_RPFC4',                       'CDMIB[5.4]'),
                        ('CDMIB_RPFCOFF3',                    'CDMIB[5.3]'),
                        ('CDMIB_RPFC3',                       'CDMIB[5.2]'),
                        ('CDMIB_RPFCOFF2',                    'CDMIB[5.1]'),
                        ('CDMIB_RPFC2',                       'CDMIB[5.0]'),

                        ('CDMIB_RRPKT',                       'CDMIB[6.6]'),
                        ('CDMIB_RFRG',                        'CDMIB[6.5]'),
                        ('CDMIB_RUND',                        'CDMIB[6.4]'),
                        ('CDMIB_RPFCOFF7',                    'CDMIB[6.3]'),
                        ('CDMIB_RPFC7',                       'CDMIB[6.2]'),
                        ('CDMIB_RPFCOFF6',                    'CDMIB[6.1]'),
                        ('CDMIB_RPFC6',                       'CDMIB[6.0]'),

                        ('CDMIB_T2047',                       'CDMIB[7.7]'),
                        ('CDMIB_TMGV',                        'CDMIB[7.6]'),
                        ('CDMIB_T1518',                       'CDMIB[7.5]'),
                        ('CDMIB_T1023',                       'CDMIB[7.4]'),
                        ('CDMIB_T511',                        'CDMIB[7.3]'),
                        ('CDMIB_T255',                        'CDMIB[7.2]'),
                        ('CDMIB_T127',                        'CDMIB[7.1]'),
                        ('CDMIB_T64',                         'CDMIB[7.0]'),

                        ('CDMIB_TPFCOFF1',                    'CDMIB[8.7]'),
                        ('CDMIB_TPFC1',                       'CDMIB[8.6]'),
                        ('CDMIB_TPFCOFF0',                    'CDMIB[8.5]'),
                        ('CDMIB_TPFC0',                       'CDMIB[8.4]'),
                        ('CDMIB_TBCA',                        'CDMIB[8.3]'),
                        ('CDMIB_T16383',                      'CDMIB[8.2]'),
                        ('CDMIB_T9216',                       'CDMIB[8.1]'),
                        ('CDMIB_T4095',                       'CDMIB[8.0]'),

                        ('CDMIB_TPFCOFF5',                    'CDMIB[9.7]'),
                        ('CDMIB_TPFC5',                       'CDMIB[9.6]'),
                        ('CDMIB_TPFCOFF4',                    'CDMIB[9.5]'),
                        ('CDMIB_TPFC4',                       'CDMIB[9.4]'),
                        ('CDMIB_TPFCOFF3',                    'CDMIB[9.3]'),
                        ('CDMIB_TPFC3',                       'CDMIB[9.2]'),
                        ('CDMIB_TPFCOFF2',                    'CDMIB[9.1]'),
                        ('CDMIB_TPFC2',                       'CDMIB[9.0]'),

                        ('CDMIB_TUFL',                        'CDMIB[A.7]'),
                        ('CDMIB_TUCA',                        'CDMIB[A.6]'),
                        ('CDMIB_TPOK',                        'CDMIB[A.5]'),
                        ('CDMIB_TPKT',                        'CDMIB[A.4]'),
                        ('CDMIB_TPFCOFF7',                    'CDMIB[A.3]'),
                        ('CDMIB_TPFC7',                       'CDMIB[A.2]'),
                        ('CDMIB_TPFCOFF6',                    'CDMIB[A.1]'),
                        ('CDMIB_TPFC6',                       'CDMIB[A.0]'),

                        ('CDMIB_TJBR',                        'CDMIB[B.7]'),
                        ('CDMIB_TOVR',                        'CDMIB[B.6]'),
                        ('CDMIB_TERR',                        'CDMIB[B.5]'),
                        ('CDMIB_TFCS',                        'CDMIB[B.4]'),
                        ('CDMIB_TXCF',                        'CDMIB[B.3]'),
                        ('CDMIB_TXPP',                        'CDMIB[B.2]'),
                        ('CDMIB_TXPF',                        'CDMIB[B.1]'),
                        ('CDMIB_TMCA',                        'CDMIB[B.0]'),

                        ('CDMIB_TBYT',                        'CDMIB[C.6]'),
                        ('CDMIB_RRBYT',                       'CDMIB[C.5]'),
                        ('CDMIB_RBYT',                        'CDMIB[C.4]'),
                        ('CDMIB_TDVLN',                       'CDMIB[C.3]'),
                        ('CDMIB_TVLN',                        'CDMIB[C.2]'),
                        ('CDMIB_TFRG',                        'CDMIB[C.1]'),
                        ('CDMIB_TRPKT',                       'CDMIB[C.0]'),
                       ] #CDMIB_REGS_NAME_ADDR

## MACSEC (Egres) ...
## ------------------
# - In progress

EGR163_COMMON_REGS_NAME_ADDR = [
                        ('EGR163_STBYPAS_CTRL',                                    '0x49004002'),
                        ('EGR163_PKTINFLIGHT_STS',                                 '0x49004005'),
                       ] #EGR163_COMMON_REGS_NAME_ADDR

EGR163_COMMON_L2_REGS_NAME_ADDR = [
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM0',                           '0x4500FF00'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM1',                           '0x4500FF04'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM2',                           '0x4500FF08'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM3',                           '0x4500FF0C'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM4',                           '0x4500FF10'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM5',                           '0x4500FF14'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM6',                           '0x4500FF18'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM7',                           '0x4500FF1C'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM8',                           '0x4500FF20'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM9',                           '0x4500FF24'),
                        ('EGR163_SYSCTRL_ECC_CNTR_RAM10',                          '0x4500FF28'),
                        ('EGR163_SYSCTRL_ECC_CORR_ENABLE',                         '0x4500FF60'),
                        ('EGR163_SYSCTRL_ECC_DERR_ENABLE',                         '0x4500FF64'),
                        ('EGR163_SYSCTRL_CLOCK_STATE',                             '0x4500FFE8'),
                        ('EGR163_SYSCTRL_FORCE_CLOCK_ON',                          '0x4500FFEC'),
                        ('EGR163_SYSCTRL_FORCE_CLOCK_OFF',                         '0x4500FFF0'),
                       ] #EGR163_COMMON_L2_REGS_NAME_ADDR

EGR163_PER_CH_REGS_NAME_ADDR = [
                        ('EGR163_SYSCTRL_CHANNEL_CTRL_i',                          '0x4500FE00'),
                       ] #EGR163_COMMON_REGS_NAME_ADDR

EGR163_PER_CH_L2_REGS_NAME_ADDR = [
                        ('EGR163_CH_TCAMHitMultiple_lo_CHAN',                      '0x4500E000'),
                        ('EGR163_CH_TCAMHitMultiple_hi_CHAN',                      '0x4500E004'),
                        ('EGR163_CH_HeaderParserDroppedPkts_lo_CHAN',              '0x4500E008'),
                        ('EGR163_CH_HeaderParserDroppedPkts_hi_CHAN',              '0x4500E00C'),
                        ('EGR163_CH_CNTR_TCAMMiss_lo_CHAN',                        '0x4500E010'),
                        ('EGR163_CH_CNTR_TCAMMiss_hi_CHAN',                        '0x4500E014'),
                        ('EGR163_CH_CNTR_PktsCtrl_lo_CHAN',                        '0x4500E018'),
                        ('EGR163_CH_CNTR_PktsCtrl_hi_CHAN',                        '0x4500E01C'),
                        ('EGR163_CH_CNTR_PktsData_lo_CHAN',                        '0x4500E020'),
                        ('EGR163_CH_CNTR_PktsData_hi_CHAN',                        '0x4500E024'),
                        ('EGR163_CH_CNTR_PktsDropped_lo_CHAN',                     '0x4500E028'),
                        ('EGR163_CH_CNTR_PktsDropped_hi_CHAN',                     '0x4500E02C'),
                        ('EGR163_CH_CNTR_PktsErrIn_lo_CHAN',                       '0x4500E030'),
                        ('EGR163_CH_CNTR_PktsErrIn_hi_CHAN',                       '0x4500E034'),
                        ] #EGR163_PER_CH_L2_REGS_NAME_ADDR

EGR164_COMMON_REGS_NAME_ADDR = [
                        ('EGR164_STBYPAS_CTRL',                                    '0x49004042'),
                        ('EGR164_PKTINFLIGHT_STS',                                 '0x49004045'),
                       ] #EGR164_COMMON_REGS_NAME_ADDR

EGR164_COMMON_L2_REGS_NAME_ADDR = [
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM0',                           '0x4600FF00'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM1',                           '0x4600FF04'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM2',                           '0x4600FF08'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM3',                           '0x4600FF0C'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM4',                           '0x4600FF10'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM5',                           '0x4600FF14'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM6',                           '0x4600FF18'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM7',                           '0x4600FF1C'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM8',                           '0x4600FF20'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM9',                           '0x4600FF24'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM10',                          '0x4600FF28'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM11',                          '0x4600FF2C'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM12',                          '0x4600FF30'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM13',                          '0x4600FF34'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM14',                          '0x4600FF38'),
                        ('EGR164_SYSCTRL_ECC_CNTR_RAM15',                          '0x4600FF3C'),
                        ('EGR164_SYSCTRL_ECC_CORR_ENABLE',                         '0x4600FF00'),
                        ('EGR164_SYSCTRL_ECC_DERR_ENABLE',                         '0x4600FF00'),
                        ('EGR164_SYSCTRL_INFLIGHT_GL',                             '0x4600FFE0'),
                        ('EGR164_SYSCTRL_CLOCK_STATE',                             '0x4600FFE8'),
                        ('EGR164_SYSCTRL_FORCE_CLOCK_ON',                          '0x4600FFEC'),
                        ('EGR164_SYSCTRL_FORCE_CLOCK_OFF',                         '0x4600FFF0'),
                       ] #EGR164_COMMON_L2_REGS_NAME_ADDR

EGR164_PER_CH_REGS_NAME_ADDR = [
                       ] #EGR164_COMMON_REGS_NAME_ADDR

EGR164_PER_CH_L2_REGS_NAME_ADDR = [
                        ('EGR164_SYSCTRL_CHANNEL_CTRL_CHAN',                       '0x4600FE00'),
                       ] #EGR164_PER_CH_L2_REGS_NAME_ADDR

ING163_COMMON_REGS_NAME_ADDR = [
                        ('ING163_STBYPAS_CTRL',                                    '0x49003002'),
                        ('ING163_PKTINFLIGHT_STS',                                 '0x49003005'),
                       ] #ING163_COMMON_REGS_NAME_ADDR

ING163_COMMON_L2_REGS_NAME_ADDR = [
                        ('ING163_SYSCTRL_ECC_CNTR_RAM0',                           '0x4300FF00'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM1',                           '0x4300FF04'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM2',                           '0x4300FF08'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM3',                           '0x4300FF0C'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM4',                           '0x4300FF10'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM5',                           '0x4300FF14'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM6',                           '0x4300FF18'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM7',                           '0x4300FF1C'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM8',                           '0x4300FF20'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM9',                           '0x4300FF24'),
                        ('ING163_SYSCTRL_ECC_CNTR_RAM10',                          '0x4300FF28'),
                        ('ING163_SYSCTRL_ECC_CORR_ENABLE',                         '0x4300FF60'),
                        ('ING163_SYSCTRL_ECC_DERR_ENABLE',                         '0x4300FF64'),
                        ('ING163_SYSCTRL_CLOCK_STATE',                             '0x4300FFE8'),
                        ('ING163_SYSCTRL_FORCE_CLOCK_ON',                          '0x4300FFEC'),
                        ('ING163_SYSCTRL_FORCE_CLOCK_OFF',                         '0x4300FFF0'),
                       ] #ING163_COMMON_L2_REGS_NAME_ADDR

ING163_PER_CH_REGS_NAME_ADDR = [
                        ('ING163_SYSCTRL_CHANNEL_CTRL',                            '0x4300FE00'),
#                       ('ING163_CHSETTINGS_CP_MATCH_MODE_1',                      '0x4300A040'),
#                       ('ING163_CHSETTINGS_CP_MATCH_ENABLE_1',                    '0x4300A044'),
#                       ('ING163_CHSETTINGS_SAM_NM_PARAMS_1',                      '0x4300A048'),
#                       ('ING163_CHSETTINGS_SAM_POLICY_1',                         '0x4300A04C'),
                       ] #ING163_COMMON_REGS_NAME_ADDR

ING163_PER_CH_L2_REGS_NAME_ADDR = [
                        ('ING163_CH_TCAMHitMultiple_lo_CHAN',                      '0x4300E000'),
                        ('ING163_CH_TCAMHitMultiple_hi_CHAN',                      '0x4300E004'),
                        ('ING163_CH_HeaderParserDroppedPkts_lo_CHAN',              '0x4300E008'),
                        ('ING163_CH_HeaderParserDroppedPkts_hi_CHAN',              '0x4300E00C'),
                        ('ING163_CH_CNTR_TCAMMiss_lo_CHAN',                        '0x4300E010'),
                        ('ING163_CH_CNTR_TCAMMiss_hi_CHAN',                        '0x4300E014'),
                        ('ING163_CH_CNTR_PktsCtrl_lo_CHAN',                        '0x4300E018'),
                        ('ING163_CH_CNTR_PktsCtrl_hi_CHAN',                        '0x4300E01C'),
                        ('ING163_CH_CNTR_PktsData_lo_CHAN',                        '0x4300E020'),
                        ('ING163_CH_CNTR_PktsData_hi_CHAN',                        '0x4300E024'),
                        ('ING163_CH_CNTR_PktsDropped_lo_CHAN',                     '0x4300E028'),
                        ('ING163_CH_CNTR_PktsDropped_hi_CHAN',                     '0x4300E02C'),
                        ('ING163_CH_CNTR_PktsErrIn_lo_CHAN',                       '0x4300E030'),
                        ('ING163_CH_CNTR_PktsErrIn_hi_CHAN',                       '0x4300E034'),
                       ] #ING163_PER_CH_L2_REGS_NAME_ADDR

ING164_COMMON_REGS_NAME_ADDR = [
                        ('ING164_STBYPAS_CTRL',                                    '0x49003042'),
                        ('ING164_PKTINFLIGHT_STS',                                 '0x49003045'),
                       ] #ING164_COMMON_REGS_NAME_ADDR

ING164_COMMON_L2_REGS_NAME_ADDR = [
                        ('ING164_SYSCTRL_ECC_CNTR_RAM0',                           '0x4400FF00'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM1',                           '0x4400FF04'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM2',                           '0x4400FF08'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM3',                           '0x4400FF0C'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM4',                           '0x4400FF10'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM5',                           '0x4400FF14'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM6',                           '0x4400FF18'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM7',                           '0x4400FF1C'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM8',                           '0x4400FF20'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM9',                           '0x4400FF24'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM10',                          '0x4400FF28'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM11',                          '0x4400FF2C'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM12',                          '0x4400FF30'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM13',                          '0x4400FF34'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM14',                          '0x4400FF38'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM15',                          '0x4400FF3C'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM16',                          '0x4400FF40'),
                        ('ING164_SYSCTRL_ECC_CNTR_RAM17',                          '0x4400FF44'),
                        ('ING164_SYSCTRL_ECC_CORR_ENABLE',                         '0x4400FF60'),
                        ('ING164_SYSCTRL_ECC_DERR_ENABLE',                         '0x4400FF64'),
                        ('ING164_SYSCTRL_INFLIGHT_GL',                             '0x4400FFE0'),
                        ('ING164_SYSCTRL_CLOCK_STATE',                             '0x4400FFE8'),
                        ('ING164_SYSCTRL_FORCE_CLOCK_ON',                          '0x4400FFEC'),
                        ('ING164_SYSCTRL_FORCE_CLOCK_OFF',                         '0x4400FFF0'),
                       ] #ING164_COMMON_L2_REGS_NAME_ADDR

ING164_PER_CH_REGS_NAME_ADDR = [
                       ] #ING164_COMMON_REGS_NAME_ADDR

ING164_PER_CH_L2_REGS_NAME_ADDR = [
                        ('ING164_SYSCTRL_CHANNEL_CTRL_CHAN',                       '0x4400FE00'),
                       ] #ING164_PER_CH_L2_REGS_NAME_ADDR
#
#  REG-NAME                                     PORT-0   PORT-1   PORT-2   PORT-3   PORT-4   PORT-5   PORT-6   PORT-7
#  -------------------------------------------  -------  -------  -------  -------  -------  -------  -------  -------
#  EGR163_STBYPAS_CTRL                                                                                                     // (Common)
#  EGR163_PKTINFLIGHT_STS                                                                                                  // (Common)
#  EGR163_SYSCTRL_CHANNEL_CTRL                                                                                             // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM0                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM1                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM2                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM3                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM4                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM5                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM6                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM7                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM8                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM9                                                                                            // (Common) L2
#  EGR163_SYSCTRL_ECC_CNTR_RAM10                                                                                           // (Common) L2
#  EGR163_SYSCTRL_ECC_CORR_ENABLE                                                                                          // (Common) L2
#  EGR163_SYSCTRL_ECC_DERR_ENABLE                                                                                          // (Common) L2
#  EGR163_SYSCTRL_CLOCK_STATE                                                                                              // (Common) L2
#  EGR163_SYSCTRL_FORCE_CLOCK_ON                                                                                           // (Common) L2
#  EGR163_SYSCTRL_FORCE_CLOCK_OFF                                                                                          // (Common) L2
#  EGR163_CH_TCAMHitMultiple_lo                                                                                            // (Per-Ch) L2
#  EGR163_CH_TCAMHitMultiple_hi                                                                                            // (Per-Ch) L2
#  EGR163_CH_HeaderParserDroppedPkts_lo                                                                                    // (Per-Ch) L2
#  EGR163_CH_HeaderParserDroppedPkts_hi                                                                                    // (Per-Ch) L2
#  EGR163_CH_CNTR_TCAMMiss_lo                                                                                              // (Per-Ch) L2
#  EGR163_CH_CNTR_TCAMMiss_hi                                                                                              // (Per-Ch) L2
#  EGR163_CH_CNTR_PktsCtrl_lo                                                                                              // (Per-Ch) L2
#  EGR163_CH_CNTR_PktsCtrl_hi                                                                                              // (Per-Ch) L2
#  EGR163_CH_CNTR_PktsData_lo                                                                                              // (Per-Ch) L2
#  EGR163_CH_CNTR_PktsData_hi                                                                                              // (Per-Ch) L2
#  EGR163_CH_CNTR_PktsDropped_lo                                                                                           // (Per-Ch) L2
#  EGR163_CH_CNTR_PktsDropped_hi                                                                                           // (Per-Ch) L2
#  EGR163_CH_CNTR_PktsErrIn_lo                                                                                             // (Per-Ch) L2
#  EGR163_CH_CNTR_PktsErrIn_hi                                                                                             // (Per-Ch) L2
#
#  EGR164_STBYPAS_CTRL                                                                                                     // (Common)
#  EGR164_PKTINFLIGHT_STS                                                                                                  // (Common)
#  EGR164_SYSCTRL_CHANNEL_CTRL                                                                                             // (Per-Ch) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM0                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM1                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM2                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM3                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM4                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM5                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM6                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM7                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM8                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM9                                                                                            // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM10                                                                                           // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM11                                                                                           // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM12                                                                                           // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM13                                                                                           // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM14                                                                                           // (Common) L2
#  EGR164_SYSCTRL_ECC_CNTR_RAM15                                                                                           // (Common) L2
#  EGR164_SYSCTRL_ECC_CORR_ENABLE                                                                                          // (Common) L2
#  EGR164_SYSCTRL_ECC_DERR_ENABLE                                                                                          // (Common) L2
#  EGR164_SYSCTRL_INFLIGHT_GL                                                                                              // (Common) L2
#  EGR164_SYSCTRL_CLOCK_STATE                                                                                              // (Common) L2
#  EGR164_SYSCTRL_FORCE_CLOCK_ON                                                                                           // (Common) L2
#  EGR164_SYSCTRL_FORCE_CLOCK_OFF                                                                                          // (Common) L2
#
#
## MACSEC (Ingres) ...
## -------------------
#
#  REG-NAME                                     PORT-0   PORT-1   PORT-2   PORT-3   PORT-4   PORT-5   PORT-6   PORT-7
#  -------------------------------------------  -------  -------  -------  -------  -------  -------  -------  -------
#  ING163_STBYPAS_CTRL                                                                                                     // (Common)
#  ING163_PKTINFLIGHT_STS                                                                                                  // (Common)
#  ING163_SYSCTRL_CHANNEL_CTRL                                                                                             // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM0                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM1                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM2                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM3                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM4                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM5                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM6                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM7                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM8                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM9                                                                                            // (Common) L2
#  ING163_SYSCTRL_ECC_CNTR_RAM10                                                                                           // (Common) L2
#  ING163_SYSCTRL_ECC_CORR_ENABLE                                                                                          // (Common) L2
#  ING163_SYSCTRL_ECC_DERR_ENABLE                                                                                          // (Common) L2
#  ING163_SYSCTRL_CLOCK_STATE                                                                                              // (Common) L2
#  ING163_SYSCTRL_FORCE_CLOCK_ON                                                                                           // (Common) L2
#  ING163_SYSCTRL_FORCE_CLOCK_OFF                                                                                          // (Common) L2
#  ING163_CH_TCAMHitMultiple_lo                                                                                            // (Per-Ch) L2
#  ING163_CH_TCAMHitMultiple_hi                                                                                            // (Per-Ch) L2
#  ING163_CH_HeaderParserDroppedPkts_lo                                                                                    // (Per-Ch) L2
#  ING163_CH_HeaderParserDroppedPkts_hi                                                                                    // (Per-Ch) L2
#  ING163_CH_CNTR_TCAMMiss_lo                                                                                              // (Per-Ch) L2
#  ING163_CH_CNTR_TCAMMiss_hi                                                                                              // (Per-Ch) L2
#  ING163_CH_CNTR_PktsCtrl_lo                                                                                              // (Per-Ch) L2
#  ING163_CH_CNTR_PktsCtrl_hi                                                                                              // (Per-Ch) L2
#  ING163_CH_CNTR_PktsData_lo                                                                                              // (Per-Ch) L2
#  ING163_CH_CNTR_PktsData_hi                                                                                              // (Per-Ch) L2
#  ING163_CH_CNTR_PktsDropped_lo                                                                                           // (Per-Ch) L2
#  ING163_CH_CNTR_PktsDropped_hi                                                                                           // (Per-Ch) L2
#  ING163_CH_CNTR_PktsErrIn_lo                                                                                             // (Per-Ch) L2
#  ING163_CH_CNTR_PktsErrIn_hi                                                                                             // (Per-Ch) L2
#
#  ING164_STBYPAS_CTRL                                                                                                     // (Common)
#  ING164_PKTINFLIGHT_STS                                                                                                  // (Common)
#  ING164_SYSCTRL_CHANNEL_CTRL                                                                                             // (Per-Ch) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM0                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM1                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM2                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM3                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM4                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM5                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM6                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM7                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM8                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM9                                                                                            // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM10                                                                                           // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM11                                                                                           // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM12                                                                                           // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM13                                                                                           // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM14                                                                                           // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM15                                                                                           // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM16                                                                                           // (Common) L2
#  ING164_SYSCTRL_ECC_CNTR_RAM17                                                                                           // (Common) L2
#  ING164_SYSCTRL_ECC_CORR_ENABLE                                                                                          // (Common) L2
#  ING164_SYSCTRL_ECC_DERR_ENABLE                                                                                          // (Common) L2
#  ING164_SYSCTRL_INFLIGHT_GL                                                                                              // (Common) L2
#  ING164_SYSCTRL_CLOCK_STATE                                                                                              // (Common) L2
#  ING164_SYSCTRL_FORCE_CLOCK_ON                                                                                           // (Common) L2
#  ING164_SYSCTRL_FORCE_CLOCK_OFF                                                                                          // (Common) L2

#<EOF>

