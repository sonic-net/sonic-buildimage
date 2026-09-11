/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef INCLUDE_APML_MAILBOX_H_
#define INCLUDE_APML_MAILBOX_H_

#include "apml_common.h"
#include "apml_err.h"
#include <stdbool.h>

#define BYTE_BITS              8       //!< Byte bits //
#define WORD_BITS              16      //!< word bits //

/* RAS OOB Config bits position for 0x61 - 0x65 msgs */
#define DRAM_CECC_OOB_EC_MODE   1	//!< DRAM cecc oob ec mode //
#define ERR_COUNT_TH            2	//!< Error count threshold position //
/* DRAM cecc leak rate */
#define DRAM_CECC_LEAK_RATE     3	//!< DRAM CECC Leak rate	//
/* PCIE error reporting enable */
#define PCIE_ERR_REPORT_EN      8	//!< PCIE oob counter enable //
/* MCA Threshold Interrupt */
#define MCA_TH_INTR             11	//!< MCA threshold interrupt //
/* CECC Threshold Interrupt */
#define CECC_TH_INTR            12	//!< CECC threshold interrupt //
/* PCIE Threshold Interrupt */
#define PCIE_TH_INTR		13	//!< PCIE threshold interrupt //
/* MCA Max interrupt rate */
#define MCA_MAX_INTR_RATE       15	//!< MCA max interrupt rate //
/* Interrupt rate position */
#define MAX_INTR_RATE_POS       18	//!< Max interrupt rate position //
/* DRAM CECC Max Interrupt rate */
#define DRAM_CECC_MAX_INTR_RATE 19	//!< DRAM CECC Max interrupt rate //
/* PCIE Max Interrrupt rate */
#define PCIE_MAX_INTR_RATE	23     //!< PCIE Max interrupt rate //
/* core mca error reporting enable*/
#define MCA_ERR_REPORT_EN 	31	//!< MCA error report enable //

/* Legacy APML encodings count */
#define LEGACY_ENCODING_SIZE		8	//!< Legacy encoding size //

/** \file esmi_mailbox.h
 *  Header file for the Mailbox messages supported by APML library.
 *  All required function, structure, enum, etc. definitions should be defined
 *  in this file.
 *
 *  @details  This header file contains the following:
 *  APIs prototype of the Mailbox messages exported by the APML library.
 *  Description of the API, arguments and return values.
 *  The Error codes returned by the API.
 */

/* Apml link ID encodings for legacy platforms */
static struct apml_encodings encodings[LEGACY_ENCODING_SIZE] = {{1, "P0"}, {2, "P1"}, {4, "P2"},
								{8, "P3"}, {16, "G0"}, {32, "G1"},
								{64, "G2"}, {128, "G3"}};	//!< Legacy platforms link ID encodings

/**
 * @brief Mailbox message types defined in the APML library
 */
typedef enum {
	READ_PACKAGE_POWER_CONSUMPTION  = 0x1,
	WRITE_PACKAGE_POWER_LIMIT,
	READ_PACKAGE_POWER_LIMIT,
	READ_MAX_PACKAGE_POWER_LIMIT,
	READ_TDP,
	READ_MAX_cTDP,
	READ_MIN_cTDP,
	READ_BIOS_BOOST_Fmax,
	READ_APML_BOOST_LIMIT,
	WRITE_APML_BOOST_LIMIT,
	WRITE_APML_BOOST_LIMIT_ALLCORES,
	READ_DRAM_THROTTLE,
	WRITE_DRAM_THROTTLE,
	READ_PROCHOT_STATUS,
	READ_PROCHOT_RESIDENCY,
	READ_NBIO_ERROR_LOGGING_REGISTER = 0x11,
	READ_IOD_BIST = 0x13,
	READ_CCD_BIST_RESULT,
	READ_CCX_BIST_RESULT,
	READ_PACKAGE_CCLK_FREQ_LIMIT,
	READ_PACKAGE_C0_RESIDENCY,
	READ_DDR_BANDWIDTH = 0x18,
	READ_SMU_FW_VER = 0x1C,
	READ_PPIN_FUSE = 0x1F,
	GET_POST_CODE = 0x20,
	GET_RTC,
	WRITE_BMC_REPORT_DIMM_POWER = 0X40,
	WRITE_BMC_REPORT_DIMM_THERMAL_SENSOR,
	READ_BMC_RAS_PCIE_CONFIG_ACCESS,
	READ_BMC_RAS_MCA_VALIDITY_CHECK,
	READ_BMC_RAS_MCA_MSR_DUMP,
	READ_BMC_RAS_FCH_RESET_REASON,
	READ_DIMM_TEMP_RANGE_AND_REFRESH_RATE,
	READ_DIMM_POWER_CONSUMPTION,
	READ_DIMM_THERMAL_SENSOR,
	READ_PWR_CURRENT_ACTIVE_FREQ_LIMIT_SOCKET,
	READ_PWR_CURRENT_ACTIVE_FREQ_LIMIT_CORE,
	READ_PWR_SVI_TELEMETRY_ALL_RAILS,
	READ_SOCKET_FREQ_RANGE,
	READ_CURRENT_IO_BANDWIDTH,
	READ_CURRENT_XGMI_BANDWIDTH,
	WRITE_GMI3_LINK_WIDTH_RANGE,
	WRITE_XGMI_LINK_WIDTH_RANGE,
	WRITE_APB_DISABLE,
	WRITE_APB_ENABLE,
	READ_CURRENT_DFPSTATE_FREQUENCY,
	WRITE_LCLK_DPM_LEVEL_RANGE,
	READ_BMC_RAPL_UNITS,
	READ_BMC_RAPL_CORE_LO_COUNTER,
	READ_BMC_RAPL_CORE_HI_COUNTER,
	READ_BMC_RAPL_PKG_COUNTER,
	READ_BMC_CPU_BASE_FREQUENCY,
	READ_BMC_CONTROL_PCIE_GEN5_RATE,
	READ_RAS_LAST_TRANS_ADDR_CHK,
	READ_RAS_LAST_TRANS_ADDR_DUMP,
	WRITE_PWR_EFFICIENCY_MODE,
	WRITE_DF_PSTATE_RANGE,
	READ_LCLK_DPM_LEVEL_RANGE,
	READ_UCODE_REVISION,
	GET_BMC_RAS_RUNTIME_ERR_VALIDITY_CHECK,
	GET_BMC_RAS_RUNTIME_ERR_INFO,
	SET_BMC_RAS_ERR_THRESHOLD,
	SET_BM_RAS_OOB_CONFIG,
	GET_BMC_RAS_OOB_CONFIG,
	SET_BMC_PCIE_CONFIG = 0x68,
	BMC_RAS_DELAY_RESET_ON_SYNCFLOOD_OVERRIDE = 0x6A,
	READ_BMC_RAS_RESET_ON_SYNC_FLOOD,
	GET_DIMM_SPD	= 0x70,
	SET_XGMI_PSTATE_RANGE = 0x73,
	CPU_RAIL_ISO_FREQ_POLICY,
	DFC_ENABLE = 0x76,
	GET_DRAM_THROTTLE_CHANNELS = 0x78
} esb_mailbox_commmands;

/**
 * @brief APML IO Bandwidth Encoding defined in the APML library
 */
typedef enum {
	AGG_BW = BIT(0),
	RD_BW = BIT(1),
	WR_BW = BIT(2)
} apml_io_bw_encoding;

/**
 * @brief APML IO LINK ID Encoding defined in the APML library
 */
typedef enum {
	P0 = BIT(0),
	P1 = BIT(1),
	P2 = BIT(2),
	P3 = BIT(3),
	G0 = BIT(4),
	G1 = BIT(5),
	G2 = BIT(6),
	G3 = BIT(7)
} apml_link_id_encoding;

/**
 * @brief DIMM power(mW), update rate(ms) and dimm address
 */
struct dimm_power {
	uint16_t power : 15;		//!< Dimm power consumption
	uint16_t update_rate : 9;	//!< update rate in ms
	uint8_t dimm_addr;		//!< Dimm address
};

/**
 * @brief DIMM thermal sensor (degree C), update rate
 * and dimm address
 */
struct dimm_thermal {
	uint16_t sensor : 11;		//!< Dimm thermal sensor
	uint16_t update_rate : 9;	//!< update rate in ms
	uint8_t dimm_addr;		//!< Dimm address
};

/**
 * @brief DIMM temperature range and refresh rate, temperature update flag
 */
struct temp_refresh_rate {
	uint8_t range : 3;		//!< temp refresh rate (3 bit data)
	uint8_t ref_rate : 1;		//!< temp update flag (1 bit data)
};

/**
 * @brief PCI address information .PCI address includes 4 bit segment,
 * 12 bit aligned offset, 8 bit bus, 5 bit device info and 3 bit function
 */
struct pci_address {
	uint8_t func : 3;	//!< function (3 bit data)
	uint8_t device : 5;	//!< device info (5 bit data)
	uint8_t bus;		//!< bus (8 bit data)
	uint16_t offset : 12;	//!< offset address (12 bit data)
	uint8_t segment : 4;	//!< segment (4 bit data)
};

/**
 * @brief Max and min LCK DPM level on a given NBIO ID.
 * Valid Max and min DPM level values are 0 - 1.
 */
struct dpm_level {
	uint8_t max_dpm_level;		//!< Max LCLK DPM level [0 - 1]
	uint8_t min_dpm_level;		//!< Min LCLK DPM level [ 0 - 1]
};

/**
 * @brief Max and Min Link frequency clock (LCLK) DPM level on a socket.
 * 8 bit NBIO ID, dpm_level struct containing 8 bit max DPM level,
 * 8 bit min DPM level
 */
struct lclk_dpm_level_range {
	uint8_t nbio_id;		//!< NBIOD id (8 bit data [0 - 3])
	struct dpm_level dpm;           //!< struct with max dpm, min dpm levels
};

/**
 * @brief NBIO quadrant(8 bit data) and NBIO register offset(24 bit) data.
 */
struct nbio_err_log {
	uint8_t quadrant;	//!<< NBIO quadrant data
	uint32_t offset : 24;	//!<< NBIO register offset (24 bit data)
};

/**
 * @brief Structure for Max DDR bandwidth and utilization.
 * It contains max bandwidth(12 bit data) in GBps, current utilization
 * bandwidth(12 bit data) in GBps, current utilized bandwidth( 8 bit data)
 * in percentage.
 */
struct max_ddr_bw {
	uint16_t max_bw : 12;		//!< Max Bandwidth (12 bit data)
	uint16_t utilized_bw : 12;	//!< Utilized Bandwidth  (12 bit data)
	uint8_t utilized_pct;		//!< Utliized Bandwidth percentage
};

/**
 * @brief MCA bank information.It contains 16 bit Index for MCA Bank
 * and 16 bit offset.
 */
struct mca_bank {
	uint16_t offset;	//!< Offset with in MCA Bank
	uint16_t index;		//!< Index of MCA Bank
};

/**
 * @brief APML LINK ID and Bandwidth type Information.It contains
 * APML LINK ID Encoding. Non-MI300 Platforms Valid Link ID encodings are
 * 1(P0), 2(P1), 4(P2), 8(P3), 16(G0), 32(G1), 64(G2), 128(G3). Valid APML
 * MI300 APML LINK ID Encoding. Valid Link ID encodings are 3(P2), 4(P3),
 * 8(G0), 9(G1), 10(G2), 11(G3), 12(G4), 13(G5), 14(G6), 15(G7).
 * IO Bandwidth types 1(Aggregate_BW), 2 (Read BW), 4 (Write BW).
 *
 */
struct link_id_bw_type {
	apml_io_bw_encoding bw_type;	//!< Bandwidth Type Information [1, 2, 4]
	uint8_t link_id;		//!< Link ID [1,2,4,8,16,32,64,128] for Non-MI300 platforms
					//!< Link ID [3,4,8,9,10,11,12,13,14,15] for MI300 platforms
};

/**
 * @brief DF P-state frequency.It includes mem clock(16 bit data) frequency
 * (DRAM memory clock), data fabric clock (12 bit data), UMC clock divider
 * (UMC) (1 bit data).
 */
struct pstate_freq {
	uint16_t mem_clk;	//!< DRAM Memory clock Frequency (MHz)(12 bit)
	uint16_t fclk : 12;	//!< Data fabric clock (MHz)(12 bit data)
	uint8_t uclk : 1;	//!< UMC clock divider (1 bit data)
};

/**
 * @brief BMC RAS override delay reset CPU on sync flood. The structure contains
 * delay value override in mins [5 -120 mins], disable delay counter and
 * stop delay counter. If disable delay counter is set ResetCpuOnSyncFlood
 * response will NOT be delayed in the next SyncFlood regardless of the value
 * specified in delay_val_override. If StopDelayCounter is set it stops the
 * active delay countdown which extends the DelayResetCpuOnSyncFlood indefinitely
 * and system will not reset.
 */

struct ras_override_delay {
	uint8_t delay_val_override;	//!< Delay value override [5 -120 mins]
	uint8_t disable_delay_counter : 1;	//!< Disable delay counter
	uint8_t stop_delay_counter : 1;	//!< stop delay counter
};

/**
 * @brief Get the Error type and request type.
 * Supported Runtime error type[1:0]:
 *	- 00 = MCA
 *	- 01 = DRAM CECC
 *	- 10 = PCIe
 * Supported Request type[31]:
 *	- 0: polling mode
 *	- 1: interrupt mode
 */
struct ras_rt_err_req_type {
	uint8_t err_type : 2;	//!< Error type, [00, 01, 10]
	uint8_t req_type : 1;	//!< Request type, [0, 1]
};

/**
 * @brief Number of valid error instance per category. It consists of
 * number of bytes per each category and number of instances of each
 * category.
 */
struct ras_rt_valid_err_inst {
	uint16_t number_bytes;          //!< Number of bytes of given category
	uint16_t number_of_inst;        //!< Number of instances of given catg
};

/**
 * @brief Get run time error information data_in. Runtime error data_in includes
 * 4 byte aligned offset in instance, runtime error category and zero based
 * index of valid instance
 * number of bytes per each category and number of instances of each
 * category.
 */
struct run_time_err_d_in {
	uint8_t offset;                 //!< 4 byte aligned offset in instance
	uint8_t category;               //!< Runtime error category
	uint8_t valid_inst_index;       //!< Valid inst index returned by 61h
};

/**
 * @brief Configure threshold counters for MCA, DRAM CECC and PCIE.
 * structure consists of error type, error count threshold and
 * max interrupt rate.
 */
struct run_time_threshold {
	uint8_t err_type : 2;           //!< error type [00(MCA), 01(DRAM CECC)
					//!< 10 (PCIE_UE) and 11(RSVD)
	uint16_t err_count_th;          //!< error count threshold
	uint8_t max_intrupt_rate : 4;	//!< Max interrupt rate
};

/**
 * @brief Configure oob state infrastructure in SoC.
 * structure consists of mca_oob_misc0_ec_enable, dram_cecc_oob_ec_mode,
 * dram_cecc_leak_rate, pcie_err_reporting_en and mca_err_reporting_en.
 */
struct oob_config_d_in {
	uint8_t mca_oob_misc0_ec_enable : 1;	//!< MCA OOB MISC0 Error Counter
						//!< Enable: 0 = disable, 1 = enable
	uint8_t dram_cecc_oob_ec_mode : 2;	//!< DRAM CECC OOB error counter
						//!< mode 00 = disable,
						//!< 01 enable in noleak mode, 10 = enable
						//!< in leak mode and 11 is reserved
	uint8_t dram_cecc_leak_rate: 5;		//!< DRAM CECC OOB leak rate.
						//!< Valid values are 00 - 1Fh
	uint8_t pcie_err_reporting_en : 1;	//!< PCIe OOB error reporting enable
						//!< 0 disable and 1 for enable
	uint8_t	core_mca_err_reporting_en : 1;	//!< MCA OOB error reporting enable
						//!< 0 disable and 1 for enable
};

/**
 * @brief RAS df err validity check output status.
 * Structure  contains the following members.
 * df_block_instances number of block instance with error log to report (0 - 256)
 * err_log_len length of error log in bytes per instance (0 - 256).
 */
struct ras_df_err_chk {
	uint32_t add_err_data;		  //!< field is updated when additional error data is available
	uint16_t df_block_instances : 9;  //!< Number of DF block instances
	uint16_t err_log_len : 9;	  //!< len of er log in bytes per inst.
};

/**
 * @brief RAS df error dump input.
 * Union contains the following members.
 * input[0] 4 byte alligned offset in error log ( 0 - 255)
 * input[1] DF block ID (0 - 255)
 * input[2] Zero based index of DF block instance (0 - 255)
 * input[3] Reserved
 * data_in  32-bit data input
 *
 */
union ras_df_err_dump {
	uint8_t input[4];     //!< [0] offset, [1] DF block ID
			      //!< [2] block ID inst, [3] RESERVED
	uint32_t data_in;     //!< 32 bit data in for the DF err dump
};

/**
 * @brief dimm_spd_d_in:
 * strucutre contains following memebers to be passed as
 * input, to get 4 byte DIMM spd register data
 */
struct dimm_spd_d_in {
	uint8_t dimm_addr;		//!< DIMM address
	uint8_t lid : 4;		//!< local identifier of device
	uint16_t reg_offset : 11;	//!< Register offset in given register space
	uint8_t reg_space : 1;		//!< Register space, Volatile:0, NVM:1
	uint8_t rsvd;			//!< reserved
};

/**
 * @brief frequency limit source names
 */
static char * const freqlimitsrcnames[] = {
	"cHTC-Active",
	"PROCHOT",
	"TDC limit",
	"PPT Limit",
	"OPN Max",
	"Reliability Limit",
	"APML Agent",
	"HSMP Agent"
};

/**
 * @brief energy status multiplier
 * value is 1/2^ESU where ESU is
 * [12:8] bit of the mailbox command 0x55h.
 */
extern float esu_multiplier;

/*****************************************************************************/

/** @defgroup MailboxMsg SB-RMI Mailbox Service
 *  write, 'write and read' operations for a given socket.
 *  @{
 */
/** @defgroup PowerQuer Power Monitor
 *  Below functions provide interfaces to get the current power usage and
 *  Power Limits for a given socket.
 *  @{
 */
/** @defgroup Supporting Platforms
 *  @{
 */
/** @page PLATFORMS
 *  Supporting Platforms:
 *  \ref Fam-19h_Mod-00h-0Fh
 *  \ref Fam-19h_Mod-10h-1Fh
 *  \ref Fam-19h_Mod-90h-9Fh
 *  \ref Fam-1Ah_Mod-00h-0Fh
 *  \section Fam-19h_Mod-00h-0Fh
 *  Platform with Family 19h and model in range 00h - 0Fh
 *  \section Fam-19h_Mod-10h-1Fh
 *  Platform with Family 19h and model in range 10h - 1Fh
 *  \section Fam-19h_Mod-90h-9Fh
 *  Platform with Family 19h and model in range 90h - 9Fh
 *  \section Fam-1Ah_Mod-00h-0Fh
 *  Platform with Family 1Ah and model in range 00h - 0Fh
 *
 */
/** @} */ // end of Supporting Platforms


/**
 *  @brief Get the power consumption of the socket.
 *
 *  @details Given socket number and a pointer to a uint32_t
 *  @p buffer, this function will get the current power consumption
 *  (in watts) to the uint32_t pointed to by @p buffer.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to uint32_t value of power
 *  consumption
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_socket_power(uint8_t soc_num, uint32_t *buffer);

/**
 *  @brief Get the current power cap/limit value for a given socket.
 *
 *  @details This function will return the valid power cap @p buffer for a given
 *  socket, this value will be used for the system to limit the power.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to a uint32_t that indicates the valid
 *  possible power cap/limit, in watts
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_socket_power_limit(uint8_t soc_num, uint32_t *buffer);

/**
 *  @brief Get the maximum value that can be assigned as a power cap/limit for
 *         a given socket.
 *
 *  @details This function will return the maximum possible valid power cap/limit
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] buffer a pointer to a uint32_t that indicates the maximum
 *                possible power cap/limit, in watts
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_max_socket_power_limit(uint8_t soc_num, uint32_t *buffer);

/** @} */  // end of PowerQuer
/*****************************************************************************/

/** @defgroup PowerCont Power Control
 *  This function provides a way to control Power Limit.
 *  @{
 */

/**
 *  @brief Set the power cap/limit value for a given socket.
 *
 *  @details This function will set the power cap/limit
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] limit uint32_t that indicates the desired power cap/limit,
 *  in milliwatts
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_socket_power_limit(uint8_t soc_num, uint32_t limit);

/** @} */  // end of PowerCont

/*****************************************************************************/

/** @defgroup PerfQuer Performance (Boost limit) Monitor
 *  This function provides the current boostlimit value for a given core.
 *  @{
 */

/**
 *  @brief Get the Out-of-band boostlimit value for a given core
 *
 *  @details This function will return the core's current Out-of-band
 *  boost limit
 *  @p buffer for a particular @p value
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] value a cpu index
 *
 *  @param[inout] buffer pointer to a uint32_t that indicates the
 *  possible boost limit value
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_esb_boost_limit(uint8_t soc_num, uint32_t value,
				  uint32_t *buffer);

/**
 *  @brief Get the In-band maximum boostlimit value for a given core
 *
 *  @details This function will return the core's current maximum In-band
 *  boost limit @p buffer for a particular @p value is cpu_ind
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] value is a cpu index
 *
 *  @param[inout] buffer a pointer to a uint32_t that indicates the
 *  maximum boost limit value set via In-band
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_bios_boost_fmax(uint8_t soc_num,
				  uint32_t value,
				  uint32_t *buffer);
/** @} */  // end of PerfQuer

/*****************************************************************************/

/** @defgroup PerfCont Out-of-band Performance (Boost limit) Control
 *  Below functions provide ways to control the Out-of-band Boost limit values.
 *  @{
 */

/**
 *  @brief Set the Out-of-band boostlimit value for a given core
 *
 *  @details This function will set the boostlimit to the provided value @p
 *  limit for a given cpu.
 *  NOTE: Currently the limit is setting for all the cores instead of a
 *  particular cpu. Testing in Progress.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] cpu_ind a cpu index is a given core to set the boostlimit
 *
 *  @param[in] limit a uint32_t that indicates the desired Out-of-band
 *  boostlimit value of a given core
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_esb_boost_limit(uint8_t soc_num,
				   uint32_t cpu_ind, uint32_t limit);

/**
 *  @brief Set the boostlimit value for the whole socket (whole system).
 *
 *  @details This function will set the boostlimit to the provided value @p
 *  boostlimit for the socket.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] limit a uint32_t that indicates the desired boostlimit
 *  value of the socket
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_esb_boost_limit_allcores(uint8_t soc_num, uint32_t limit);

/** @} */  // end of PerfCont

/*****************************************************************************/

/** @defgroup TdpQuer Current, Min, Max TDP
 *  Below functions provide interfaces to get the current, Min and Max TDP,
 *  Prochot and Prochot Residency for a given socket.
 *  @{
 */

/**
 *  @brief Get the Thermal Design Power limit TDP of the socket with provided
 *  socket index.
 *
 *  @details Given a socket and a pointer to a uint32_t @p buffer, this function
 *  will get the current TDP (in milliwatts)
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to uint32_t to which the Current TDP value
 *  will be copied
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_tdp(uint8_t soc_num, uint32_t *buffer);

/**
 *  @brief Get the Maximum Thermal Design Power limit TDP of the socket with
 *  provided socket index.
 *
 *  @details Given a socket and a pointer, this function will get the Maximum
 *  TDP (watts)
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to uint32_t to which the Maximum TDP value
 *  will be copied
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_max_tdp(uint8_t soc_num, uint32_t *buffer);

/**
 *  @brief Get the Minimum Thermal Design Power limit TDP of the socket
 *
 *  @details Given a socket and a pointer to a uint32_t, this function will
 *  get the Minimum  TDP (watts)
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to uint32_t to which the Minimum TDP value
 *  will be copied
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_min_tdp(uint8_t soc_num, uint32_t *buffer);

/** @} */  // end of TdpQuer

/*****************************************************************************/

/** @defgroup ProchotQuer Prochot
 *  Below functions provide interfaces to get Prochot and Prochot Residency
 *  for a given socket.
 *  @{
 */
/**
 *  @brief Get the Prochot Status of the socket with provided socket index.
 *
 *  @details Given a socket and a pointer to a uint32_t,
 *  this function will get the Prochot status as active/1 or
 *  inactive/0
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to uint32_t to which the Prochot status
 *  will be copied
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_prochot_status(uint8_t soc_num, uint32_t *buffer);

/**
 *  @brief Get the Prochot Residency (since the boot time or last
 *  read of Prochot Residency) of the socket.
 *
 *  @details Given a socket and a pointer to a uint32_t,
 *  this function will get the Prochot residency as a percentage
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to float to which the Prochot residency
 *  will be copied
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_prochot_residency(uint8_t soc_num, float *buffer);

/** @} */  // end of ProchotQuer

/****************************************************************************/
/** @defgroup dramQuer Dram and other features Query
 *  @{
 */

/**
 *  @brief Read Dram Throttle will always read the highest percentage value.
 *
 *  @details This function will always read the highest percentage value
 *  as represented by PROCHOT throttle or write dram throttle.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh
 *  and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] buffer is to read the dram throttle in % (0 - 100).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_dram_throttle(uint8_t soc_num, uint32_t *buffer);

/**
 *  @brief Set Dram Throttle value in terms of  percentage.
 *
 *  @details This function will set the dram throttle of the provided value
 *  limit for the given socket.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh
 *  and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] limit that indicates the desired limit as per SSP PPR write can be
 *  between 0 to 80% to for a given socket
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_dram_throttle(uint8_t soc_num, uint32_t limit);

/**
 *  @brief Read VDDIOMem Power returns the estimated VDDIOMem power consumed
 *  within the socket.
 *
 *  @details This function will read VDDIOMem Power for the given socket
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] buffer is to read VDDIOMem Power.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_vddio_mem_power(uint8_t soc_num, uint32_t *buffer);

/**
 *  @brief Read NBIO Error Logging Register
 *
 *  @details Given a socket, quadrant and register offset as @p input,
 *  this function will read NBIOErrorLoggingRegister.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh  and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] nbio nbio_err_log Struct containing nbio quadrant and offset.
 *
 *  @param[out] buffer is to read NBIOErrorLoggingRegiter(register value).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t
read_nbio_error_logging_register(uint8_t soc_num,
				 struct nbio_err_log nbio,
				 uint32_t *buffer);

/**
 *  @brief Read IOD Bist status.
 *
 *  @details This function will read IOD Bist result for the given socket.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] buffer is to read IODBistResult
 *  0 = Bist pass,
 *  1 = Bist fail
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_iod_bist(uint8_t soc_num, uint32_t *buffer);

/**
 *  @brief Read CCD Bist status. Results are read for each CCD present in the
 *  system.
 *
 *  @details Given a socket bus number and address, Logical CCD instance
 *  number as @p input, this function will read CCDBistResult.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] input is a Logical CCD instance number.
 *
 *  @param[out] buffer is to read CCDBistResult
 *  0 = Bist pass,
 *  1 = Bist fail
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_ccd_bist_result(uint8_t soc_num, uint32_t input,
				  uint32_t *buffer);

/**
 *  @brief Read CPU Core Complex Bist result. results are read for each Logical
 *  CCX instance number and returns a value which is the concatenation of L3
 *  pass status and all cores in the complex(n:0).
 *
 *  @details Given a socket bus number, address, Logical CCX instance number
 *  as @p input, this function will read CCXBistResult.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] value is a Logical CCX instance number. Valid values [0, (k -1)]
 *  where k is the number of logical CCX instances.
 *
 *  @param[out] ccx_bist result is concatenation of bist results for all cores[31:16]
 *  in the complex(n:0) L3 bist[15:0], where n num of cores in CCX.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_ccx_bist_result(uint8_t soc_num, uint32_t value,
				  uint32_t *ccx_bist);

/**
 *  @brief Read CCLK frequency limit for the given socket
 *
 *  @details This function will read CPU core clock frequency limit
 *  for the given socket.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] cclk_freq CPU core clock frequency limit [MHz]
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_cclk_freq_limit(uint8_t soc_num, uint32_t *cclk_freq);

/**
 *  @brief Read socket C0 residency
 *
 *  @details This function will provides average C0 residency across all cores
 *  in the socket. 100% specifies that all enabled cores in the socket are runningin C0.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] c0_res is to read Socket C0 residency[%].
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_socket_c0_residency(uint8_t soc_num, uint32_t *c0_res);

/**
 *  @brief Get the Theoretical maximum DDR Bandwidth of the system in GB/s,
 *  Current utilized DDR Bandwidth (Read + Write) in GB/s and
 *  Current utilized DDR Bandwidth as a percentage of theoretical maximum.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] max_ddr max_ddr_bw struct containing max bandwidth,
 *  utilized bandwidth and utilized bandwidth percentage.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_ddr_bandwidth(uint8_t soc_num,
				struct max_ddr_bw *max_ddr);
/** @} */

/** @} */

/**
 *  @brief Set DIMM Power consumption in mwatts.
 *
 *  @details This function will set DIMM Power consumption periodically
 *  by BMC at specified update rate (10 ms or less) when bmc owns the
 *  SPD side-band bus.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] dp_info dimm_power Struct with power(mw), updaterate(ms) & dimm address
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_bmc_report_dimm_power(uint8_t soc_num,
					 struct dimm_power dp_info);

/**
 *  @brief Set DIMM thermal Sensor in degree Celcius.
 *
 *  @details This function will set DIMM thermal sensor (in degree celcius)
 *  periodically by BMC at specified update rate (10 ms or less) when bmc owns
 *  the SPD side-band bus.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] dt_info struct with temp(ºC), updaterate(ms) & dimm address
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_bmc_report_dimm_thermal_sensor(uint8_t soc_num,
						  struct dimm_thermal dt_info);

/**
 *  @brief Read BMC RAS PCIE config access.
 *
 *  @details This function will read the 32 bit BMC RAS
 *  extended PCI config space.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] pci_addr pci_address structure with fucntion(3 bit),
 *  device(4 bit) bus(8 bit), offset(12 bit), segment(4 bit).
 *  SEGMENT:0 BUS 0:DEVICE 18 and SEGMENT:0 BUS 0:DEVICE 19 are inaccessable.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[out] out_buf 32 bit data from offset in PCI config space.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_bmc_ras_pcie_config_access(uint8_t soc_num,
					     struct pci_address pci_addr,
					     uint32_t *out_buf);

/**
 *  @brief Read number of MCA banks with valid status after a fatal error.
 *
 *  @details This function returns the number of MCA banks with valid status
 *  after a fatal error.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] bytes_per_mca returns bytes per mca.
 *
 *  @param[out] mca_banks number of mca banks.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_bmc_ras_mca_validity_check(uint8_t soc_num,
					     uint16_t *bytes_per_mca,
					     uint16_t *mca_banks);

/**
 *  @brief Read data from mca bank reported by bmc ras mca validity check.
 *
 *  @details This function returns the data from mca bank reported by bmc
 *  ras mca validity check.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] mca_dump mca_bank Struct containing offset, index of MCA bank.
 *
 *  @param[out] out_buf 32 bit data from offset in mca bank.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_bmc_ras_mca_msr_dump(uint8_t soc_num,
				       struct mca_bank mca_dump,
				       uint32_t *out_buf);

/**
 *  @brief Read FCH reason code from the previous reset.
 *
 *  @details This function reads the FCH reason code from the previous
 *  reset.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] input integer for id of FCH register.
 *
 *  @param[out] out_buf Data from FCH register.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_bmc_ras_fch_reset_reason(uint8_t soc_num,
					   uint32_t input,
					   uint32_t *out_buf);

/**
 *  @brief Read DIMM temperature range and refresh rate.
 *
 *  @details This function returns the per DIMM temperature range and
 *  refresh rate from the MR4 register, per JEDEC spec.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] dimm_addr Encoded address of the dimm.
 *
 *  @param[out] rate temp_refresh_rate structure with refresh rate(1 bit)
 *  and range(3 bit). 
 *  refresh rate: 0 = 1X, 1 = 2X.
 *  Temperature range: 001b = 1X, 101b = 2X.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_dimm_temp_range_and_refresh_rate(uint8_t soc_num,
						   uint32_t dimm_addr,
						   struct temp_refresh_rate *rate);

/**
 *  @brief Read DIMM power consumption.
 *
 *  @details This function returns the DIMM power consumption
 *  when bmc does not own the SPD side band bus.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] dimm_addr Encoded address of the dimm.
 *
 *  @param[out] dimm_pow struct dimm_power contains
 *  updaterate(ms): Time since last update (0-511ms).
 *	0 means last update was < 1ms, and 511 means update was >= 511ms
 *  power consumption(mw): power consumed (0 - 32767 mW)
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_dimm_power_consumption(uint8_t soc_num,
					 uint32_t dimm_addr,
					 struct dimm_power *dimm_pow);

/**
 *  @brief Read DIMM thermal sensor.
 *
 *  @details This function returns the DIMM thermal sensor
 *  (2 sensors per DIMM) when bmc does not own the SPD side band bus.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] dimm_addr Encoded address of the dimm.
 *
 *  @param[out] dimm_temp struct dimm_thermal struct contains
 *  updaterate(ms): Time since last update (0-511ms).
 *  0 means last update was < 1ms, and 511 means update was >= 511ms
 *  temperature (Degrees C): Temperature (-256 - 255.75 degree C)
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_dimm_thermal_sensor(uint8_t soc_num,
				      uint32_t dimm_addr,
				      struct dimm_thermal *dimm_temp);

/**
 *  @brief Read current active frequency limit per socket.
 *
 *  @details This function returns the current active frequency limit
 *  per socket.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] freq Frequency (MHz).
 *
 *  @param[out] source_type Source of limit.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_pwr_current_active_freq_limit_socket(uint8_t soc_num,
						       uint16_t *freq,
						       char **source_type);

/**
 *  @brief Read current active frequency limit set per core.
 *
 *  @details This function returns the current active frequency limit
 *  per core.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] core_id index.
 *
 *  @param[out] base_freq Frequency (MHz).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_pwr_current_active_freq_limit_core(uint8_t soc_num,
						     uint32_t core_id,
						     uint16_t *base_freq);

/**
 *  @brief Read SVR based telemtry for all rails.
 *
 *  @details This function returns the SVR based telemetry (power and update rate)
 *  for all rails.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] power SVI-based Telemetry for all rails(mW)
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_pwr_svi_telemetry_all_rails(uint8_t soc_num,
					      uint32_t *power);

/**
 *  @brief Read socket frequency range.
 *
 *  @details This function returns the fmax and fmin frequency
 *  per socket.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] fmax maximum frequency (MHz).
 *
 *  @param[out] fmin minimum frequency (MHz).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_socket_freq_range(uint8_t soc_num,
				    uint16_t *fmax,
				    uint16_t *fmin);

/**
 *  @brief Read current bandwidth on IO Link.
 *
 *  @details This function returns the current IO bandwidth.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] link link_id_bw_type struct containing bandwidth type
 *  and Link ID encoding
 *  bandwidth type:
 *	001b Aggregate BW
 *	Other Reserved
 *  MI300A APML Link ID Encoding:
 *	00000011b: P2
 *	00000100b: P3
 *	00001000b: G0
 *	00001001b: G1
 *	00001010b: G2
 *	00001011b: G3
 *	00001100b: G4
 *	00001101b: G5
 *	00001110b: G6
 *	00001111b: G7
 *  For other platforms the APML Link ID Encoding:
 *	00000001b: P0
 *	00000010b: P1
 *	00000100b: P2
 *	00001000b: P3
 *	00010000b: G0
 *	00100000b: G1
 *	01000000b: G2
 *	10000000b: G3
 *
 *  @param[out] io_bw io bandwidth (Mbps).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_current_io_bandwidth(uint8_t soc_num,
				       struct link_id_bw_type link,
				       uint32_t *io_bw);

/**
 *  @brief Read current bandwidth on xGMI Link.
 *
 *  @details This function returns the current xGMI bandwidth.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] link link_id_bw_type struct containing link id and
 *  bandwidth type info.
 *  Valid BW type are
 *	001b Aggregate BW
 *	010b Read BW
 *	100b Write BW
 *	Other Reserved
 *  MI300A APML Link ID Encoding:
 *      00000011b: P2
 *      00000100b: P3
 *      00001000b: G0
 *      00001001b: G1
 *      00001010b: G2
 *      00001011b: G3
 *      00001100b: G4
 *      00001101b: G5
 *      00001110b: G6
 *      00001111b: G7
 *  For other platforms the APML Link ID Encoding:
 *	00000001b: P0
 *	00000010b: P1
 *	00000100b: P2
 *	00001000b: P3
 *	00010000b: G0
 *	00100000b: G1
 *	01000000b: G2
 *	10000000b: G3
 *
 *  @param[out] xgmi_bw io bandwidth (Mbps).
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_current_xgmi_bandwidth(uint8_t soc_num,
					 struct link_id_bw_type link,
					 uint32_t *xgmi_bw);

/**
 *  @brief Set the max and min width of GMI3 link.
 *
 *  @details This function will set the max and min width of GMI3 Link.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] min_link_width minimum link width.
 *  0 = Quarter width
 *  1 = Half width
 *  2 = full width
 *
 *  @param[in] max_link_width maximum link width.
 *  0 = Quarter width
 *  1 = Half width
 *  2 = full width
 *  NOTE: max value must be greater than or equal to min value.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_gmi3_link_width_range(uint8_t soc_num,
					 uint8_t min_link_width,
					 uint8_t max_link_width);

/**
 *  @brief Set the max and min width of xGMI link.
 *
 *  @details This function will set the max and min width of xGMI Link.
 *  If this API is called from both the master and the slave sockets,
 *  then the largest width values from either calls are used.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] min_link_width minimum link width.
 *  0 = X4
 *  1 = X8
 *  2 = X16
 *
 *  @param[in] max_link_width maximum link width.
 *  0 = X4
 *  1 = X8
 *  2 = X16
 *  NOTE: Max value must be greater than or equal to min value.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_xgmi_link_width_range(uint8_t soc_num,
					 uint8_t min_link_width,
					 uint8_t max_link_width);

/**
 *  @brief Set the APBDisabled.
 *
 *  @details This function will set the APBDisabled by specifying the
 *  Data Fabric(DF) P-state. Messages APBEnable and APBDisable specify
 *  DF(Data Fabric) P-state behavior. DF P-states specify the frequency
 *  of clock domains from the CPU core boundary through to and including
 *  system memory, where 0 is the highest DF P-state and 2 is the lowest.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] df_pstate data fabric p-state.
 *
 *  @param[out] prochot_asserted prochot asserted status.
 *  True  indicates asserted
 *  False indicates not-asserted.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_apb_disable(uint8_t soc_num,
			       uint8_t df_pstate,
			       bool *prochot_asserted);

/**
 *  @brief Enable the DF p-state performance boost algorithm.
 *
 *  @details This function will enable the DF p-state performance
 *  boost algorithm.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] prochot_asserted prochot asserted status. True indicates asserted
 *  and false indicates not-asserted.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_apb_enable(uint8_t soc_num, bool *prochot_asserted);

/**
 *  @brief Read current DF p-state frequency .
 *
 *  @details This function returns the current DF p-state frequency.
 *  Returns the Fclck, DRAM memory clock(memclk),umc clock divider
 *  for the current socket DF P-state.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] df_pstate struct pstate_freq contains
 *  DRAM memory clock(mem clk)
 *  data fabric clock (Fclk)
 *  UMC clock divider
 *  Uclk = 0 means divide by 1 else divide by 2.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_current_dfpstate_frequency(uint8_t soc_num,
					     struct pstate_freq *df_pstate);
/**
 *  @brief Set the max and min LCK DPM Level on a given NBIO per socket.
 *
 *  @details This function will set the LCK DPM Level on a given NBIO per
 *  socket.The DPM Level is an encoding to represent the PCIE Link Frequency
 *  (LCLK) under a root complex (NBIO).
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] lclk lclk_dpm_level_range struct containing NBIOID (8 bit)
 *  Min dpm level (8 bit) and Max dpm level(8 bit). Valid NBIOID, min dpm
 *  level and max dpm level values are between 0 ~ 3.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_lclk_dpm_level_range(uint8_t soc_num,
					struct lclk_dpm_level_range lclk);
/**
 *  @brief Read RAPL (Running Average Power Limit) Units.
 *
 *  @details This function returns the RAPL (Running Average Power Limit)
 *  Units. Energy information (in Joules) is based on the multiplier: 1/(2^ESU).
 *  Time information (in Seconds) is based on the multiplier: 1/(2^TU).
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] tu_value TU value.
 *
 *  @param[out] esu_value esu value.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_bmc_rapl_units(uint8_t soc_num,
				 uint8_t  *tu_value,
				 uint8_t  *esu_value);

/**
 *  @brief Read RAPL base frequency per CPU socket.
 *
 *  @details This function returns the base frequency per CPU socket.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] base_freq base frequency.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_bmc_cpu_base_frequency(uint8_t soc_num,
					 uint16_t *base_freq);

/**
 *  @brief Control PCIe Rate on Gen5-Capable devices..
 *
 *  @details This function returns the PCIe rate.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] rate PCIe gen rate. 0 indicates Auto-Detect
 *  BW and set link rate accordingly. 1 is for Limit at
 *  Gen4 Rate. 2 is for Limit at Gen5 rate.
 *
 *  @param[out] mode previous mode.
 *  0 for Auto-Detect,
 *  1 for Limit at Gen4 rate,
 *  2 for limit at Gen5 rate.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_bmc_control_pcie_gen5_rate(uint8_t soc_num,
					     uint8_t rate,
					     uint8_t *mode);

/**
 *  @brief Read RAPL core energy counters.
 *
 *  @details This function returns the RAPL core energy counters.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Soskcet index.
 *
 *  @param[in] core_id core id.
 *
 *  @param[out] energy_counters core energy.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_rapl_core_energy_counters(uint8_t soc_num,
					    uint32_t core_id,
					    double *energy_counters);

/**
 *  @brief Read RAPL package energy counters.
 *
 *  @details This function returns the RAPL package energy counters.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] energy_counters core energy.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_rapl_pckg_energy_counters(uint8_t soc_num,
					    double *energy_counters);

/**
 *  @brief Write power efficiency profile policy.
 *
 *  @details This function writes power efficiency mode
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] mode power efficiency mode.
 *  0 indicates High Performance Mode
 *  1 indicates Power Efficiency Mode.
 *  2 indicates I/O Performance Mode.
 *  3 Balanced Memory Performance Mode.
 *  4 Balanced Core Performance Mode.
 *  5 Balanced Core and Memory Performance Mode.
 *
 *  Modes supported as per platform,
 *  0 to 2:
 *	Fam-19h_Mod-10h-1Fh
 *	Fam-19h_Mod-90h-9Fh
 *  0 to 5:
 *	Fam-1Ah_Mod-00h-0Fh
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_pwr_efficiency_mode(uint8_t soc_num,
				       uint8_t mode);

/**
 *  @brief Write df pstate range
 *
 *  @details This function writes df pstate range
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] max_pstate value.Max value must be
 *  less than or equal to min value. Valid values
 *  are 0 - 2.
 *
 *  @param[in] min_pstate value. Valid values are
 *  from 0 - 2.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_df_pstate_range(uint8_t soc_num,
				   uint8_t max_pstate,
				   uint8_t min_pstate);

/**
 *  @brief Read LCLK Max and Min DPM level range.
 *
 *  @details This function returns the LCLK Max and Min DPM level range.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] nbio_id nbio for a socket.
 *
 *  @param[out] dpm struct dpm level containing max and min dpm levels.
 *  Valid max and min dpm levels are from 0 - 1.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_lclk_dpm_level_range(uint8_t soc_num,
				       uint8_t nbio_id,
				       struct dpm_level *dpm);
/**
 *  @brief Read ucode revision.
 *
 *  @details This function reads the micro code revision number.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] ucode_rev micro code revision.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_ucode_revision(uint8_t soc_num, uint32_t *ucode_rev);

/**
 *  @brief Read BMC RAS reset on sync flood.
 *
 *  @details This function requests reset after sync flood. Reset
 *  only works during sync flood condition
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index. At present, only P0 handles this request.
 *
 *  @param[out] ack_resp acknowledgement response.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t reset_on_sync_flood(uint8_t soc_num, uint32_t *ack_resp);

/**
 *  @brief Overrides delay reset cpu on sync flood value.
 *
 *  @details This function will override delay reset cpu on sync flood value
 *  for the current boot instance. Delay value reverts to BIOS config selection
 *  after reboot.Number of override requests is limited to 5 per boot instance.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index. At present, only P0 handles this request.
 *
 *  @param[in] data_in struct ras_override_delay_d_in containing delay value
 *  override [5 - 120 mins], disable delay counter bit and stop delay counter
 *  bit.
 *
 *  @param[out] ack_resp acknowledgment response.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t override_delay_reset_on_sync_flood(uint8_t soc_num,
						struct ras_override_delay data_in,
						bool *ack_resp);
/**
 *  @brief Read post code.
 *
 *  @details This function will read most recent post code at the
 *  specified offset.SMU caches 8 most recent post codes.When the
 *  input is 0 the SMU will refresh the cache before running the
 *  latest post code. Input as 0 refers to most recent post code
 *  and higher inputs refers to the older post code.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] offset post code offset
 *
 *  @param[out] post_code recent post code
 *  of error category and number of instances of error category.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_post_code(uint8_t soc_num, uint32_t offset, uint32_t *post_code);

/**
 *  @brief Reads number of valid error instances per category.
 *
 *  @details This function will read number of valid error instances per
 *  category. Valid categories are MCA[00], DRAM CECC[01], PCIE [10],
 *  RSVD[11].
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] err_category Runtime error category MCA[00], DRAM CECC[01],
 *  PCIe[10] and RSVD [11].
 *
 *  @param[out] inst struct ras_rt_valid_err_inst containing number of bytes
 *  of error category and number of instances of error category.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_bmc_ras_run_time_err_validity_ck(uint8_t soc_num,
						  struct ras_rt_err_req_type err_category,
						  struct ras_rt_valid_err_inst *inst);

/**
 *  @brief Reads BMC RAS runtime error information.
 *
 *  @details This function will read BMC RAS runtime error information based
 *  on category. If category is MCA then it will read 32 bit of data from
 *  MCA MSR Bank.
 *  If category is DRAM CECC then will read error count, counter info and
 *  corresponding from the valid DRAM ECC correctable error counter instance.
 *  If category is PCIE then it returns 32 bit PCIE error data from given
 *  offset of given instance.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in]  d_in struct run_time_err_d_in contatining 4 byte aligned offset,
 *  runtime error category and 0 based index of valid instance returned by
 *  BMC_RAS_RUNTIME_ERR_VALIDITY_CHECK.
 *
 *  @param[out] err_info error information for a given category with valid instance
 *  and offset.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_bmc_ras_run_time_error_info(uint8_t soc_num,
					     struct run_time_err_d_in d_in,
					     uint32_t *err_info);

/**
 *  @brief Sets BMC RAS error threshold
 *
 *  @details This function will configure thresholding counters for MCA,
 *  DRAM CECC or PCIE.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] th struct run_time_threshold containing error type [00(MCA),
 *  01(DRAM CECC), 10(PCIE), error count threshold and
 *  max interrupt rate.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_bmc_ras_err_threshold(uint8_t soc_num,
				       struct run_time_threshold th);


/**
 *  @brief Configures OOB state infrastructure in SoC
 *
 *  @details This function will configure OOB state infrastructure in
 *  the SoC.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] d_in struct oob_config_d_in containing
 *  [0]   mca_oob_misc0_ec_enable,
 *  [2:1] dram_cecc_oob_ec_mode,
 *  [7:3] dram_cecc_leak_rate,
 *  [8]   pcie_err_reporting_en,
 *  [31]  mca_err_reporting_en.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_bmc_ras_oob_config(uint8_t soc_num,
				    struct oob_config_d_in d_in);

/**
 *  @brief Reads the current status of OOB state infrastructure in SoC
 *
 *  @details This function will read the current status of OOB state
 *  configuration in the SoC.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] oob_config containing
 *  [0]     mca_oob_misc0_ec_enable,
 *  [2:1]   dram_cecc_oob_ec_mode,
 *  [7:3]   dram_cecc_leak_rate,
 *  [8]     pcie_err_reporting_en,
 *  [11]    MCA Thresholding Interrupt Enable value
 *  [12]    DRAM CECC Thresholding Interrupt Enable value
 *  [13]    PCIe error Thresholding Interrupt Enable value
 *  [18:15] MCA MaxIntRate value
 *  [22:19] DRAM CECC MaxIntRate1 value
 *  [26:23] PCIe error MaxIntRate1 value
 *  [31]    mca_err_reporting_en
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_bmc_ras_oob_config(uint8_t soc_num, uint32_t *oob_config);

/**
 *  @brief Get the 64 bit PPIN fuse
 *
 *  @details This function will read the 64 bit PPIN fuse available via OPN_PPIN
 *  fuse.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] data PPIN fuse data
 *
 */
oob_status_t read_ppin_fuse(uint8_t soc_num, uint64_t *data);

/**
 *  @brief Read number of instances of DF blocks of type DF_BLOCK_ID with errors.
 *
 *  @details This function reads the number instance of DF blocks
 *  of type DF_BLOCK_ID that have an error log to report.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] df_block_id DF block ID.
 *
 *  @param[out] err_chk ras_df_err_chk Struct containing number of DF block
 *  instances andclength of error log in bytes per instanace.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_ras_df_err_validity_check(uint8_t soc_num,
					    uint8_t df_block_id,
					    struct ras_df_err_chk *err_chk);

/**
 *  @brief Read RAS DF error dump.
 *
 *  @details This function reads 32 bits of data from the offset provided for
 *  DF block instance retported by  read_ras_df_err_validity_check.
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh
 *  and \ref Fam-19h_Mod-90h-9Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] ras_err ras_df_err_dump Union containing 4 byte offset,
 *  DF block ID and block ID instance.
 *
 *  @param[out] data output data from offset of DF block instance.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t read_ras_df_err_dump(uint8_t soc_num,
				  union ras_df_err_dump ras_err,
				  uint32_t *data);

/**
 *  @brief Read RTC.
 *  @details Read RTC timer value. RTC time represents the year,
 *  month, day, hour, minute and seconds value in a 64b encoding.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] rtc [63:0] = RTC value,
 *	[31:0]=> DataIn==0 & [63:32]=> DataIn==4
 *	If DataIn==0 RTC=DD_hh_mm_ss
 *	If DataIn==4 RTC=00_YYYY_MM
 *	All digits in BCD format.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_rtc(uint8_t soc_num, uint64_t *rtc);

/**
 *  @brief Get SPD SB data
 *
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] spd_d_in struct dimm_spd_d_in containing
 *  DIMM address, LID, register offset, register space.
 *
 *  @param[out] spd_data containing spd data as per input
 */
oob_status_t read_dimm_spd_register(uint8_t soc_num,
				    struct dimm_spd_d_in spd_d_in,
				    uint32_t *spd_data);

/**
 *  @brief Get DIMM serial number
 *
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] dimm_addr DIMM address
 *
 *  @param[out] serial_num containing DIMM serial number
 */
oob_status_t get_dimm_serial_num(uint8_t soc_num,
				 uint8_t dimm_addr,
				 uint32_t *serial_num);

/**
 *  @brief Get SMU FW version
 *
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh
 *
 *  @param[in] soc_num Socket index.
 *
 *
 *  @param[out] smu_fw_ver SMU firmware version
 */
oob_status_t read_smu_fw_ver(uint8_t soc_num, uint32_t *smu_fw_ver);

/**
 *  @brief Write to BMC PCIE config.
 *
 *  @details This function will write 32 bit data_in to extended PCI config space
 *  of target specified in previous READ_BMC_RAS_PCIE_CONFIG_ACCESS (cmd ID 0x42h).
 *  Supported platforms: \ref Fam-19h_Mod-10h-1Fh, \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] pci_addr pci_address structure with fucntion(3 bit),
 *  device(4 bit) bus(8 bit), offset(12 bit), segment(4 bit).
 *  SEGMENT:0 BUS 0:DEVICE 18 and
 *  SEGMENT:0 BUS 0:DEVICE 19 are inaccessable.
 *
 *  @param[in] pcie_data data_in to extended PCI config space of target.
 *
 *  @param[out] r_code return code.Response is 0 if operation is successful.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t write_bmc_pcie_config(uint8_t soc_num, struct pci_address pci_addr,
				   uint32_t pcie_data, uint32_t *r_code);

/**
 *  @brief Set the max and min xGMI pstate.
 *
 *  @details This function will set the max and min xGMI pstate.
 *  pstate values are 0 - DFP0 (high performace) and 1 - DFP1
 *  (low performance). Max value must be less than or equal to
 *  min value. XGMI p-state can be set through either HSMP or APML
 *  and the value that was last set will be enforced.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] min_xgmi_pstate minimum xgmi pstate.Valid values are 0,1
 *
 *  @param[in] max_xgmi_pstate maximum xgmi pstate.Valid values are 0,1
 *
 *  NOTE: Max value must be less than or equal to min value.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_xgmi_pstate_range(uint8_t soc_num,
				   uint8_t min_xgmi_pstate,
				   uint8_t max_xgmi_pstate);

/**
 *  @brief Set the cpu rail iso frequency policy.
 *
 *  @details This function will set the cpu rail iso freq policy.
 *  This policy allows or disables independent core clocks per
 *  rail (VDDCR_CPU0 or VDDCR_CPU1).
 *  If a socket wide limit (e.g. PPT) is setting the core clock
 *  frequency, then this setting has no effect.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] policy frequency policy. 0 means both rails have
 *  same freqyency limit. 1 means each rail has independent frequency
 *  limit.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_cpu_rail_iso_freq_policy(uint8_t soc_num, uint8_t policy);

/**
 *  @brief Get the cpu rail iso frequency policy.
 *
 *  @details This function will get the cpu rail iso freq policy.
 *  The response 0 means both rails have same frequency limit and
 *  1 means each rail has independent frequency limit.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] policy current frequency policy. 0 means both rails have
 *  same freqyency limit. 1 means each rail has independent frequency
 *  limit.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_cpu_rail_iso_freq_policy(uint8_t soc_num, uint8_t *policy);

/**
 *  @brief Set the DF C-state enabling control.
 *
 *  @details This function will set the DF C-state enabling control.
 *  DF C-state is a low power state for IOD.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] state DF C-state. 0 to disable DFC 1 to enable DFC
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t set_dfc_enable(uint8_t soc_num, uint8_t state);

/**
 *  @brief Get the DF C-state enabling control.
 *
 *  @details This function will get the DF C-state enabling control.
 *  DF C-state is a low power state for IOD.
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] state current DF C-state
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_dfc_enable(uint8_t soc_num, uint8_t *state);

/**
 *  @brief Get the Avg DRAM throttle across all the channels.
 *
 *  @details This function will get the average DRAM throttle across
 *  all the channels
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] dram_throttle average dram throttle
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_avg_dram_throttle(uint8_t soc_num, uint32_t *dram_throttle);

/**
 *  @brief Get the channel DRAM throttle.
 *
 *  @details This function will get the DRAM throttle for the given channel
 *  Supported platforms: \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] dimm_addr dimm address for mode 1 containing umc instance ID,
 *  dimm 0/1 and temp sensor 0/1.
 *
 *  @param[out] dram_throttle average dram throttle
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t get_ch_dram_throttle(uint8_t soc_num, uint8_t dimm_addr,
				  uint32_t *dram_throttle);
/*
 * @} */  //  end of MailboxMsg
/****************************************************************************/

#endif  // INCLUDE_APML_MAILBOX_H_
