## global
chip_name               = 'Bald_Eagle'
chip_rst_addr           = [0x9802, [11,0]]
chip_soft_rst_val       = 0x888
chip_logic_rst_val      = 0x777
chip_cpu_rst_val        = 0xAAA
chip_reg_rst_val        = 0x999

## Firmware related registers
fw_load_magic_word_addr = [0x9805, [15,0]]
fw_load_magic_word      = 0x6A6A

fw_unload_addr          = [0x9805, [15,0]]
fw_unload_word          = 0xFFF0

fw_cmd_addr             = [0x9806, [15,0]]
fw_cmd_detail_addr      = [0x9807, [15,0]]
fw_cmd_status_addr      = [0x98C7, [15,0]]
fw_opt_done_addr        = [0x98C9, [15,0]]

fw_hash_read_en_addr    = [0x9806, [15,0]]
fw_hash_word_hi_addr    = [0x9806, [7,0]]
fw_hash_word_lo_addr    = [0x9807, [15,0]]
fw_hash_read_en         = 0xF000  
fw_hash_read_status     = 0x0F00

fw_crc_read_en_addr     = [0x9806, [15,0]]
fw_crc_word_addr        = [0x9807, [15,0]]
fw_crc_read_en          = 0xF001  
fw_crc_read_status      = 0x0F00

fw_date_read_en_addr    = [0x9806, [15,0]]
fw_date_word_addr       = [0x9807, [15,0]]
fw_date_read_en         = 0xF002  
fw_date_read_status     = 0x0F00

fw_ver_read_en_addr     = [0x9806, [15,0]]
fw_ver_word_hi_addr     = [0x9806, [7,0]]
fw_ver_word_lo_addr     = [0x9807, [15,0]]
fw_ver_read_en          = 0xF003
fw_ver_read_status      = 0x0F00
            
fw_watchdog_timer_addr  = [0x98C6, [15,0]]

fw_halt_addr            = [0x9805, [15,0]]
fw_halt_en              = 0xD000  
fw_halt_status          = 0x0D00

fw_config_lane_off      = 0x9000  
fw_config_lane_active   = 0x8000  
fw_config_lane_status   = 0x0800

fw_debug_info_cmd       = 0xb000  
fw_debug_info_status    = 0x0b00  

## TOP PLL A (0x9500) & TOP PLL B (0x9600)
top_pll_en_refclk_addr  = [0x9500, [7]]
top_pll_pu_addr         = [0x9501, [2]]
top_pll_div4_addr       = [0x9500, [6]]
top_pll_n_addr          = [0x9507, [12,4]]
top_pll_lvcocap_addr    = [0x9501, [12,6]]
top_pll_refclk_div_addr = [0x9513, [15,7]] 
top_pll_div2_addr       = [0x9513, [6]]

## Phoenix3 PLL RX
rx_pll_pu_addr          = [0x1FD, [0]]
rx_pll_div4_addr        = [0x1FF, [6]]
rx_pll_n_addr           = [0x1FD, [15,7]]
rx_pll_lvcocap_addr     = [0x1F5, [15,9]] 
rx_pll_div2_addr        = [0x1F5, [8]]
rx_pll_frac_n_addr      = [0x1F1, [15,0]]
rx_pll_frac_order_addr  = [0x1F0, [15,14]]
rx_pll_frac_en_addr     = [0x1F0, [13]]

rx_en_vcominbuf_addr    = [0x1e7, [11]]
rx_vagccom_addr         = [0x1e7, [10,8]]
rx_vagccom2_addr        = [0x1e7, [7,5]]
rx_vgavdsat_addr        = [0x1e6, [15,12]]
rx_vrefagcreg_addr      = [0x1e6, [5,3]]
rx_vref2agcreg_addr     = [0x1e6, [2,0]]
rx_vcomrefsel_ex_addr   = [0x1dd, [9]]
rx_skef_en_addr         = [0x1dd, [8]]
rx_skef_degen_addr      = [0x1dd, [7,5]]
rx_skef_addcap_addr     = [0x1dd, [4,2]]
rx_agc_degen_addr       = [0x1d7, [3,2]]
rx_agcgain1_addr        = [0x1d4, [15,9]] # 7-bit gray coded
rx_agcgain2_addr        = [0x1d4, [8,4]]  # 5-bit gray coded

## Phoenix3 ana reg
tx_pll_pu_addr          = [0x0FE, [0]]
tx_pll_div2_addr        = [0x0FF, [1]]
tx_pll_div4_addr        = [0x0FF, [0]]
tx_pll_n_addr           = [0x0fe, [15,7]]
tx_pll_lvcocap_addr     = [0x0db, [14,8]]
tx_pll_frac_n_addr      = [0x0D8, [15,0]]
tx_pll_frac_order_addr  = [0x0D7, [15,14]]
tx_pll_frac_en_addr     = [0x0D7, [13]]

rx_edge1_addr           = [0x0ed, [15,12]]
rx_edge2_addr           = [0x0ed, [11,8]]
rx_edge3_addr           = [0x0ed, [7,4]]
rx_edge4_addr           = [0x0ed, [3,0]]

## Phoenix + Eagle tx digital top
tx_test_patt_sc_addr    = [0x0a0, [15]]
tx_prbs_clk_en_addr     = [0x0a0, [14]]
tx_test_patt_en_addr    = [0x0a0, [13]]
tx_prbs_en_addr         = [0x0a0, [11]]
tx_prbs_1b_err_addr     = [0x0a0, [10]]
tx_prbs_patt_sel_addr   = [0x0a0, [9,8]] # nrz:00=prbs9, 01=prbs15, 10=prbs23, 11=prbs31 # pam4:00=prbs9, 01=prbs13, 10=prbs15, 11=prbs31
tx_pol_addr             = [0x0a0, [5]] # analog output flip
tx_patt_mode_addr       = [0x0a0, [3,2]] # 00=pattern_mem, 01=jp03b, 10=linear pattern
tx_test_patt4_addr      = [0x0A1, [15,0]] # msb
tx_test_patt3_addr      = [0x0A2, [15,0]]
tx_test_patt2_addr      = [0x0A3, [15,0]]
tx_test_patt1_addr      = [0x0A4, [15,0]] # lsb
tx_tap1_addr            = [0x0A5, [15,8]] 
tx_tap2_addr            = [0x0A7, [15,8]]
tx_tap3_addr            = [0x0A9, [15,8]] 
tx_tap4_addr            = [0x0ab, [15,8]] 
tx_tap5_addr            = [0x0ad, [15,8]]
tx_tap6_addr            = [0x0b1, [15,12]]
tx_tap7_addr            = [0x0b1, [11,8]]
tx_tap8_addr            = [0x0b1, [7,4]]
tx_tap9_addr            = [0x0b1, [3,0]]
tx_tap10_addr           = [0x0b2, [15,12]]
tx_tap11_addr           = [0x0b2, [11,8]] 
tx_taps_sum_limit       = 31
tx_msb_swap_addr        = [0x0af, [10]]
tx_gray_en_addr         = [0x0af, [9]]
tx_precoder_en_addr     = [0x0af, [8]]
tx_taps_hf_addr         = [0x0af, [5,1]]
tx_prbs_1b_err_nrz_addr = [0x0b0, [10]]

## PAM4 / NRZ / NRZ-HalfRate Parameters
## Need these 6 in both Eagle/Phoenix Register definition files
rx_pam4_en_addr             = [0x041,[15]]
tx_prbs_clk_en_nrz_addr     = [0x0b0,[14]]
tx_prbs_en_nrz_addr         = [0x0b0,[11]]
tx_nrz_mode_addr            = [0x0b0, [1]]
tx_mode10g_en_addr          = [0x0b0, [0]]
rx_mode10g_addr             = [0x179, [0]]

## Phoenix state machine
rx_lane_rst_addr            = [0x000, [15]]
rx_cntr_target_init_addr    = [0x001, [15,0]]
rx_cntr_target_final_addr   = [0x002, [15,0]]
rx_timer_meas_s6_addr       = [0x007, [6,3]]
rx_fixed_patt_b0_addr       = [0x007, [2,1]]
rx_fixed_patt_dir_addr      = [0x007, [0]]
rx_iter_s6_addr             = [0x009, [3,0]]
                                 
rx_of_period_addr           = [0x00a, [9,5]]
rx_hf_period_addr           = [0x00a, [4,0]]
rx_of_ths_addr              = [0x00b, [14,8]]
rx_hf_ths_addr              = [0x00b, [6,0]]
rx_of_cntr_upper_limit_addr = [0x00c, [15,0]]
rx_of_cntr_lower_limit_addr = [0x00d, [15,0]]
rx_hf_cntr_target_addr      = [0x00e, [15,0]]

rx_bp1_en_addr              = [0x011, [15]]
rx_bp1_st_addr              = [0x011, [12,8]]
rx_bp2_en_addr              = [0x011, [14]]
rx_bp2_st_addr              = [0x011, [4,0]]
rx_sm_cont_addr             = [0x011, [13]]
rx_delta_owen_addr          = [0x012, [13]]
rx_delta_ow_addr            = [0x012, [12,6]]
rx_kp_owen_addr             = [0x020, [9]]
rx_kp_ow_addr               = [0x020, [8,6]]

rx_acal_clk_en_owen_addr    = [0x018, [11]]
rx_acal_clk_en_ow_addr      = [0x018, [10]]
rx_acal_start_ow_addr       = [0x01f, [0]]
rx_acal_start_owen_addr     = [0x01f, [1]]
rx_acal_done_ow_addr        = [0x023, [2]]
rx_acal_done_owen_addr      = [0x023, [3]]

rx_dac_ow_addr              = [0x021, [11,8]]
rx_dac_owen_addr            = [0x021, [12]]
rx_agc_ow_addr              = [0x021, [6,4]]
rx_agc_owen_addr            = [0x021, [7]]
rx_margin_patt_dis_owen_addr= [0x021, [3]]
rx_margin_patt_dis_ow_addr  = [0x021, [2]]
rx_bp1_reached_addr         = [0x028, [15]]
rx_bp2_reached_addr         = [0x028, [14]]
rx_state_addr               = [0x028, [13,9]]
rx_dac_addr                 = [0x028, [8,5]]

rx_state_cntr_addr          = [0x02d, [15,0], 0x2e, [15,0]]
rx_mj_addr                  = [0x02e, [1,0]]

rx_cntr_sel_addr            = [0x030, [7,4]]

rx_plus_margin_addr         = [0x032, [15,4]]
rx_minus_margin_addr        = [0x032, [3,0], 0x33, [15,8]]
rx_c_plus_margin_addr       = [0x033, [7,0], 0x34, [15,12]]
rx_c_minus_margin_addr      = [0x034, [11,0]]

rx_fixed_patt_plus_margin_addr  = [0x036, [7,0], 0x37, [15,12]]
rx_fixed_patt_minus_margin_addr = [0x037, [11,0]]
rx_em_addr                  = [0x038, [11,0]]
rx_delta_addr               = [0x040, [13,7]]
rx_pam4_tap_addr            = [0x040,[6,0]] # pam4 fm1

rx_mu_ow_addr               = [0x087, [10,9]]
rx_mu_owen_addr             = [0x087, [11]]

rx_plus_margin_sel_addr     = [0x088,[15,12]]
rx_minus_margin_sel_addr    = [0x088,[11,8]]
rx_pam4_dfe_sel_addr        = [0x088, [7,4]] # Ths_q_sel
rx_pam4_dfe_rd_addr         = [0x02f, [15,4]] # read_Ths_q

## Phoenix RX digital top

rx_theta_update_mode_addr   = [0x041, [12,10]] # theta2,3,4 update mode, used delta_search()

rx_precoder_en_addr         = [0x42, [1]]
rx_gray_en_addr             = [0x42, [0]]
rx_msb_swap_addr            = [0x43, [15]]

rx_err_cntr_rst_addr        = [0x43, [0]] # use sync err cntr
rx_pol_addr                 = [0x43, [7]] # firmware flip [8], customer flip [7]
rx_prbs_checker_pu_addr     = [0x43, [3]] # use sync checker
rx_prbs_mode_addr           = [0x43, [6,5]]

rx_delay_loop_freeze_addr   = [0x044, [1]] # freeze delay loop, used delta_search

rx_fixed_patt_mode_addr     = [0x045, [6,3]]
rx_err_cntr_msb_addr        = [0x050, [15,0]]
rx_err_cntr_lsb_addr        = [0x051, [15,0]]
rx_agc_map0_addr            = [0x048, [15,0]] # highest peaking
rx_agc_map1_addr            = [0x049, [15,0]]
rx_agc_map2_addr            = [0x04a, [15,0]] # lowest peaking

rx_sd_addr                  = [0x06a, [7]]
rx_rdy_addr                 = [0x06a, [15]]
rx_cnt3_addr                = [0x06f, [15,0]]
rx_cnt0_addr                = [0x070, [15,0]]
rx_hf_cnt_addr              = [0x072, [15,0]]
rx_freq_accum_addr          = [0x073, [10,0]]
rx_ted_en_addr              = [0x079, [13]]

# ## FFE related registers
rx_ffe_nbias_main_addr      = [0x1d6,[15,12]]
rx_ffe_nbias_sum_addr       = [0x1d6,[11,8]]

# rx_ffe_g11_addr           = [0xf0, [15,12]]
# rx_ffe_g12_addr           = [0xf0, [11,8]]
# rx_ffe_g21_addr           = [0xf0, [7,4]]
# rx_ffe_g22_addr           = [0xf0, [3,0]]
# rx_ffe_g31_addr           = [0xef, [15,12]]
# rx_ffe_g32_addr           = [0xef, [11,8]]
# rx_ffe_g41_addr           = [0xef, [7,4]]
# rx_ffe_g42_addr           = [0xef, [3,0]]
# rx_ffe_g51_addr           = [0xee, [15,12]]
# rx_ffe_g52_addr           = [0xee, [11,8]]
# rx_ffe_g61_addr           = [0xee, [7,4]]
# rx_ffe_g62_addr           = [0xee, [3,0]]
# rx_ffe_g71_addr           = [0xed, [15,12]]
# rx_ffe_g72_addr           = [0xed, [11,8]]
# rx_ffe_g1_pu_addr         = [0xf8, [14]]
# rx_ffe_g2_pu_addr         = [0xf8, [13]]

rx_ffe_k3_msb_addr          = [0x1e4, [15,12]]
rx_ffe_k3_lsb_addr          = [0x1e4, [11,8]]
rx_ffe_k4_msb_addr          = [0x1e4, [7,4]]
rx_ffe_k4_lsb_addr          = [0x1e4, [3,0]]
            
rx_ffe_k2_msb_addr          = [0x1e3, [15,12]]
rx_ffe_k2_lsb_addr          = [0x1e3, [11,8]]
rx_ffe_s2_msb_addr          = [0x1e3, [7,4]]
rx_ffe_s2_lsb_addr          = [0x1e3, [3,0]]
            
rx_ffe_k1_msb_addr          = [0x1e2, [15,12]]
rx_ffe_k1_lsb_addr          = [0x1e2, [11,8]]
rx_ffe_s1_msb_addr          = [0x1e2, [7,4]]
rx_ffe_s1_lsb_addr          = [0x1e2, [3,0]]
            
rx_ffe_sf_msb_addr          = [0x1e1, [15,12]]
rx_ffe_sf_lsb_addr          = [0x1e1, [7,4]]
            
rx_ffe_k4_pu_addr           = [0x1e0, [15]]
rx_ffe_k3_pu_addr           = [0x1e0, [14]]
rx_ffe_k2_pu_addr           = [0x1e0, [13]]
rx_ffe_k1_pu_addr           = [0x1e0, [12]]
rx_ffe_s2_pu_addr           = [0x1e0, [11]]
rx_ffe_s1_pu_addr           = [0x1e0, [10]]
            
rx_ffe_pol1_addr            = [0x1e0, [9]]
rx_ffe_pol2_addr            = [0x1e0, [8]]
rx_ffe_pol3_addr            = [0x1e0, [7]]
rx_ffe_pol4_addr            = [0x1e0, [6]]

# ## RX PAM4 registers
rx_f1over3_addr             = [0x004, [11,5]]
rx_input_mode_addr          = [0x1e7, [11]]




