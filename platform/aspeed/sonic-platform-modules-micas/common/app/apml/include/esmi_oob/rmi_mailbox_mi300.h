/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
 #ifndef INCLUDE_RMI_MI300_H_
 #define INCLUDE_RMI_MI300_H_

 #include "apml_common.h"
 #include "apml_err.h"

/* MI300A APML encoding count */
#define MI300A_ENCODING_SIZE		10	//!< MI300A encoding size //

/** \file rmi_mailbox_mi300.h
 * Header file for the MI300 mailbox messages supported by APML library.
 *  All required function, structure, enum, etc. definitions should be defined
 *  in this file.
 *
 *  @details  This header file contains the following:
 *  APIs prototype of the Mailbox messages for MI300 exported by the APML library.
 *  Description of the API, arguments and return values.
 *  The Error codes returned by the API.
 */

/**
 * @brief Mailbox message types defined in the APML library
 */
typedef enum {
	SET_MAX_GFX_CORE_CLOCK = 0x81,
	SET_MIN_GFX_CORE_CLOCK,
	SET_MAX_PSTATE,
	GET_PSTATES,
	GET_CURR_XGMI_PSTATE,
	SET_XGMI_PSTATE,
	UNSET_XGMI_PSTATE,
	GET_XGMI_PSTATES,
	GET_XCC_IDLE_RESIDENCY,
	GET_ENERGY_ACCUMULATOR = 0x90,
	GET_RAS_ALARMS,
	GET_PM_ALARMS,
	GET_PSN,
	GET_LINK_INFO,
	GET_ABS_MAX_MIN_GFX_FREQ = 0x96,
	GET_SVI_TELEMETRY_BY_RAIL,
	GET_DIE_TYPE,
	GET_ACT_GFX_FREQ_CAP_SELECTED = 0x9c,
	GET_DIE_HOT_SPOT_INFO = 0xA0,
	GET_MEM_HOT_SPOT_INFO,
	GET_MAX_OP_TEMP,
	GET_SLOW_DOWN_TEMP,
	GET_STATUS = 0xA4,
	GET_MAX_MEM_BW_UTILIZATION = 0XB0,
	GET_HBM_THROTTLE,
	SET_HBM_THROTTLE,
	GET_HBM_STACK_TEMP,
	GET_GFX_CLK_FREQ_LIMITS,
	GET_FCLK_FREQ_LIMITS,
	GET_SOCKETS_IN_SYSTEM,
	GET_HBM_DEVICE_INFO,
	GET_PCIE_STATS = 0xBA,
	GET_BIST_RESULTS = 0xBC,
	QUERY_STATISTICS,
	CLEAR_STATISTICS
} esb_mi300_mailbox_commmands;

/* Apml link ID encodings for MI300A encodings */
static struct apml_encodings mi300A_encodings[MI300A_ENCODING_SIZE] = {{3, "P2"}, {4, "P3"},
								       {8, "G0"}, {9, "G1"},
								       {10, "G2"}, {11, "G3"},
								       {12, "G4"}, {13, "G5"},
								       {14, "G6"}, {15, "G7"}};	//!< MI300A platforms link ID encodings

/**
 * @brief APML range type used by GFX core clock frequency.
 * Max, MIN are the values. Min is 0 and Max is 1.
 */
enum range_type {
	MIN = 0,
	MAX
};

/**
 * @brief APML clock frequency type. GFX_CLK or F_CLK
 * GFX_CLK value is 0 and F_CLK value is 1.
 */
enum clk_type {
	GFX_CLK = 0,
	F_CLK
};

/**
 * @brief Structure for Max DDR/HBM bandwidth and utilization.
 * It contains max bandwidth(16 bit data) in GBps, current utilization
 * bandwidth(Read+Write)(16 bit data) in GBps.
 */
struct max_mem_bw {
	uint16_t max_bw;		//!< Max Bandwidth (16 bit data)
	uint16_t utilized_bw;		//!< Utilized Bandwidth  (16 bit data)
};

/**
 * @brief struct containing port and slave address.
 */
struct svi_port_domain {
	uint8_t port : 2;               //!< SVI port
	uint8_t slave_addr : 3;         //!< slave address
};

/**
 * @brief struct containing max frequency and min frequencey limit
 */
struct freq_limits {
	uint16_t max;                   //!< Max clock frequency
	uint16_t min;                   //!< Min clock frequency
};

/**
 * @brief struct containing memory clock and fabric clock
 * pstate mappings.
 */
struct mclk_fclk_pstates {
	uint16_t mem_clk;               //!< memory clock frequency in MHz
	uint16_t f_clk;                 //!< fabric clock frequnecy in MHz
};

/**
 * @brief struct containing statistics parameter of interest and output control
 * pstate mappings.
 */
struct statistics {
	uint16_t stat_param;		//!< statistics parameter of interest
	uint16_t output_control;	//!< Output control
};

/**
 * @brief struct containing xgmi speed rate in MHZ and link width in units
 * of Gpbs. If link_width[0] = 1 then XGMI link X2 is supported. If
 * link_width[1] = 1 then XGMI link X4 is supported. If Link_width[2] = 1
 * then XGMI link X4 is supported and similarly if link_width[3] = 1
 * then XGMI link X8 is supported.
 */
struct xgmi_speed_rate_n_width {
	uint16_t speed_rate;		//!< Speed rate
	uint8_t link_width : 4;		//!< XGMI link width
};

/**
 * @brief struct containing device vendor, part number
 * and total memory size in GBs.
 */
struct hbm_device_info {
        uint8_t dev_vendor;             //!< device vendor
        uint8_t part_num;               //!< part number
        uint16_t total_mem;             //!< total memory size(GBs)
};

/**
 * @brief struct containing power management controlling status,
 * driving running status.
 */
struct host_status {
        uint8_t controller_status : 1;	//!< power mangagement controller status
        uint8_t driver_status : 1;	//!< driver running status
};

/**
 * @brief APML alarms type. PM_ALARMS. PM is 0.
 */
enum alarms_type {
	PM
};

/**
 * @brief PM alarm status.
 */
static char *pm_alarm_status[4] = {"VRHOT", "DIE OVER TEMP",
				   "HBM OVER TEMP", "PWRBRK"};

/**
 *  @brief Set maximum/minimum gfx core clock frequency.
 *
 *  @details This function sets user provied frequency as MAX GFX/MIN GFX core
 *  clock frequency in MHZ based on enumeration type #range_type.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] freq_type enumeration type #range_type containing "MIN" = 0
 *  or "MAX" = 1.
 *
 *  @param[in] freq frequency in MHZ.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_gfx_core_clock(uint8_t soc_num, enum range_type freq_type,
				uint32_t freq);

/**
 *  @brief Set maximum mem and fclck pstate.
 *
 *  @details This function sets the maximum memory and fabric clock power state.
 *  Mappings from memory and fabric clock pstate to MEMCLK/FCLK frequency can be
 *  found by issuing GetPstates command.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  In case if the in-band has also set the maximum Pstate, then lower of the
 *  limits is used.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] pstate maximum pstate.Valid pstate range is 0 - 3.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_mclk_fclk_max_pstate(uint8_t soc_num, uint32_t pstate);

/**
 *  @brief Get memory and fabric clock power state mappings
 *
 *  @details This function returns the memory and fabric
 *  clock power state mappings. Returns MEMCLK/FCLK frequency
 *  in units of 1MHz for the available clock power states (Pstates).
 *  Each MEMCLK/FCLK frequency pair is returned independently
 *  for each pstate.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] pstate_ind index of the pstate.
 *
 *  @param[out] pstate struct mclk_fclk_pstates containing mem_clk and
 *  f_clk frequency in MHz.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_mclk_fclk_pstates(uint8_t soc_num, uint8_t pstate_ind,
				   struct mclk_fclk_pstates *pstate);

/**
 *  @brief Sets the XGMI Pstate
 *
 *  @details This function sets the specified XGMI Pstate. This disables all
 *  active XGMI Pstate management although XGMI power down modes will still
 *  be supported. Only 2 XGMI states are supported (0/1).
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] pstate power state. valid values are 0 - 1.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_xgmi_pstate(uint8_t soc_num, uint32_t pstate);

/**
 *  @brief Resets the XGMI Pstate
 *
 *  @details This function resets the XGMI Pstate specified in the
 *  SetXgmiPstate, causing XGMI link speed/width to be actively managed
 *  by the GPU.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t unset_xgmi_pstate(uint8_t soc_num);

/**
 *  @brief Read XGMI power state mappings
 *
 *  @details This function reads the XGMI power state mappings.Reads the
 *  supported XGMI link speeds and widths available to the SetXgmiPstate
 *  message.Link speeds reported in units of 1Gpbs.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] pstate_ind xgmi pstate index for speed rate.
 *
 *  @param[out] xgmi_pstate struct xgmi_speed_rate_n_width containing
 *  speed rate in MHz and link width
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_xgmi_pstates(uint8_t soc_num, uint8_t pstate_ind,
			      struct xgmi_speed_rate_n_width *xgmi_pstate);

/**
 *  @brief Read xcc idle residency percentage
 *
 *  @details This function will provide the average xcc idle residency across
 *  all GFX cores in the socket.100% specifies that all enabled GFX cores
 *  in the socket are running in idle.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] gfx_cores_idle_res idle residency in percentage
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_xcc_idle_residency(uint8_t soc_num, uint32_t *gfx_cores_idle_res);

/**
 *  @brief Read energy accumulator with time stamp
 *
 *  @details This function will read 64 bits energy accumulator
 *  and the 56 bit time stamp.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] energy accumulator  2^ 16 J.
 *
 *  @param[out] time_stamp time stamp (units:10ns).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_energy_accum_with_timestamp(uint8_t soc_num, uint64_t *energy,
					     uint64_t *time_stamp);

/**
 *  @brief Read PM alarm status based on enumeration type #alarms_type
 *
 *  @details This function provides PM alarm status if the enumeration type
 *  #alarms_type is PM = 0 then it will retrieve PM alarm status .
 *  If buffer value  is 1 the status is VRHOT. If the buffer
 *  value is 2 status is die over temp. If the buffer value is 4 status is
 *  HBM over temp and if the buffer is 8 then status is PWRBRK.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] type enumeration type #alarms_type. PM = 0.
 *
 *  @param[out] buffer returns PSP fw return data.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_alarms(uint8_t soc_num, enum alarms_type type,
			uint32_t *buffer);

/**
 *  @brief Reads public serial number (PSN).
 *
 *  @details This function will return 64 bit public serial number (PSN)
 *  unique to each die.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] die_index core/die index.
 *
 *  @param[out] buffer returns 64 bit unique public serial number.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_psn(uint8_t soc_num, uint32_t die_index, uint64_t *buffer);

/**
 *  @brief Read link Info
 *
 *  @details This function will read link info. Function will
 *  read the module ID and link config reflecting strapping pins.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] link_config link configuration.
 *
 *  @param[out] module_id module ID.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_link_info(uint8_t soc_num, uint8_t *link_config,
			   uint8_t *module_id);

/**
 *  @brief Read maximum and minimum allowed GFX engine frequency
 *
 *  @details This function will read maximum and minimum
 *  allowed GFX engine frequency.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] max_freq maximum GFX frequency in MHZ.
 *
 *  @param[out] min_freq minimum GFX frequency in MHZ.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_max_min_gfx_freq(uint8_t soc_num, uint16_t *max_freq,
				  uint16_t *min_freq);

/**
 *  @brief Read Actual GFX frequency cap selected
 *
 *  @details This function will read current seleted
 *  GFX engine clock frequency.It reflects minimum of
 *  all frequency caps seleted via in-band and out-of-band
 *  controls.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] freq maximum GFX frequency in MHZ.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_act_gfx_freq_cap(uint8_t soc_num, uint16_t *freq);

/**
 *  @brief Read SVI based telemetry for individual rails
 *
 *  @details This function will read SVI based telemetry for
 *  individual rails.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] port struct svi_telemetry_domain containing port
 *  and slave address.
 *
 *  @param[out] pow power in milliwatts.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_svi_rail_telemetry(uint8_t soc_num,
				    struct svi_port_domain port,
				    uint32_t *pow);

/**
 *  @brief Reads local ID of the hottest die and its temperature.
 *
 *  @details This function will read local ID of the hottest die and its
 *  corresponding die temperature. Measured in every 1 ms and
 *  the most recently measured temperature in °C is reported.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] die_id Hottest die ID.
 *
 *  @param[out] temp Die hot spot temperature in °C.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_die_hotspot_info(uint8_t soc_num, uint8_t *die_id,
				  uint16_t *temp);

/**
 *  @brief Reads local ID of the HBM stack and its temperature.
 *
 *  @details This function will read local ID of the HBM stack and its
 *  corresponding HBM stack temperature. Measured in every 1 ms and
 *  the most recently measured temperature is reported.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] hbm_stack_id Local ID of the hottest HBM stack.
 *
 *  @param[out] hbm_temp temperature in units of 1 °C.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_mem_hotspot_info(uint8_t soc_num, uint8_t *hbm_stack_id,
				  uint16_t *hbm_temp);

/**
 *  @brief Reads the status in a bit vector
 *
 *  @details This function will read PM controller status
 *  and driver running status in a bit vector
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] status struct host_status containing power management
 *  controller status and driver running status.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_host_status(uint8_t soc_num, struct host_status *status);

/**
 *  @brief Reads max memory bandwidth utilization.
 *
 *  @details This function will provide theoretic.al maximum HBM/memory
 *  bandwidth of the system in GB/s, utilized bandwidth in GB/S.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] bw struct max_mem_bw containing max bw,
 *  utilized b/w.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_max_mem_bw_util(uint8_t soc_num,
				 struct max_mem_bw *bw);

/**
 *  @brief Reads HBM throttle.
 *
 *  @details This function will read HBM throttle in percentage.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] mem_th hbm throttle in percentage (0 - 100%).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_hbm_throttle(uint8_t soc_num, uint32_t *mem_th);

/**
 *  @brief writes HBM throttle.
 *
 *  @details This function will write HBM throttle.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] mem_th hbm throttle in percentage (0 - 80%).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_hbm_throttle(uint8_t soc_num, uint32_t mem_th);

/**
 *  @brief Reads hbm stack temperature.
 *
 *  @details This function will read particular hbm stack temperature.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] index hbm stack index ( 0 - 7 for MI300).
 *
 *  @param[out] temp temperature in units of 1 °C.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_hbm_temperature(uint8_t soc_num, uint32_t index,
				 uint16_t *temp);

/**
 *  @brief Reads GFX/F clk frequency limits based on enumeration
 *  type #clk_type .
 *
 *  @details This function will provide socket's GFX/F clk max and min
 *  frequnecy limits based on enumberation type #clk_type .
 *  The function reads GFX clk frequency limits if the enumberation
 *  type #clk_type is GFX_CLK = 0 else it will read F_CLK frequency
 *  limits.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] type enumeration type #clk_type .
 *  Values are "GFX_CLK" = 0  or "F_CLK" = 1.
 *
 *  @param[out] limit struct freq_limits containing max and min GFX/F_clk
 *  frequency in MHZ.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_clk_freq_limits(uint8_t soc_num, enum clk_type type,
				 struct freq_limits *limit);

/**
 *  @brief Reads number of sockets in system
 *
 *  @details This function will read number of sockets in system.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] sockets_count Numbers of sockets in system
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_sockets_in_system(uint8_t soc_num, uint32_t *sockets_count);

/**
 *  @brief Reads die level bist result status from package
 *
 *  @details This function will read die level bist result status
 *  from package.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] die_id die level id.
 *
 *  @param[out] bist_result constituent bist results depending on
 *  MI300X/A/C configuration.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_bist_results(uint8_t soc_num, uint8_t die_id,
			      uint32_t *bist_result);

/**
 *  @brief Reads statistics for a given parameter.
 *
 *  @details This function will read statistics for a given parameter
 *  since the last clear statistics command.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] stat struct statistics containing statistics parameter
 *  of interest and output control.
 *
 *  @param[out] param parameter or timestamp HI/Lo value.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_statistics(uint8_t soc_num, struct statistics stat,
			    uint32_t *param);

/**
 *  @brief Clears statistics.
 *
 *  @details This function will clear all stored query statistics timestamps
 *  and then resumes data collection or aggregation.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t clear_statistics(uint8_t soc_num);

/**
 *  @brief Get die type
 *
 *  @details This function will read die-type, counts and
 *  AID base die based on die-ID input.
 *  If the bit[0] of input/data_in is 1 then it will get maximum die-ID
 *  (0 - 255). If the bit[0] of input/data_in is 0 then the data_out[7:0]
 *  will be die type i.e. 0 Not available, 1 means AID, 2 means XCD, 3 means
 *  CCD and 4 means HBM stack and 5- 255 are reserved. data_out[15:8] means
 *  maximum coumt of currently indexed die type. Data_out[19:16] means
 *  AID associated with specified die-ID.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] data_in input to get the die-type, counts and
 *  AID base.
 *
 *  @param[out] data_out maximum die id idnex or the die type
 *  based on die-id input.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_die_type(uint8_t soc_num, uint32_t data_in,
			  uint32_t *data_out);

/**
 *  @brief Get current xgmi pstate
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] xgmi_pstate current xgmi pstate
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_curr_xgmi_pstate(uint8_t soc_num, uint8_t *xgmi_pstate);

/**
 *  @brief Get maximum operating temperature
 *
 *  @details This function will get critical temperature fault core
 *  and HBM temperature.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] ctf_type critical temperature fault type. 0 for AID,
 *  1 for CCD, 2 for XCD and 3 for HBM.
 *
 *  @param[out] temp Max operating temp in °C
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_max_operating_temp(uint8_t soc_num, uint32_t ctf_type,
                                    uint16_t *temp);

/**
 *  @brief Get slow down temperature
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] ctf_type critical temperature fault type. 0 for AID,
 *  1 for CCD, 2 for XCD and 3 for HBM.
 *
 *  @param[out] slow_down_temp slow down temperature in °C
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_slow_down_temp(uint8_t soc_num, uint32_t ctf_type,
				uint16_t *slow_down_temp);

/**
 *  @brief Get hbm device information
 *
 *  @details This function will get hbm device information i.e. device
 *  vendor, part number and total memory size (GBs).
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] dev_info struct hbm_device_info contatining device vendor,
 *  part number and total memory size (GBs).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_hbm_dev_info(uint8_t soc_num,
                              struct hbm_device_info *dev_info);

/**
 *  @brief Get PCIe statistics
 *
 *  @details This function will get PCIe statistics
 *  including total transitions from L0 to recovery
 *  state, total number of replays issued on the PCIe link,
 *  total number of NAKs issued on the PCIe link by the device,
 *  and total number of NAKs issued on the PCIe link by the
 *  receiver.
 *  Supported platforms: \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] pcie_stat_select PCIe stat selector.
 *  0 for L0 to recovery count, 1 for replay count,
 *  2 for NAK sent count and 3 for NAK recieved count.
 *
 *  @param[out] pcie_stats pcie statistics
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_pciestats(uint8_t soc_num, uint32_t pcie_stat_select,
                           uint32_t *pcie_stats);

/* @}
 */  // end of MailboxMsg
/****************************************************************************/

 #endif  // INCLUDE_RMI_MI300_H_
