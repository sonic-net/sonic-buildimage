/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file XO2_dev.h
 * Definitions for the structures and types used by the API and Command
 * routines.
 * <p>
 * The XO2Handle_t contains the references to the specific hardware interface
 * that the commands and APIs will use (passed to them as an arg) for
 * accessing the specific XO2 device.
 * <p>
 * The XO2_JEDEC_t structure contains data from the converted JEDEC file,
 * using a utility to parse out the Configuration, UFM and other fields
 * from the JEDEC data representation to C variables.  The instance(s) of
 * this JEDEC file data are then passed to the program APIs to program
 * the configuration and/or UFM sectors with the binary data extracted
 * from the JEDEC file.
 */

#ifndef LATTICE_XO2_H
#define LATTICE_XO2_H

#include "eca_drvr.h"

#define XO2_FEATURE_ROW_FEATURE_LEN     (8)
#define XO2_FEATURE_ROW_FEABITS_LEN     (2)

#define XO2_FLASH_PAGE_SIZE (16)   /**< 16 bytes per page in Cfg and UFM sectors */
#define XO2_FLASH_PAGES_LEN(n) (n * XO2_FLASH_PAGE_SIZE)   /**< Number of bytes in that many pages */
//#define LATTICE_XO2_NUM_DEVS 9

typedef int Boolean_t;

/**
 * List of the various XO2 device types.
 * Used for indexing into the XO2 device data base for finding information
 * such as number of UFM pages, erase times, etc.
 *  @note this order must match the initialized order in XO2DevList.
 */
typedef enum
{
    MachXO2_256,       /**< XO2 256 LUT */
    MachXO2_640,       /**< XO2 640 LUT */
    MachXO2_640U,      /**< XO2 640U LUT with larger UFM size */
    MachXO2_1200,      /**< XO2 1200 LUT */
    MachXO2_1200U,     /**< XO2 1200 LUT with larger UFM size */
    MachXO2_2000,      /**< XO2 2000 LUT  */
    MachXO2_2000U,     /**< XO2 2000 LUT with larger UFM size */
    MachXO2_4000,      /**< XO2 4000 LUT device */
    MachXO2_7000,      /**< XO2 7000 LUT device */
    MachXO3_2100,
    MachXO3_4300c,
    Mach_END
} XO2Devices_t;

/**
 * Mode of XO2 device operation during programming and erasing operations.
 */
typedef enum
{
    TRANSPARENT_MODE,   /**< Erase and program sector while device is still operating. */
    OFFLINE_MODE        /**< Halt user mode during erase and program sector. */
} XO2CfgMode_t;

/**
 * Which sector to operate on.
 */
typedef enum
{
    CFG_SECTOR,   /**< Flash Configuration sector (holds design) */
    UFM_SECTOR,    /**< User Flash Memory sector (holds EBR values and general purpose usage) */
    FEATURE_ROW_H,
    SRAM
} XO2SectorMode_t;

/**
 * Device parameters needed for accessing and programming.
 */
typedef struct
{
    const char *pName;      /**< String for printing the XO2 devic epart number. */
    int        Cfgpages;    /**<  Number of Configuration pages in an XO2 device. */
    int        UFMpages;    /**<  Number of UFM pages in an XO2 device. */
    int        CfgErase;    /**< How long to wait (msec) for Configuration sector to erase. */
    int        UFMErase;    /**< How long to wait (msec) for UFM sector to erase. */
    int        Trefresh;    /**< Used for pausing after FlashCheck command or Refresh command. */
} XO2DevInfo_t;

/**
 * This structure associates the particular XO2 device with the access layer driver
 * functions required for reading/writing bytes to the XO2 device over a supported
 * protocol link (such as I2C or SPI).
 * Specific Driver information is kept in a user defined location (structure) pointed
 * to by pDrvrParams.  This pointer is passed to all driver calls so if a particular
 * driver for a certain communication link needs special parameters, values, settings,
 * etc. the information will be available to it when invoked.
 * <p>
 * The implementation of the drivers needed to implement read/write commands needs to
 * follow a specific signature. The list of driver routines is pointed to by pDrvrCalls.
 * @see ECADrvrCalls_t
 *
 */
typedef struct
{
    int             fd;
    Boolean_t       cfgEn;    /**< 1=Configuration Logic access is enabled, 0=not */
    XO2Devices_t    devType;     /**< XO2 part number for information about sizes and programming times */
    ECADrvrCalls_t  *pDrvrCalls; /**< specific driver routines for framing commands and reading/writing commands. */
    void            *pDrvrParams; /**< reference to any specific driver information needed to execute a read/write */

} XO2Handle_t;

/**
 * The bitfields of the Feature Row, extracted from the JEDEC file.
 * These bits fields are very important for device operation, so only
 * use values properly parsed from a valid XO2 JEDEC file.
 */
typedef struct
{
    unsigned char feature[XO2_FEATURE_ROW_FEATURE_LEN];
    unsigned char feabits[XO2_FEATURE_ROW_FEABITS_LEN];
} XO2FeatureRow_t;

/**
 * The extracted data from the JEDEC file.
 */
typedef struct
{
    XO2Devices_t devID;
    unsigned int pageCnt;
    unsigned int CfgDataSize;
    unsigned int UFMDataSize;
    unsigned int UserCode;
    unsigned int SecurityFuses;
    unsigned char *pCfgData;
    unsigned char *pUFMData;
    XO2FeatureRow_t *pFeatureRow;
} XO2_JEDEC_t;

/**
 * Struture to return register information read from the XO2 configuration block.
 * @see XO2ECA_apiGetHdwInfo
 */
typedef struct
{
    unsigned int  devID;      /**< Lattice Device ID code for this XO2 device */
    unsigned int  UserCode;   /**< 32 bit user code set in XO2 */
    unsigned char TraceID[8]; /**< 8 byte trace ID prorammed into XO2 */
    int       devInfoIndex;  /**< to index into the pXO2DevList[] for more info */
} XO2RegInfo_t;

#define LATTICE_XO2_NUM_DEVS    Mach_END

#ifndef XO2DEVLIST_DECLARATION
// Prototype for device data base
extern const XO2DevInfo_t XO2DevList[LATTICE_XO2_NUM_DEVS];

#endif

#endif
