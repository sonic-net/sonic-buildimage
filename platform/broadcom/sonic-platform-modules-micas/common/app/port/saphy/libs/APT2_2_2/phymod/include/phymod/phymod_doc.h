/*
 *         
 * $Id: phymod.xml,v 1.1.2.5 Broadcom SDK $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *         
 *
 */
/*!
@page phymod-doc PHYMod Documentation
@tableofcontents
@section Introduction
@version 0.1

Phy is the gate for each network device to the world. It is usually common, and
Phys from the same type are integrated into a range of devices and
architectures. Phymod is new phy driver architecture for phy devices. It is
stateless, system independent library, which provide APIs for the set of feature
provided by the Phy. This document has two main sections relevent to:

-# Application developers, which will use the phymod for all phy operations.
-# Phy developers, which will write their phy driver under the phymod envelope.

The full Phymod APIs list is described in (FIXME: which doc. describes APIs?)

\section phymod-too Theory of Operation

\subsection Hier-str Hierarchical structure

The hierarchical layering of the PHYMod software is shown in figure below.

\image html phymod_arch.jpg "PHYMod Architecture"

The layers (from highest to lowest) are as follows.
\li Application: Uses the library APIs. The supplied package contains 'phymod_diag'.
\li PHYMod API: The library interface. Dispatch layer for the different Serdes types.
\li Tier2: A Sequence of tier1 functions to accomplish a complete PHY configuration.
\li Tier1: A set of PHY strictly ordered register operations for a sub-configuration.
\li Access: Platform specific register access mechanism

\subsection API-str API structure

Each Phymod API has the format:
\code
phymod_core_func(plp_aperta2_phymod_core_access_t, ...);
phymod_phy_func(plp_aperta2_phymod_phy_access_t, ...);
\endcode

The phymod_core APIs receive plp_aperta2_phymod_core_access_t structure. This set of APIs
performs operations that affect the entire core. The phymod_phy APIs receive
plp_aperta2_phymod_phy_access_t structure. This set of APIs performs operations that affect
single lane, or a given group of lanes within a core.

Both phy and core structure contains the following two members:
\li plp_aperta2_phymod_dispatch_type_t - the driver type, this parameter is used for the
 dispatcher (can be retrieved using the probe function that will be presented
 later in this document).
\li plp_aperta2_phymod_access_t Access information structure.

Note: for each structure in Phymod there is matching init function named
phymod_structure_name_t_init() which initialize the structure parameters to
default. Although its not forced in code, it is assumed that each structure
is initialized before used.

\section Int-with-Phymod Integrating with Phymod

\subsection Acc-Str Access Structure

As described above, phymod APIs get core/phy access structure as parameter. This
section describes the preliminary required operations for creating this access
structures.

\subsubsection Acc-mem Access members
\li user_acc - data that provided for each plp_aperta2_phymod_access_t, and is used in every
PHY register read/write and mutex take/give operations. This structure is passed
directly from the access struct to the registers and mutex callbacks (described
below), and is not used by the phymod library.
\li Bus - The bus is required for register read/write operations.
\li Flags currently single flag exist
\li PHYMOD_PHYMOD_ACC_F_CLAUSE45 indicate that core address should be converted to CL45 format.
\li Lane A bitmap of lanes that should be accessed. For phymod_core APIs this field is ignored.
\li Addr Core Address
\li Devad - Clause 45 device address. Use 0 if not relevant.

\subsubsection bus-str Bus structure

The bus structure defines the read/write of single register.
\code
typedef struct plp_aperta2_phymod_bus_s {
  char* bus_name; 
  phymod_bus_read_f read;
  phymod_bus_write_f write;
  phymod_bus_mutex_take_f mutex_take;
  phymod_bus_mutex_give_f mutex_give;
  uint32_t bus_capabilities;
} plp_aperta2_phymod_bus_t;
\endcode

In order to initialize the structure call the function plp_aperta2_phymod_bus_t_init(bus_structure);

\subsubsection  Read/write functions

The read/write functions provide register access from the Phymod library to the device.

The callbacks prototype:

\code
typedef int (*phymod_bus_read_f)( void* user_acc, uint32_t core_addr, uint32_t reg_addr, uint32_t* val);
typedef int (*phymod_bus_write_f)( void* user_acc, uint32_t core_addr, uint32_t reg_addr, uint32_t val);
\endcode

The callback parameters:

\li user_acc - data that provided for each plp_aperta2_phymod_access_t, and can be used in
every PHY register read/write. This structure is passed directly from the access
struct to the register read/write callback, and is not used by the phymod
library.
\li core_addr- the core address
\li reg_addr - register address to read or write
\li val - register read/write value

\subsubsection mutex-take-give-func Mutex take/give functions
The mutex callbacks provide built-in thread-safty protection.  These callbacks
are optional, a NULL can be assigned in case this protection is not required.
The mutex take called before every Tier2 fucntion, and the mutex give called
after every Tier2 function.

\li The callback prototype:
\code
typedef int (*phymod_bus_mutex_take_f)(void* user_acc);
typedef int (*phymod_bus_mutex_give_f)(void* user_acc);
\endcode

The user_acc is the same pointer that passed to read/write functions. The user
that implements these callbacks should identify for the user_acc which mutex
should be used. It is recommended to use per-core mutex. 

\subsubsection bus-cap bus capabilities
A set of flags to indicate flag capabilities:
- PHYMOD_BUS_CA_WR_MODIFY - The bus support modify functionality
- PHYMOD_BUS_CA_LANE_CTRL - The bus support direct lane configuration

\subsubsection PHY-Core-acc-init PHY/Core access init

-# Initialize the struct
\code
plp_aperta2_phymod_core_access_t core; 
plp_aperta2_phymod_core_access_t_init(&core.access);
\endcode
For Phy (i.e. for each port)
\code
plp_aperta2_phymod_phy_access_t phy;
plp_aperta2_phymod_phy_access_t_init(&phy.access);
\endcode
-# Set the access fields
\code
PHYMOD_ACC_USER_ACC(&core.access) = user_access; //the user specific data
PHYMOD_ACC_BUS(&core.access) = bus; //bus structure
PHYMOD_ACC_FLAGS(&core.access) = flags;//access flags
PHYMOD_ACC_ADDR(&core.access) = core_addr; //the core address 
PHYMOD_ACC_DEVAD(&core.access) = dev_addr; //CL45 device addr. default if not specified
PHYMOD_ACC_LANE_MASK(&phy.access) = lanes; //bit map of lanes in use by the phy
core.type = type; //set the core type
\endcode

For more information of how to get the core type read about plp_aperta2_phymod_core_probe.

\subsubsection plp_aperta2_phymod_core_probe plp_aperta2_phymod_core_probe
This function is used to get the core type.
\code
int plp_aperta2_phymod_core_probe(const plp_aperta2_phymod_access_t* access, plp_aperta2_phymod_dispatch_type_t* type);
\endcode
Parameters:
\li Access - access structure.
\li Type - The function output. The core type in case that the core was not identified PHYMOD_E_INIT returned.

\subsection Phy-Init-Seq Phymod Init Sequence
TBD

\section diag-pkt-integ Diagnostics Package Integration

This part describes how to integrate the Phymod diagnostics.  The phymod
diagnotics header file is phymod_diag.h.  The common diagnostics implemented in
the package contains the required data collection and result printing. The
parameters parsing process is done outside of the Phymod package (see figure
below)

@image html phymod_diag.jpg

_phymod_diag_print_func_register must be called before the first call of one of
the diagnostics function. This function gets a print function:
\code
typedef int (*print_func_f)(const char *, ...); 
\endcode
And will be used for all the messages printed by the diagnostics.

There are several PHY drivers that live in PHYMod.  The working of the various
layers to create various drivers and interacting with the adopting platform is
shown below.

- TEMod : The PHY driver for the 10G link, 28nm PHY with TSCE PCS and Eagle PMD
- TEFMod : The PHY driver for the 25G link, 28nm PHY with TSCF PCS and Falcon PMD
- QEMod : The PHY driver for the 1G link, 28nm PHY with QSGMII PCS and Eagle PMD
- EagleMod : The PHY driver for the 28nm PHY Eagle PMD (No PCS)
- FalconMod : The PHY driver for the 28nm PHY Falcon PMD (No PCS)

Devices working on non-XGS-SDK platforms will interact directly with the PHYMod
dispatch layer. The XGS SDK needs a 'glue layer' to conform to legacy.

@image html PHYmily.jpg

\subsection reg-env Regression Environment
Refer to the Port Regression and Interop Lab document (TBD).

*/

