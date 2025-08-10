## global
chip_name                       = 'Bald_Eagle'
chip_rst_addr                   = [0x9802, [11,0]]
chip_soft_rst_val               = 0x888
chip_logic_rst_val              = 0x777
chip_cpu_rst_val                = 0xAAA
chip_reg_rst_val                = 0x999

### Firmware related registers
fw_load_magic_word_addr         = [0x9805, [15,0]]
fw_load_magic_word              = 0x6A6A
            
fw_unload_addr                  = [0x9805, [15,0]]
fw_unload_word                  = 0xFFF0
            
fw_cmd_addr                     = [0x9806, [15,0]]
fw_cmd_detail_addr              = [0x9807, [15,0]]
fw_cmd_status_addr              = [0x98C7, [15,0]]
fw_opt_done_addr                = [0x98C9, [15,0]]
            
fw_hash_read_en_addr            = [0x9806, [15,0]]
fw_hash_word_hi_addr            = [0x9806, [7,0]]
fw_hash_word_lo_addr            = [0x9807, [15,0]]
fw_hash_read_en                 = 0xF000  
fw_hash_read_status             = 0x0F00
            
fw_crc_read_en_addr             = [0x9806, [15,0]]
fw_crc_word_addr                = [0x9807, [15,0]]
fw_crc_read_en                  = 0xF001  
fw_crc_read_status              = 0x0F00
            
fw_date_read_en_addr            = [0x9806, [15,0]]
fw_date_word_addr               = [0x9807, [15,0]]
fw_date_read_en                 = 0xF002  
fw_date_read_status             = 0x0F00
            
fw_ver_read_en_addr            = [0x9806, [15,0]]
fw_ver_word_hi_addr            = [0x9806, [7,0]]
fw_ver_word_lo_addr            = [0x9807, [15,0]]
fw_ver_read_en                 = 0xF003
fw_ver_read_status             = 0x0F00
            
fw_watchdog_timer_addr          = [0x98C6, [15,0]]
            
fw_halt_addr                    = [0x9805, [15,0]]
fw_halt_en                      = 0xD000  
fw_halt_status                  = 0x0D00
            
fw_config_lane_off              = 0x9000   
fw_config_lane_active           = 0x8000  
fw_config_lane_status           = 0x0800
            
fw_debug_info_cmd               = 0xb000  
fw_debug_info_status            = 0x0b00 

## TOP PLL A (0x9500) & TOP PLL B (0x9600)
top_pll_en_refclk_addr          = [0x9500, [7]]
top_pll_pu_addr                 = [0x9501, [2]]
top_pll_div4_addr               = [0x9500, [6]]
top_pll_n_addr                  = [0x9507, [12,4]]
top_pll_lvcocap_addr            = [0x9501, [12,6]] 
top_pll_refclk_div_addr         = [0x9513, [15,7]]
top_pll_div2_addr               = [0x9513, [6]]
            
## Eagle PLL RX          
rx_pll_pu_addr                  = [0x1FD, [0]]
rx_pll_div4_addr                = [0x1FF, [6]]
rx_pll_n_addr                   = [0x1FD, [15,7]]
rx_pll_lvcocap_addr             = [0x1F5, [15,9]]
rx_pll_div2_addr                = [0x1F5, [8]]
rx_pll_frac_n_addr              = [0x1F1, [15,0]]
rx_pll_frac_order_addr          = [0x1F0, [15,14]]
rx_pll_frac_en_addr             = [0x1F0, [13]]

rx_en_vcominbuf_addr            = [0x1e7, [11]]   
rx_vagccom_addr                 = [0x1e7, [10,8]]
rx_vagccom2_addr                = [0x1e7, [7,5]]
rx_vgavdsat_addr                = [0x1e6, [15,12]]
rx_vrefagcreg_addr              = [0x1e6, [5,3]]
rx_vref2agcreg_addr             = [0x1e6, [2,0]]
rx_vcomrefsel_ex_addr           = [0x1dd, [9]]
rx_skef_en_addr                 = [0x1dd, [8]]
rx_skef_degen_addr              = [0x1dd, [7,5]]
rx_skef_addcap_addr             = [0x1dd, [4,2]]
rx_agcgain1_addr                = [0x1d4, [15,9]] # 7-bit gray coded
rx_agcgain2_addr                = [0x1d4, [8,4]]  # 5-bit gray coded  

## Eagle ana reg         
tx_pll_pu_addr                  = [0x0FE, [0]]
tx_pll_div2_addr                = [0x0FF, [1]]
tx_pll_div4_addr                = [0x0FF, [0]]
tx_pll_n_addr                   = [0x0fe, [15,7]]
tx_pll_lvcocap_addr             = [0x0db, [14,8]]
tx_pll_frac_n_addr              = [0x0D8, [15,0]]
tx_pll_frac_order_addr          = [0x0D7, [15,14]]
tx_pll_frac_en_addr             = [0x0D7, [13]]
            
rx_edge1_addr                   = [0x0ed, [15,12]]
rx_edge2_addr                   = [0x0ed, [11,8]]
rx_edge3_addr                   = [0x0ed, [7,4]]
rx_edge4_addr                   = [0x0ed, [3,0]]

## Phoenix + Eagle tx digital top
tx_test_patt_sc_addr            = [0x0a0, [15]]
tx_prbs_clk_en_addr             = [0x0a0, [14]]
tx_test_patt_en_addr            = [0x0a0, [13]]
tx_prbs_en_addr                 = [0x0a0, [11]]
tx_prbs_1b_err_addr             = [0x0a0, [10]]
tx_prbs_patt_sel_addr           = [0x0a0, [9,8]] # nrz:00=prbs9, 01=prbs15, 10=prbs23, 11=prbs31 # pam4:00=prbs9, 01=prbs13, 10=prbs15, 11=prbs31
tx_pol_addr                     = [0x0a0, [5]] # analog output flip
tx_patt_mode_addr               = [0x0a0, [3,2]] # 00=pattern_mem, 01=jp03b, 10=linear pattern
tx_test_patt4_addr              = [0x0A1, [15,0]] # msb
tx_test_patt3_addr              = [0x0A2, [15,0]]
tx_test_patt2_addr              = [0x0A3, [15,0]]
tx_test_patt1_addr              = [0x0A4, [15,0]] # lsb
tx_tap1_addr                    = [0x0A5, [15,8]] 
tx_tap2_addr                    = [0x0A7, [15,8]]
tx_tap3_addr                    = [0x0A9, [15,8]] 
tx_tap4_addr                    = [0x0ab, [15,8]] 
tx_tap5_addr                    = [0x0ad, [15,8]]
tx_tap6_addr                    = [0x0b1, [15,12]]
tx_tap7_addr                    = [0x0b1, [11,8]]
tx_tap8_addr                    = [0x0b1, [7,4]]
tx_tap9_addr                    = [0x0b1, [3,0]]
tx_tap10_addr                   = [0x0b2, [15,12]]
tx_tap11_addr                   = [0x0b2, [11,8]] 
tx_taps_sum_limit               = 31
tx_msb_swap_addr                = [0x0af, [10]]
tx_gray_en_addr                 = [0x0af, [9]]
tx_precoder_en_addr             = [0x0af, [8]]
tx_taps_hf_addr                 = [0x0af, [5,1]]            
tx_prbs_1b_err_nrz_addr         = [0x0b0, [10]]

## PAM4 / NRZ / NRZ-HalfRate Parameters
## Need these 6 in both Eagle/Phoenix Register definition files
rx_pam4_en_addr                 = [0x041,[15]]
tx_prbs_clk_en_nrz_addr         = [0x0b0,[14]]
tx_prbs_en_nrz_addr             = [0x0b0,[11]]
tx_nrz_mode_addr                = [0x0b0, [1]]
tx_mode10g_en_addr              = [0x0b0, [0]]
rx_mode10g_addr                 = [0x179, [0]]
rx_ted_en_addr                  = [0x079, [13]]

## Eagle state machine
rx_cntr_target_addr             = [0x102, [15,4]]
rx_delta_adapt_en_addr          = [0x101, [14]]
rx_timer_meas_s6_addr           = [0x108, [7,4]]
rx_em_en_addr                   = [0x109, [15]]
rx_max_eye_delta_owen_addr      = [0x10a, [14]]
rx_delta_owen_addr              = [0x10a, [12]]
rx_bp1_en_addr                  = [0x10b, [15]]
rx_bp1_st_addr                  = [0x10b, [12,8]]
rx_bp2_en_addr                  = [0x10b, [7]]
rx_bp2_st_addr                  = [0x10b, [4,0]]
rx_sm_cont_addr                 = [0x10c, [15]]
rx_acal_done_ow_addr            = [0x10c, [0]]
rx_acal_done_owen_addr          = [0x10c, [1]]
rx_bp1_reached_addr             = [0x10d, [15]]
rx_bp2_reached_addr             = [0x10d, [14]]
rx_state_addr                   = [0x10d, [12,8]]
rx_delta_addr                   = [0x10d, [6,0]]
rx_state_cntr_addr              = [0x112, [15,0], 0x113, [15,0]]
rx_fixed_patt_plus_margin_addr  = [0x127, [3,0], 0x128, [15,8]]
rx_fixed_patt_minus_margin_addr = [0x128, [7,0], 0x129, [15,12]]
rx_max_eye_margin_addr          = [0x129, [11,0]]
rx_em_addr                      = [0x12a, [11,0]]
rx_max_eye_delta_addr           = [0x12b, [14,8]]
rx_f1_addr                      = [0x12b, [6,0]]
rx_f2_addr                      = [0x12c, [14,8]]
rx_f3_addr                      = [0x12c, [6,0]]
rx_kp_addr                      = [0x12e, [14,12]]
rx_kf_addr                      = [0x12e, [8,7]]
rx_sd_addr                      = [0x12e, [3]]
rx_rdy_addr                     = [0x12e, [2]]
rx_kp_owen_addr                 = [0x13b, [15]]
rx_kp_ow_addr                   = [0x13b, [14,12]]
rx_ktheta_owen_addr             = [0x13b, [10]]
rx_ktheta_ow_addr               = [0x13b, [9,8]]
rx_kf_owen_addr                 = [0x13b, [7]]
rx_kf_ow_addr                   = [0x13b, [6,5]]
rx_acal_clk_en_owen_addr        = [0x145, [15]]
rx_acal_clk_en_ow_addr          = [0x145, [7]]
rx_acal_start_ow_addr           = [0x144, [7]]
rx_acal_start_owen_addr         = [0x144, [15]]
rx_of_period_addr               = [0x14a, [11,7]]
rx_hf_period_addr               = [0x14c, [12,8]]
rx_of_ths_addr                  = [0x14a, [6,0]]
rx_hf_ths_addr                  = [0x14c, [6,0]]
rx_of_cntr_upper_limit_addr     = [0x148, [15,0]]
rx_of_cntr_lower_limit_addr     = [0x149, [15,0]]
rx_hf_cntr_target_addr          = [0x14b, [15,0]]
rx_agc_ow_addr                  = [0x14e, [13,11]]
rx_agc_owen_addr                = [0x14d, [15]]
rx_delta_ow_addr                = [0x15f, [6,0]]
rx_dac_ow_addr                  = [0x14f, [15,12]]
rx_dac_owen_addr                = [0x14c, [7]]
rx_max_eye_delta_ow_addr        = [0x15f, [14,8]]
rx_dac_addr                     = [0x17f, [15,12]]
rx_restart_cntr_addr            = [0x17f, [4,0]]
rx_lane_rst_addr                = [0x181, [11]]


# rx_fixed_patt_b0_addr         = [0x7, [2,1]]
# rx_fixed_patt_dir_addr        = [0x7, [0]]
# rx_iter_s6_addr               = [0x9, [3,0]]
# rx_margin_patt_dis_owen_addr  = [0x21, [3]]
# rx_margin_patt_dis_ow_addr    = [0x21, [2]]
# rx_plus_margin_addr           = [0x32, [15,4]]
# rx_minus_margin_addr          = [0x32, [3,0], 0x33, [15,8]]
# rx_c_plus_margin_addr         = [0x33, [7,0], 0x34, [15,12]]
# rx_c_minus_margin_addr        = [0x34, [11,0]]
# rx_pam4_tap_addr              = [0x40,[6,0]]
# rx_mu_ow_addr                 = [0x87, [10,9]]
# rx_mu_owen_addr               = [0x87, [11]]
# rx_pam4_dfe_sel_addr          = [0x88, [7,4]] # Ths_q_sel
# rx_pam4_dfe_rd_addr           = [0x2f, [15,4]] # read_Ths_q

## Eagle RX digital top
rx_err_cntr_rst_addr            = [0x161, [15]] # use sync err cntr
rx_pol_addr                     = [0x161, [14]] # firmware flip [8], customer flip [7]
rx_prbs_checker_pu_addr         = [0x161, [10]] # use sync checker
rx_prbs_mode_addr               = [0x161, [13,12]]
rx_theta_update_mode_addr       = [0x165, [7,5]]
rx_fixed_patt_mode_addr         = [0x165, [4,1]]
rx_err_cntr_msb_addr            = [0x166, [15,0]]
rx_err_cntr_lsb_addr            = [0x167, [15,0]]
rx_freq_accum_addr              = [0x173, [10,0]]
rx_top_rotator_en_addr          = [0x175, [15]]
rx_agc_map0_addr                = [0x176, [15,0]] # highest peaking
rx_agc_map1_addr                = [0x177, [15,0]]
rx_agc_map2_addr                = [0x178, [15,0]] # lowest peaking
rx_bb_en_addr                   = [0x179, [6]]
rx_patt_cnt_addr                = [0x174, [15,0]]
        
# ## FFE related registers      
rx_ffe_sf_msb_addr              = [0x1e1, [15,12]]
rx_ffe_sf_lsb_addr              = [0x1e1, [7,4]]
        
rx_f1over3_addr                 = [0x004,[11,5]]