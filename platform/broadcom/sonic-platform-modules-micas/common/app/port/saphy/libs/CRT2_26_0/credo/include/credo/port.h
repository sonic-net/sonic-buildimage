#ifndef CREDO_PORT_H
#define CREDO_PORT_H

#include "credo/base.h"
#include "credo/params.h"
#include "credo/rsfec.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Identifer for the connection type of the port
 * Used to indicate what type of mode to set the port depending on usage.
 *
 */
typedef enum { CR_PORT_RETIMER, CR_PORT_BITMUX, CR_PORT_GEARBOX, CR_PORT_SWITCHOVER_RETIMER } CredoPortType_t;

#ifndef CREDO_NO_RENAMED
/**
 * @brief Backwards compatible alias for PortType
 *
 */
typedef CredoPortType_t CredoPortConnectionMode_t;
#endif

/**
 *
 * @brief Identifier to indicate how the port connects with the next layer.
 */
typedef enum {
    CR_PMODE_SERDES,  //!< basic Lane only mode
    CR_PMODE_PCS,     //!< serdes + pcs used (no macsec)
    CR_PMODE_MACSEC   //!< macsec all blocks used
} CredoPortMode_t;

/**
 * @brief Identifer for the direction of the port traffic
 * Used to indicate what dirction of the port.
 *
 */
typedef enum {
    CR_PORT_DIR_BIDIRECTIONAL = 0,
    CR_PORT_DIR_EGRESS = 1,
    CR_PORT_DIR_HOST_TO_LINE = 1,  //!< verbose name for egress
    CR_PORT_DIR_INGRESS = 2,
    CR_PORT_DIR_LINE_TO_HOST = 2  //!< verbose name for ingress
} CredoPortDirection_t;

/**
 * @brief Auto assign port
 *
 * Use as the port id.
 */
#define CR_PORT_AUTO_ASSIGN_ID 0x10000

/**
 * @brief
 * @note it is the same value as CR_PORT_AUTO_ASSIGN_ID.
 * When querying a port will return this value in port_id if the port is not configured. Note
 */
#define CR_PORT_UNCONFIGURED 0x10000

// flags for CredoPortConfig_t use
/**
 * @brief Port config flag to indicate line side optical
 */
#define CR_PFLAG_LINE_SIDE_OPTICAL (1 << 0)

/**
 * @brief Port config flag to indicate line side ANLT
 */
#define CR_PFLAG_LINE_SIDE_ANLT (1 << 1)

/**
 * @brief Port config flag to indicate system side LT.
 * @details This is only effective for PAM4/Clause 136 link training.
 *          For NRZ link in system side, this flag is ignored.
 *          For Lane only modes, this flag is also ignored.
 */
#define CR_PFLAG_SYS_SIDE_LT (1 << 2)

/**
 * @brief Port config flag to indicate system side optical.
 *
 */
#define CR_PFLAG_SYS_SIDE_OPTICAL (1 << 3)

/**
 * @brief Port config flag to indicate line side AN
 */
#define CR_PFLAG_LINE_SIDE_AN (1 << 4)

/**
 * @brief Port config flag to indicate line side LT
 */
#define CR_PFLAG_LINE_SIDE_LT (CR_PFLAG_LINE_SIDE_ANLT | CR_PFLAG_AUTONEG_DISABLE)

/**
 * @brief Port config flag to override auto negotiation
 */
#define CR_PFLAG_AUTONEG_OVERRIDE (1 << 8)

/**
 * @brief Port config flag to disable line side auto negotiation.
 * @details This is only effective for PAM4/Clause 136 link training.
 *          If line side is configured as NRZ mode, an error will be returned.
 *          This flag has higher priority than {c:any}`CR_PFLAG_AUTONEG_OVERRIDE`.
 */
#define CR_PFLAG_AUTONEG_DISABLE (1 << 9)

/**
 * @brief Port config flag to enable double CRC (double CRC is disabled by default)
 */
#define CR_PFLAG_ENABLE_DOUBLE_CRC (1 << 16)

/**
 * @brief Information needed to configure a port
 */
typedef struct {
    uint32_t port_id;                 //!< Which port id to use in firmware
    uint32_t flags;                   //!< special flags to set about the port
    CredoPortType_t connection_mode;  //!< type of port to configure
    CredoPortMode_t port_mode;        //!< how the port is connected
    uint32_t speed;                   //!< Speed of the port in Kb/s
    uint32_t line_start_lane;         //!< initial lane of line side to use
    uint32_t host_start_lane;         //!< initial lane of host side to use
    uint32_t line_no_of_lanes;        //!< how many lanes are used on line side
    uint32_t host_no_of_lanes;        //!< how many lanes are used on host side
    CredoFecType_t line_fec_type;     //!< line side fec type to use
    CredoFecType_t host_fec_type;     //!< host side fec type to use
} CredoPortConfig_t;

/**
 * @brief Port setup for building a port
 */
typedef struct {
    CredoPortType_t mode;
    unsigned speed;
    int host_lanes[32];
#ifdef SWIG
%immutable;
#endif
    unsigned host_count;
#ifdef SWIG
%mutable;
#endif
    int line_lanes[32];
#ifdef SWIG
%immutable;
#endif
    unsigned line_count;
#ifdef SWIG
%mutable;
#endif
} CredoPortSetup_t;

// Port Operations

/**
 * @brief Configure a port through firmware
 * @param[in] slice slice handle
 * @param[in] port_config port configuration. If port_config.port_id == CR_PORT_AUTO_ASSIGN_ID, port_config.port_id will
 * be the identifier assigned by firmware on exit
 * @param[in] force force port configuration if lanes already in use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_configure(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force);

/**
 * @brief Destroy a port through firmware
 * @param[in] slice slice handle
 * @param[in] portId port to destroy
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_destroy(CredoSlice_t* slice, uint32_t portId);

/**
 * @brief Get information about a configured port
 * @param[in] slice slice handle
 * @param[in] portId port to get infromation
 * @param[out] port_config port configuration that is provided
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_query(CredoSlice_t* slice, uint32_t portId, CredoPortConfig_t* port_config);

/**
 * @brief Check if a port is linked and operational
 *
 * The definition of "up"/"operational" will depend on both the chip and the port's layer. Refer to your chip's
 * documentation for more information.
 *
 * @param[in] slice slice handle
 * @param[in] portId port to check
 * @param[in] side which side to check is outputting an up link
 * @param[out] up is the port up?
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_is_link_up(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, bool* up);

/**
 * @brief Destroy all ports configured
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_destroy_all(CredoSlice_t* slice);

/**
 * @addtogroup PortParam
 * @{
 */
#define CR_PPARAM_CIPHER_SUITE         "cipher_suite"
#define CR_PPARAM_PACKET_EXPANSION     "packet_expansion"
#define CR_PPARAM_EXTENDED_PAD_REMOVAL "extended_pad_removal"
#define CR_PPARAM_LAYER                "layer"
#define CR_PPARAM_LINK                 "link"
#define CR_PPARAM_LINK_HOST            "link_host"
#define CR_PPARAM_LINK_LINE            "link_line"
#define CR_PPARAM_HOST_MAC             "host_mac"
#define CR_PPARAM_LINE_MAC             "line_mac"
#define CR_PPARAM_HOST_PCS             "host_pcs"
#define CR_PPARAM_LINE_PCS             "line_pcs"
#define CR_PPARAM_HOST_REG_SPACE       "host_reg_space"
#define CR_PPARAM_LINE_REG_SPACE       "line_reg_space"
#define CR_PPARAM_BITMUX_FIFO          "bitmux_fifo"
#define CR_PPARAM_RETIMER_FIFO         "retimer_fifo"

typedef enum { CR_CIPHER_AES_GCM = 0, CR_CIPHER_SM4 = 1 } CredoCipher_t;
/** @} */

/**
 * @brief Get a port option
 * @param[in] slice slice to use
 * @param[in] port_id
 * @param[in] name
 * @param[out] value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_get_option(CredoSlice_t* slice, int port_id, const char* name, int* value);
/**
 * @brief Set a port option
 * @param[in] slice
 * @param[in] port_id
 * @param[in] name
 * @param[in] value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_set_option(CredoSlice_t* slice, int port_id, const char* name, int value);

/**
 * @brief A more powerful replacement for cr_port_configure()
 *
 * This function splits configuring a port into 3 separate steps.
 * 1. Build cr_port_build()
 * 2. Options: this may not need to be done depending what kind of port you are configuring cr_port_set_option()
 * 3. Start cr_port_start()
 *
 * This function may support basic error checking for what kind of port you are configuring, but cr_port_start which
 * talks to firmware is the true arbiter of determining if it is a valid port setup.
 *
 * @param[in] slice slice to use
 * @param[in] port_id port to build
 * @param[in] setup setup to
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_build(CredoSlice_t* slice, uint32_t port_id, const CredoPortSetup_t* setup);

/**
 * @brief Start a built port
 * Once a port is built and all options are configured, you can start the port.
 *
 * Force is used to destroy other ports under the following conditions:
 * 1. The same port id
 * 2. The other port(s) share a lane (host or line) with the port to start
 *
 * If force is false then the port start will fail instead if it faces those conditions.
 *
 * @param[in] slice slice to use
 * @param[in] port_id port id to use
 * @param[in] force whether to force starting the port
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_start(CredoSlice_t* slice, uint32_t port_id, bool force);

/**
 * @brief Similar to port query but provides less information
 *
 * To get information not included in CredoPortSetup_t you should use cr_port_get_option() or other port funcions.
 *
 * @note This captures setup information on launched ports (not built ports).
 *
 * @param[in] slice slice
 * @param[in] port_id port to configure
 * @param[out] started indicates if the port is started
 * @param[out] setup setup information, if started=false then the setup struct values are junk
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_get_setup(CredoSlice_t* slice, uint32_t port_id, bool* started, CredoPortSetup_t* setup);

/**
 * @brief Determine a free port id available to configure
 *
 * Will fail if no port id is free.
 *
 * @param[in] slice
 * @param[out] port_id
 * @return Error Code (no available port)
 */
CREDOAPI CredoError_t cr_port_assign_id(CredoSlice_t* slice, uint32_t* port_id);

/** @brief Get serdes parameter
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[in,out] data You must provide the value storage, type, and count. param.value buffer will be updated
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_get_param(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data);

/**
 * @brief Get serdes param information and use heap
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[out] data parameter data, you must free after use
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_get_paramh(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data);

/**
 * @brief Set serdes parameter
 *
 * @param[in] slice
 * @param[in] name
 * @param[in] index
 * @param[out] data
 * @return Error Code
 */
CREDOAPI CredoError_t cr_port_set_param(CredoSlice_t* slice, const char* name, int index, const CredoParamData_t* data);

// utility for c11 that enables easy overload access
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#define cr_port_get_paramv(slice, name, index, val, count) cr_port_get_param(slice, name, index, CR_PDATA(val, count))

#define cr_port_set_paramv(slice, name, index, val, count) cr_port_set_param(slice, name, index, CR_PDATA(val, count))

#endif

#ifdef __cplusplus
}
#endif

#endif
