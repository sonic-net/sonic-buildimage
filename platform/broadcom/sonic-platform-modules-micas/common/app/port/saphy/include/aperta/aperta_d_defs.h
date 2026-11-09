#ifndef APERTA_D_DEFS_H
#define APERTA_D_DEFS_H
#include <stdint.h>

#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r (0x00018201 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_GEN_CONTROL1.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_s {
     uint32_t v[1];
     uint32_t gen_cntrls_gen_control1[1];
     uint32_t _gen_cntrls_gen_control1;
} BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_t;

#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_CLR(r) (r).gen_cntrls_gen_control1[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SET(r,d) (r).gen_cntrls_gen_control1[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_GET(r) (r).gen_cntrls_gen_control1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_MODCTRL_RSTBf_GET(r) ((((r).gen_cntrls_gen_control1[0]) >> 6) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_MODCTRL_RSTBf_SET(r,f) (r).gen_cntrls_gen_control1[0]=(((r).gen_cntrls_gen_control1[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_GET(r) ((((r).gen_cntrls_gen_control1[0]) >> 1) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(r,f) (r).gen_cntrls_gen_control1[0]=(((r).gen_cntrls_gen_control1[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_RESETBf_GET(r) (((r).gen_cntrls_gen_control1[0]) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_RESETBf_SET(r,f) (r).gen_cntrls_gen_control1[0]=(((r).gen_cntrls_gen_control1[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access GEN_CNTRLS_GEN_CONTROL1.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL1r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r,(_r._gen_cntrls_gen_control1))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r,(_r._gen_cntrls_gen_control1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_GEN_CONTROL1r BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r
#define GEN_CNTRLS_GEN_CONTROL1r_SIZE BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_t GEN_CNTRLS_GEN_CONTROL1r_t;
#define GEN_CNTRLS_GEN_CONTROL1r_CLR BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_CLR
#define GEN_CNTRLS_GEN_CONTROL1r_SET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SET
#define GEN_CNTRLS_GEN_CONTROL1r_GET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_GET
#define GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_GET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_GET
#define GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET
#define GEN_CNTRLS_GEN_CONTROL1r_RESETBf_GET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_RESETBf_GET
#define GEN_CNTRLS_GEN_CONTROL1r_RESETBf_SET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_RESETBf_SET
#define READ_GEN_CNTRLS_GEN_CONTROL1r BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL1r
#define WRITE_GEN_CNTRLS_GEN_CONTROL1r BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r
#define MODIFY_GEN_CNTRLS_GEN_CONTROL1r BCMI_APERTA_D_MODIFY_GEN_CNTRLS_GEN_CONTROL1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_GEN_CONTROL2
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8202
 * DEVAD:    1
 * DESC:     General control register 2.
 * RESETVAL: 0x3 (3)
 * ACCESS:   R/W
 * FIELDS:
 *     MST_RSTB         Reset for Master M0 micro (Active low).
 *     MST_UCP_RSTB     Reset for Master M0 micro peripherals(Active low).
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr (0x00018215 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_FIRMWARE_VERSION.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_s {
     uint32_t v[1];
     uint32_t gen_cntrls_firmware_version[1];
     uint32_t _gen_cntrls_firmware_version;
} BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_t;

#define BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_CLR(r) (r).gen_cntrls_firmware_version[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_SET(r,d) (r).gen_cntrls_firmware_version[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_GET(r) (r).gen_cntrls_firmware_version[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_GET(r) (((r).gen_cntrls_firmware_version[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_SET(r,f) (r).gen_cntrls_firmware_version[0]=(((r).gen_cntrls_firmware_version[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_FIRMWARE_VERSION.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_FIRMWARE_VERSIONr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr,(_r._gen_cntrls_firmware_version))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_FIRMWARE_VERSIONr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr,(_r._gen_cntrls_firmware_version))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_FIRMWARE_VERSIONr BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr
#define GEN_CNTRLS_FIRMWARE_VERSIONr_SIZE BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_t GEN_CNTRLS_FIRMWARE_VERSIONr_t;
#define GEN_CNTRLS_FIRMWARE_VERSIONr_CLR BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_CLR
#define GEN_CNTRLS_FIRMWARE_VERSIONr_SET BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_SET
#define GEN_CNTRLS_FIRMWARE_VERSIONr_GET BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_GET
#define GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_GET BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_GET
#define GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_SET BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_SET
#define READ_GEN_CNTRLS_FIRMWARE_VERSIONr BCMI_APERTA_D_READ_GEN_CNTRLS_FIRMWARE_VERSIONr
#define WRITE_GEN_CNTRLS_FIRMWARE_VERSIONr BCMI_APERTA_D_WRITE_GEN_CNTRLS_FIRMWARE_VERSIONr
#define MODIFY_GEN_CNTRLS_FIRMWARE_VERSIONr BCMI_APERTA_D_MODIFY_GEN_CNTRLS_FIRMWARE_VERSIONr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_FIRMWARE_VERSIONr'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_FIRMWARE_ENABLE
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8216
 * DEVAD:    1
 * DESC:     Firmware enable
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     FW_ENABLE_VAL    When set to 1, special firmware features are enabled
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r (0x00018251 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_GPREG_01.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_s {
     uint32_t v[1];
     uint32_t gen_cntrls_gpreg_01[1];
     uint32_t _gen_cntrls_gpreg_01;
} BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t;

#define BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_CLR(r) (r).gen_cntrls_gpreg_01[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_SET(r,d) (r).gen_cntrls_gpreg_01[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GET(r) (r).gen_cntrls_gpreg_01[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(r) (((r).gen_cntrls_gpreg_01[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_SET(r,f) (r).gen_cntrls_gpreg_01[0]=(((r).gen_cntrls_gpreg_01[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_GPREG_01.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_GPREG_01r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r,(_r._gen_cntrls_gpreg_01))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_GPREG_01r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r,(_r._gen_cntrls_gpreg_01))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_GPREG_01r BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r
#define GEN_CNTRLS_GPREG_01r_SIZE BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t GEN_CNTRLS_GPREG_01r_t;
#define GEN_CNTRLS_GPREG_01r_CLR BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_CLR
#define GEN_CNTRLS_GPREG_01r_SET BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_SET
#define GEN_CNTRLS_GPREG_01r_GET BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GET
#define GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET
#define GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_SET BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_SET
#define READ_GEN_CNTRLS_GPREG_01r BCMI_APERTA_D_READ_GEN_CNTRLS_GPREG_01r
#define WRITE_GEN_CNTRLS_GPREG_01r BCMI_APERTA_D_WRITE_GEN_CNTRLS_GPREG_01r
#define MODIFY_GEN_CNTRLS_GPREG_01r BCMI_APERTA_D_MODIFY_GEN_CNTRLS_GPREG_01r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_GPREG_02
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8252
 * DEVAD:    1
 * DESC:     General purpose register 02 for micro/external communication
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     GPREG_02_DATA    Data
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r (0x00018230 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_DWNLD_00.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_s {
     uint32_t v[1];
     uint32_t gen_cntrls_dwnld_00[1];
     uint32_t _gen_cntrls_dwnld_00;
} BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_t;

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_CLR(r) (r).gen_cntrls_dwnld_00[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_SET(r,d) (r).gen_cntrls_dwnld_00[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_GET(r) (r).gen_cntrls_dwnld_00[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_DOWNLOAD_00f_GET(r) (((r).gen_cntrls_dwnld_00[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_DOWNLOAD_00f_SET(r,f) (r).gen_cntrls_dwnld_00[0]=(((r).gen_cntrls_dwnld_00[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_DWNLD_00.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_00r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r,(_r._gen_cntrls_dwnld_00))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_00r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r,(_r._gen_cntrls_dwnld_00))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_DWNLD_00r BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r
#define GEN_CNTRLS_DWNLD_00r_SIZE BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_t GEN_CNTRLS_DWNLD_00r_t;
#define GEN_CNTRLS_DWNLD_00r_CLR BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_CLR
#define GEN_CNTRLS_DWNLD_00r_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_SET
#define GEN_CNTRLS_DWNLD_00r_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_GET
#define GEN_CNTRLS_DWNLD_00r_DOWNLOAD_00f_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_DOWNLOAD_00f_GET
#define GEN_CNTRLS_DWNLD_00r_DOWNLOAD_00f_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r_DOWNLOAD_00f_SET
#define READ_GEN_CNTRLS_DWNLD_00r BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_00r
#define WRITE_GEN_CNTRLS_DWNLD_00r BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_00r
#define MODIFY_GEN_CNTRLS_DWNLD_00r BCMI_APERTA_D_MODIFY_GEN_CNTRLS_DWNLD_00r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_DWNLD_00r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_DWNLD_01
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8231
 * DEVAD:    1
 * DESC:     Firmware download register 01
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     DOWNLOAD_01      download related registers
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r (0x00018231 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_DWNLD_01.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_s {
     uint32_t v[1];
     uint32_t gen_cntrls_dwnld_01[1];
     uint32_t _gen_cntrls_dwnld_01;
} BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_t;

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_CLR(r) (r).gen_cntrls_dwnld_01[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_SET(r,d) (r).gen_cntrls_dwnld_01[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_GET(r) (r).gen_cntrls_dwnld_01[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_GET(r) (((r).gen_cntrls_dwnld_01[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_SET(r,f) (r).gen_cntrls_dwnld_01[0]=(((r).gen_cntrls_dwnld_01[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_DWNLD_01.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_01r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r,(_r._gen_cntrls_dwnld_01))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_01r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r,(_r._gen_cntrls_dwnld_01))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_DWNLD_01r BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r
#define GEN_CNTRLS_DWNLD_01r_SIZE BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_t GEN_CNTRLS_DWNLD_01r_t;
#define GEN_CNTRLS_DWNLD_01r_CLR BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_CLR
#define GEN_CNTRLS_DWNLD_01r_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_SET
#define GEN_CNTRLS_DWNLD_01r_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_GET
#define GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_GET
#define GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_SET
#define READ_GEN_CNTRLS_DWNLD_01r BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_01r
#define WRITE_GEN_CNTRLS_DWNLD_01r BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_01r
#define MODIFY_GEN_CNTRLS_DWNLD_01r BCMI_APERTA_D_MODIFY_GEN_CNTRLS_DWNLD_01r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_DWNLD_01r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_DWNLD_02
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8232
 * DEVAD:    1
 * DESC:     Firmware download register 02
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     DOWNLOAD_02      download related registers
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r (0x00018233 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_DWNLD_03.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_s {
     uint32_t v[1];
     uint32_t gen_cntrls_dwnld_03[1];
     uint32_t _gen_cntrls_dwnld_03;
} BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_t;

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_CLR(r) (r).gen_cntrls_dwnld_03[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_SET(r,d) (r).gen_cntrls_dwnld_03[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_GET(r) (r).gen_cntrls_dwnld_03[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_GET(r) (((r).gen_cntrls_dwnld_03[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_SET(r,f) (r).gen_cntrls_dwnld_03[0]=(((r).gen_cntrls_dwnld_03[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_DWNLD_03.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_03r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r,(_r._gen_cntrls_dwnld_03))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_03r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r,(_r._gen_cntrls_dwnld_03))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_DWNLD_03r BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r
#define GEN_CNTRLS_DWNLD_03r_SIZE BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_t GEN_CNTRLS_DWNLD_03r_t;
#define GEN_CNTRLS_DWNLD_03r_CLR BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_CLR
#define GEN_CNTRLS_DWNLD_03r_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_SET
#define GEN_CNTRLS_DWNLD_03r_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_GET
#define GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_GET
#define GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_SET
#define READ_GEN_CNTRLS_DWNLD_03r BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_03r
#define WRITE_GEN_CNTRLS_DWNLD_03r BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_03r
#define MODIFY_GEN_CNTRLS_DWNLD_03r BCMI_APERTA_D_MODIFY_GEN_CNTRLS_DWNLD_03r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_DWNLD_03r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_DWNLD_04
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8234
 * DEVAD:    1
 * DESC:     Firmware download register 04
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     DOWNLOAD_04      download related registers
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r (0x00018241 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_DWNLD_11.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_s {
     uint32_t v[1];
     uint32_t gen_cntrls_dwnld_11[1];
     uint32_t _gen_cntrls_dwnld_11;
} BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_t;

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_CLR(r) (r).gen_cntrls_dwnld_11[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_SET(r,d) (r).gen_cntrls_dwnld_11[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_GET(r) (r).gen_cntrls_dwnld_11[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_DOWNLOAD_11f_GET(r) (((r).gen_cntrls_dwnld_11[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_DOWNLOAD_11f_SET(r,f) (r).gen_cntrls_dwnld_11[0]=(((r).gen_cntrls_dwnld_11[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_DWNLD_11.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_11r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r,(_r._gen_cntrls_dwnld_11))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_11r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r,(_r._gen_cntrls_dwnld_11))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_DWNLD_11r BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r
#define GEN_CNTRLS_DWNLD_11r_SIZE BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_t GEN_CNTRLS_DWNLD_11r_t;
#define GEN_CNTRLS_DWNLD_11r_CLR BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_CLR
#define GEN_CNTRLS_DWNLD_11r_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_SET
#define GEN_CNTRLS_DWNLD_11r_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_GET
#define GEN_CNTRLS_DWNLD_11r_DOWNLOAD_11f_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_DOWNLOAD_11f_GET
#define GEN_CNTRLS_DWNLD_11r_DOWNLOAD_11f_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r_DOWNLOAD_11f_SET
#define READ_GEN_CNTRLS_DWNLD_11r BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_11r
#define WRITE_GEN_CNTRLS_DWNLD_11r BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_11r
#define MODIFY_GEN_CNTRLS_DWNLD_11r BCMI_APERTA_D_MODIFY_GEN_CNTRLS_DWNLD_11r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_DWNLD_11r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_DWNLD_12
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8242
 * DEVAD:    1
 * DESC:     Firmware download register 18
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     DOWNLOAD_12      download related registers
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r (0x00018243 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_DWNLD_13.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_s {
     uint32_t v[1];
     uint32_t gen_cntrls_dwnld_13[1];
     uint32_t _gen_cntrls_dwnld_13;
} BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_t;

#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_CLR(r) (r).gen_cntrls_dwnld_13[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_SET(r,d) (r).gen_cntrls_dwnld_13[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_GET(r) (r).gen_cntrls_dwnld_13[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_DOWNLOAD_13f_GET(r) (((r).gen_cntrls_dwnld_13[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_DOWNLOAD_13f_SET(r,f) (r).gen_cntrls_dwnld_13[0]=(((r).gen_cntrls_dwnld_13[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_DWNLD_13.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_13r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r,(_r._gen_cntrls_dwnld_13))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_13r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r,(_r._gen_cntrls_dwnld_13))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_DWNLD_13r BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r
#define GEN_CNTRLS_DWNLD_13r_SIZE BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_t GEN_CNTRLS_DWNLD_13r_t;
#define GEN_CNTRLS_DWNLD_13r_CLR BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_CLR
#define GEN_CNTRLS_DWNLD_13r_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_SET
#define GEN_CNTRLS_DWNLD_13r_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_GET
#define GEN_CNTRLS_DWNLD_13r_DOWNLOAD_13f_GET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_DOWNLOAD_13f_GET
#define GEN_CNTRLS_DWNLD_13r_DOWNLOAD_13f_SET BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r_DOWNLOAD_13f_SET
#define READ_GEN_CNTRLS_DWNLD_13r BCMI_APERTA_D_READ_GEN_CNTRLS_DWNLD_13r
#define WRITE_GEN_CNTRLS_DWNLD_13r BCMI_APERTA_D_WRITE_GEN_CNTRLS_DWNLD_13r
#define MODIFY_GEN_CNTRLS_DWNLD_13r BCMI_APERTA_D_MODIFY_GEN_CNTRLS_DWNLD_13r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_DWNLD_13r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_DWNLD_14
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8244
 * DEVAD:    1
 * DESC:     Firmware download register 20
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     DOWNLOAD_14      download related registers
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr (0x00018221 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_MST_MSGOUT.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_s {
     uint32_t v[1];
     uint32_t gen_cntrls_mst_msgout[1];
     uint32_t _gen_cntrls_mst_msgout;
} BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t;

#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_CLR(r) (r).gen_cntrls_mst_msgout[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_SET(r,d) (r).gen_cntrls_mst_msgout[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_GET(r) (r).gen_cntrls_mst_msgout[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET(r) (((r).gen_cntrls_mst_msgout[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_SET(r,f) (r).gen_cntrls_mst_msgout[0]=(((r).gen_cntrls_mst_msgout[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_MST_MSGOUT.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr,(_r._gen_cntrls_mst_msgout))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGOUTr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr,(_r._gen_cntrls_mst_msgout))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_MST_MSGOUTr BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr
#define GEN_CNTRLS_MST_MSGOUTr_SIZE BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t GEN_CNTRLS_MST_MSGOUTr_t;
#define GEN_CNTRLS_MST_MSGOUTr_CLR BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_CLR
#define GEN_CNTRLS_MST_MSGOUTr_SET BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_SET
#define GEN_CNTRLS_MST_MSGOUTr_GET BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_GET
#define GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET
#define GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_SET BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_SET
#define READ_GEN_CNTRLS_MST_MSGOUTr BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr
#define WRITE_GEN_CNTRLS_MST_MSGOUTr BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGOUTr
#define MODIFY_GEN_CNTRLS_MST_MSGOUTr BCMI_APERTA_D_MODIFY_GEN_CNTRLS_MST_MSGOUTr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_MST_MSGIN
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8222
 * DEVAD:    1
 * DESC:     Incoming message from external world to Master micro
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MST_MSGIN_VAL    message from external world to Master micro.
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr (0x00018204 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_MDIO_PHYAD_CTRL.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_s {
     uint32_t v[1];
     uint32_t gen_cntrls_mdio_phyad_ctrl[1];
     uint32_t _gen_cntrls_mdio_phyad_ctrl;
} BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t;

#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_CLR(r) (r).gen_cntrls_mdio_phyad_ctrl[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_SET(r,d) (r).gen_cntrls_mdio_phyad_ctrl[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_GET(r) (r).gen_cntrls_mdio_phyad_ctrl[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MULTI_PORT_PHYADf_GET(r) ((((r).gen_cntrls_mdio_phyad_ctrl[0]) >> 2) & 0x1f)
#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MULTI_PORT_PHYADf_SET(r,f) (r).gen_cntrls_mdio_phyad_ctrl[0]=(((r).gen_cntrls_mdio_phyad_ctrl[0] & ~((uint32_t)0x1f << 2)) | ((((uint32_t)f) & 0x1f) << 2)) | (31 << (16 + 2))
#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_MULTI_ENf_GET(r) ((((r).gen_cntrls_mdio_phyad_ctrl[0]) >> 1) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_MULTI_ENf_SET(r,f) (r).gen_cntrls_mdio_phyad_ctrl[0]=(((r).gen_cntrls_mdio_phyad_ctrl[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_GET(r) (((r).gen_cntrls_mdio_phyad_ctrl[0]) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(r,f) (r).gen_cntrls_mdio_phyad_ctrl[0]=(((r).gen_cntrls_mdio_phyad_ctrl[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access GEN_CNTRLS_MDIO_PHYAD_CTRL.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr,(_r._gen_cntrls_mdio_phyad_ctrl))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr,(_r._gen_cntrls_mdio_phyad_ctrl))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_MDIO_PHYAD_CTRLr BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_SIZE BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t GEN_CNTRLS_MDIO_PHYAD_CTRLr_t;
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_CLR BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_CLR
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_SET BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_SET
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_GET BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_GET
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_MULTI_PORT_PHYADf_GET BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MULTI_PORT_PHYADf_GET
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_MULTI_PORT_PHYADf_SET BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MULTI_PORT_PHYADf_SET
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_MULTI_ENf_GET BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_MULTI_ENf_GET
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_MULTI_ENf_SET BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_MULTI_ENf_SET
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_GET BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_GET
#define GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET
#define READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr BCMI_APERTA_D_READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr
#define WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr BCMI_APERTA_D_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr
#define MODIFY_GEN_CNTRLS_MDIO_PHYAD_CTRLr BCMI_APERTA_D_MODIFY_GEN_CNTRLS_MDIO_PHYAD_CTRLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_MCTRL_RAM_MDIO_CTRL
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8205
 * DEVAD:    1
 * DESC:     Module Controller NVRAM select bits for mdio side access
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MD_EXTMOD_SELECT NVRAM select bits for access from MDIO side  1'b0: Select to read NVRAM for module 0 from MDIO side  1'b1: Select to read NVRAM for module 1 from MDIO side
 *
 ******************************************************************************/

#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr (0x00018213 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_BOOT.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_BOOTr_s {
     uint32_t v[1];
     uint32_t gen_cntrls_boot[1];
     uint32_t _gen_cntrls_boot;
} BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t;

#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr_CLR(r) (r).gen_cntrls_boot[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SET(r,d) (r).gen_cntrls_boot[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr_GET(r) (r).gen_cntrls_boot[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_GET(r) ((((r).gen_cntrls_boot[0]) >> 2) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_SET(r,f) (r).gen_cntrls_boot[0]=(((r).gen_cntrls_boot[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(r) ((((r).gen_cntrls_boot[0]) >> 1) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_SET(r,f) (r).gen_cntrls_boot[0]=(((r).gen_cntrls_boot[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))

/*
 * These macros can be used to access GEN_CNTRLS_BOOT.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_BOOTr,(_r._gen_cntrls_boot))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_BOOTr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_BOOTr,(_r._gen_cntrls_boot))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_BOOTr BCMI_APERTA_D_GEN_CNTRLS_BOOTr
#define GEN_CNTRLS_BOOTr_SIZE BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t GEN_CNTRLS_BOOTr_t;
#define GEN_CNTRLS_BOOTr_CLR BCMI_APERTA_D_GEN_CNTRLS_BOOTr_CLR
#define GEN_CNTRLS_BOOTr_SET BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SET
#define GEN_CNTRLS_BOOTr_GET BCMI_APERTA_D_GEN_CNTRLS_BOOTr_GET
#define GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_GET BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_GET
#define GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_SET BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_DONE_ONCEf_SET
#define GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET
#define GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_SET BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_SET
#define READ_GEN_CNTRLS_BOOTr BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr
#define WRITE_GEN_CNTRLS_BOOTr BCMI_APERTA_D_WRITE_GEN_CNTRLS_BOOTr
#define MODIFY_GEN_CNTRLS_BOOTr BCMI_APERTA_D_MODIFY_GEN_CNTRLS_BOOTr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_BOOTr'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_MST_AHB_ERROR
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8214
 * DEVAD:    1
 * DESC:     latch bits for illegal hsize accesses to arbiter
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MST_AHB_ARB_8B_LH This latch is set when Master M0 tries to make an 8-bit access to arbiter register.The latch is cleared upon read.
 *     MST_AHB_ARB_32B_LH This latch is set when Master M0 tries to make an 32-bit access to arbiter register.The latch is cleared upon read.
 *     AHB_SPIM_32B_LH  This latch is set when M0 tries to make an 32-bit access to SPI master register.The latch is cleared upon read.
 *     AHB_SPIM_16B_LH  This latch is set when M0 tries to make an 16-bit access to SPI master register.The latch is cleared upon read.
 *     MST_AHB_DEF_LH   This latch is set when master M0 tries to make an access to default peripheral.The latch is cleared upon read.
 *
 ******************************************************************************/

#define BCMI_APERTA_D_CTRL_RESET_CTRLr (0x00018b18 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_RESET_CTRLr_SIZE 4

/*
 * This structure should be used to declare and program CTRL_RESET_CTRL.
 *
 */
typedef union BCMI_APERTA_D_CTRL_RESET_CTRLr_s {
     uint32_t v[1];
     uint32_t ctrl_reset_ctrl[1];
     uint32_t _ctrl_reset_ctrl;
} BCMI_APERTA_D_CTRL_RESET_CTRLr_t;

#define BCMI_APERTA_D_CTRL_RESET_CTRLr_CLR(r) (r).ctrl_reset_ctrl[0] = 0
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_SET(r,d) (r).ctrl_reset_ctrl[0] = d
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_GET(r) (r).ctrl_reset_ctrl[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_GCT_MST_TS_RSTBf_GET(r) ((((r).ctrl_reset_ctrl[0]) >> 7) & 0x1)
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_GCT_MST_TS_RSTBf_SET(r,f) (r).ctrl_reset_ctrl[0]=(((r).ctrl_reset_ctrl[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_ING_MS_RSTBf_GET(r) ((((r).ctrl_reset_ctrl[0]) >> 6) & 0x1)
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_ING_MS_RSTBf_SET(r,f) (r).ctrl_reset_ctrl[0]=(((r).ctrl_reset_ctrl[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_EGR_MS_RSTBf_GET(r) ((((r).ctrl_reset_ctrl[0]) >> 5) & 0x1)
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_EGR_MS_RSTBf_SET(r,f) (r).ctrl_reset_ctrl[0]=(((r).ctrl_reset_ctrl[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_SPM_CORE_RSTBf_GET(r) ((((r).ctrl_reset_ctrl[0]) >> 3) & 0x1)
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_SPM_CORE_RSTBf_SET(r,f) (r).ctrl_reset_ctrl[0]=(((r).ctrl_reset_ctrl[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_LPM_CORE_RSTBf_GET(r) ((((r).ctrl_reset_ctrl[0]) >> 1) & 0x1)
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_LPM_CORE_RSTBf_SET(r,f) (r).ctrl_reset_ctrl[0]=(((r).ctrl_reset_ctrl[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_DP_RSTBf_GET(r) (((r).ctrl_reset_ctrl[0]) & 0x1)
#define BCMI_APERTA_D_CTRL_RESET_CTRLr_DP_RSTBf_SET(r,f) (r).ctrl_reset_ctrl[0]=(((r).ctrl_reset_ctrl[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access CTRL_RESET_CTRL.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_RESET_CTRLr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_RESET_CTRLr,(_r._ctrl_reset_ctrl))
#define BCMI_APERTA_D_WRITE_CTRL_RESET_CTRLr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_RESET_CTRLr,(_r._ctrl_reset_ctrl))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_RESET_CTRLr BCMI_APERTA_D_CTRL_RESET_CTRLr
#define CTRL_RESET_CTRLr_SIZE BCMI_APERTA_D_CTRL_RESET_CTRLr_SIZE
typedef BCMI_APERTA_D_CTRL_RESET_CTRLr_t CTRL_RESET_CTRLr_t;
#define CTRL_RESET_CTRLr_CLR BCMI_APERTA_D_CTRL_RESET_CTRLr_CLR
#define CTRL_RESET_CTRLr_SET BCMI_APERTA_D_CTRL_RESET_CTRLr_SET
#define CTRL_RESET_CTRLr_GET BCMI_APERTA_D_CTRL_RESET_CTRLr_GET
#define CTRL_RESET_CTRLr_GCT_MST_TS_RSTBf_GET BCMI_APERTA_D_CTRL_RESET_CTRLr_GCT_MST_TS_RSTBf_GET
#define CTRL_RESET_CTRLr_GCT_MST_TS_RSTBf_SET BCMI_APERTA_D_CTRL_RESET_CTRLr_GCT_MST_TS_RSTBf_SET
#define CTRL_RESET_CTRLr_ING_MS_RSTBf_GET BCMI_APERTA_D_CTRL_RESET_CTRLr_ING_MS_RSTBf_GET
#define CTRL_RESET_CTRLr_ING_MS_RSTBf_SET BCMI_APERTA_D_CTRL_RESET_CTRLr_ING_MS_RSTBf_SET
#define CTRL_RESET_CTRLr_EGR_MS_RSTBf_GET BCMI_APERTA_D_CTRL_RESET_CTRLr_EGR_MS_RSTBf_GET
#define CTRL_RESET_CTRLr_EGR_MS_RSTBf_SET BCMI_APERTA_D_CTRL_RESET_CTRLr_EGR_MS_RSTBf_SET
#define CTRL_RESET_CTRLr_SPM_CORE_RSTBf_GET BCMI_APERTA_D_CTRL_RESET_CTRLr_SPM_CORE_RSTBf_GET
#define CTRL_RESET_CTRLr_SPM_CORE_RSTBf_SET BCMI_APERTA_D_CTRL_RESET_CTRLr_SPM_CORE_RSTBf_SET
#define CTRL_RESET_CTRLr_LPM_CORE_RSTBf_GET BCMI_APERTA_D_CTRL_RESET_CTRLr_LPM_CORE_RSTBf_GET
#define CTRL_RESET_CTRLr_LPM_CORE_RSTBf_SET BCMI_APERTA_D_CTRL_RESET_CTRLr_LPM_CORE_RSTBf_SET
#define CTRL_RESET_CTRLr_DP_RSTBf_GET BCMI_APERTA_D_CTRL_RESET_CTRLr_DP_RSTBf_GET
#define CTRL_RESET_CTRLr_DP_RSTBf_SET BCMI_APERTA_D_CTRL_RESET_CTRLr_DP_RSTBf_SET
#define READ_CTRL_RESET_CTRLr BCMI_APERTA_D_READ_CTRL_RESET_CTRLr
#define WRITE_CTRL_RESET_CTRLr BCMI_APERTA_D_WRITE_CTRL_RESET_CTRLr
#define MODIFY_CTRL_RESET_CTRLr BCMI_APERTA_D_MODIFY_CTRL_RESET_CTRLr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_RESET_CTRLr'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_RESET_CTRL_P0
 * BLOCKS:   CTRL
 * REGADDR:  0x8b19
 * DEVAD:    1
 * DESC:     Port0 reset control register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     EGR_LPM_IF_RSTB_P0 Active low reset for line side PM egress interface block
 *     EGR_SF_RSTB_P0   Active low reset for egress store and forward block
 *     EGR_PTP_RSTB_P0  Active low reset for egress PTP block
 *     EGR_FC_RSTB_P0   Active low reset for egress flow control block
 *     EGR_SPM_IF_RSTB_P0 Active low reset for system side PM egress interface block
 *     EGR_MS_RC_RSTB_P0 Active low reset for egress macsec RC block
 *     ING_SPM_IF_RSTB_P0 Active low reset for system side PM ingress interface block
 *     ING_FC_RSTB_P0   Active low reset for ingress flow control block
 *     ING_PTP_RSTB_P0  Active low reset for ingress PTP block
 *     ING_LPM_IF_RSTB_P0 Active low reset for line side PM ingress interface block
 *
 ******************************************************************************/

#define BCMI_APERTA_D_CTRL_SWGPREG0Er (0x00018b3e | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_SWGPREG0Er_SIZE 4

/*
 * This structure should be used to declare and program CTRL_SWGPREG0E.
 *
 */
typedef union BCMI_APERTA_D_CTRL_SWGPREG0Er_s {
     uint32_t v[1];
     uint32_t ctrl_swgpreg0e[1];
     uint32_t _ctrl_swgpreg0e;
} BCMI_APERTA_D_CTRL_SWGPREG0Er_t;

#define BCMI_APERTA_D_CTRL_SWGPREG0Er_CLR(r) (r).ctrl_swgpreg0e[0] = 0
#define BCMI_APERTA_D_CTRL_SWGPREG0Er_SET(r,d) (r).ctrl_swgpreg0e[0] = d
#define BCMI_APERTA_D_CTRL_SWGPREG0Er_GET(r) (r).ctrl_swgpreg0e[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_SWGPREG0Er_SWGPREG0E_DATAf_GET(r) (((r).ctrl_swgpreg0e[0]) & 0xffff)
#define BCMI_APERTA_D_CTRL_SWGPREG0Er_SWGPREG0E_DATAf_SET(r,f) (r).ctrl_swgpreg0e[0]=(((r).ctrl_swgpreg0e[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access CTRL_SWGPREG0E.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_SWGPREG0Er(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_SWGPREG0Er,(_r._ctrl_swgpreg0e))
#define BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Er(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_SWGPREG0Er,(_r._ctrl_swgpreg0e))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_SWGPREG0Er BCMI_APERTA_D_CTRL_SWGPREG0Er
#define CTRL_SWGPREG0Er_SIZE BCMI_APERTA_D_CTRL_SWGPREG0Er_SIZE
typedef BCMI_APERTA_D_CTRL_SWGPREG0Er_t CTRL_SWGPREG0Er_t;
#define CTRL_SWGPREG0Er_CLR BCMI_APERTA_D_CTRL_SWGPREG0Er_CLR
#define CTRL_SWGPREG0Er_SET BCMI_APERTA_D_CTRL_SWGPREG0Er_SET
#define CTRL_SWGPREG0Er_GET BCMI_APERTA_D_CTRL_SWGPREG0Er_GET
#define CTRL_SWGPREG0Er_SWGPREG0E_DATAf_GET BCMI_APERTA_D_CTRL_SWGPREG0Er_SWGPREG0E_DATAf_GET
#define CTRL_SWGPREG0Er_SWGPREG0E_DATAf_SET BCMI_APERTA_D_CTRL_SWGPREG0Er_SWGPREG0E_DATAf_SET
#define READ_CTRL_SWGPREG0Er BCMI_APERTA_D_READ_CTRL_SWGPREG0Er
#define WRITE_CTRL_SWGPREG0Er BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Er
#define MODIFY_CTRL_SWGPREG0Er BCMI_APERTA_D_MODIFY_CTRL_SWGPREG0Er

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_SWGPREG0Er'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_SWGPREG0F
 * BLOCKS:   CTRL
 * REGADDR:  0x8b3f
 * DEVAD:    1
 * DESC:     Software General Puropose Register 0F
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SWGPREG0F_DATA   Data
 *
 ******************************************************************************/

#define BCMI_APERTA_D_CTRL_SWGPREG15r (0x00018b45 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_SWGPREG15r_SIZE 4

/*
 * This structure should be used to declare and program CTRL_SWGPREG15.
 *
 */
typedef union BCMI_APERTA_D_CTRL_SWGPREG15r_s {
     uint32_t v[1];
     uint32_t ctrl_swgpreg15[1];
     uint32_t _ctrl_swgpreg15;
} BCMI_APERTA_D_CTRL_SWGPREG15r_t;

#define BCMI_APERTA_D_CTRL_SWGPREG15r_CLR(r) (r).ctrl_swgpreg15[0] = 0
#define BCMI_APERTA_D_CTRL_SWGPREG15r_SET(r,d) (r).ctrl_swgpreg15[0] = d
#define BCMI_APERTA_D_CTRL_SWGPREG15r_GET(r) (r).ctrl_swgpreg15[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_SWGPREG15r_SWGPREG15_DATAf_GET(r) (((r).ctrl_swgpreg15[0]) & 0xffff)
#define BCMI_APERTA_D_CTRL_SWGPREG15r_SWGPREG15_DATAf_SET(r,f) (r).ctrl_swgpreg15[0]=(((r).ctrl_swgpreg15[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access CTRL_SWGPREG15.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_SWGPREG15r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_SWGPREG15r,(_r._ctrl_swgpreg15))
#define BCMI_APERTA_D_WRITE_CTRL_SWGPREG15r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_SWGPREG15r,(_r._ctrl_swgpreg15))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_SWGPREG15r BCMI_APERTA_D_CTRL_SWGPREG15r
#define CTRL_SWGPREG15r_SIZE BCMI_APERTA_D_CTRL_SWGPREG15r_SIZE
typedef BCMI_APERTA_D_CTRL_SWGPREG15r_t CTRL_SWGPREG15r_t;
#define CTRL_SWGPREG15r_CLR BCMI_APERTA_D_CTRL_SWGPREG15r_CLR
#define CTRL_SWGPREG15r_SET BCMI_APERTA_D_CTRL_SWGPREG15r_SET
#define CTRL_SWGPREG15r_GET BCMI_APERTA_D_CTRL_SWGPREG15r_GET
#define CTRL_SWGPREG15r_SWGPREG15_DATAf_GET BCMI_APERTA_D_CTRL_SWGPREG15r_SWGPREG15_DATAf_GET
#define CTRL_SWGPREG15r_SWGPREG15_DATAf_SET BCMI_APERTA_D_CTRL_SWGPREG15r_SWGPREG15_DATAf_SET
#define READ_CTRL_SWGPREG15r BCMI_APERTA_D_READ_CTRL_SWGPREG15r
#define WRITE_CTRL_SWGPREG15r BCMI_APERTA_D_WRITE_CTRL_SWGPREG15r
#define MODIFY_CTRL_SWGPREG15r BCMI_APERTA_D_MODIFY_CTRL_SWGPREG15r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_SWGPREG15r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_SWGPREG16
 * BLOCKS:   CTRL
 * REGADDR:  0x8b46
 * DEVAD:    1
 * DESC:     Software General Puropose Register 16
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SWGPREG16_DATA   Data
 *
 ******************************************************************************/

#define BCMI_APERTA_D_CTRL_SWGPREG16r (0x00018b46 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_SWGPREG16r_SIZE 4

/*
 * This structure should be used to declare and program CTRL_SWGPREG16.
 *
 */
typedef union BCMI_APERTA_D_CTRL_SWGPREG16r_s {
     uint32_t v[1];
     uint32_t ctrl_swgpreg16[1];
     uint32_t _ctrl_swgpreg16;
} BCMI_APERTA_D_CTRL_SWGPREG16r_t;

#define BCMI_APERTA_D_CTRL_SWGPREG16r_CLR(r) (r).ctrl_swgpreg16[0] = 0
#define BCMI_APERTA_D_CTRL_SWGPREG16r_SET(r,d) (r).ctrl_swgpreg16[0] = d
#define BCMI_APERTA_D_CTRL_SWGPREG16r_GET(r) (r).ctrl_swgpreg16[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_SWGPREG16r_SWGPREG16_DATAf_GET(r) (((r).ctrl_swgpreg16[0]) & 0xffff)
#define BCMI_APERTA_D_CTRL_SWGPREG16r_SWGPREG16_DATAf_SET(r,f) (r).ctrl_swgpreg16[0]=(((r).ctrl_swgpreg16[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access CTRL_SWGPREG16.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_SWGPREG16r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_SWGPREG16r,(_r._ctrl_swgpreg16))
#define BCMI_APERTA_D_WRITE_CTRL_SWGPREG16r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_SWGPREG16r,(_r._ctrl_swgpreg16))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_SWGPREG16r BCMI_APERTA_D_CTRL_SWGPREG16r
#define CTRL_SWGPREG16r_SIZE BCMI_APERTA_D_CTRL_SWGPREG16r_SIZE
typedef BCMI_APERTA_D_CTRL_SWGPREG16r_t CTRL_SWGPREG16r_t;
#define CTRL_SWGPREG16r_CLR BCMI_APERTA_D_CTRL_SWGPREG16r_CLR
#define CTRL_SWGPREG16r_SET BCMI_APERTA_D_CTRL_SWGPREG16r_SET
#define CTRL_SWGPREG16r_GET BCMI_APERTA_D_CTRL_SWGPREG16r_GET
#define CTRL_SWGPREG16r_SWGPREG16_DATAf_GET BCMI_APERTA_D_CTRL_SWGPREG16r_SWGPREG16_DATAf_GET
#define CTRL_SWGPREG16r_SWGPREG16_DATAf_SET BCMI_APERTA_D_CTRL_SWGPREG16r_SWGPREG16_DATAf_SET
#define READ_CTRL_SWGPREG16r BCMI_APERTA_D_READ_CTRL_SWGPREG16r
#define WRITE_CTRL_SWGPREG16r BCMI_APERTA_D_WRITE_CTRL_SWGPREG16r
#define MODIFY_CTRL_SWGPREG16r BCMI_APERTA_D_MODIFY_CTRL_SWGPREG16r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_SWGPREG16r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_SWGPREG17
 * BLOCKS:   CTRL
 * REGADDR:  0x8b47
 * DEVAD:    1
 * DESC:     Software General Puropose Register 17
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SWGPREG17_DATA   Data
 *
 ******************************************************************************/

#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr (0x00018b21 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_SIZE 4

/*
 * This structure should be used to declare and program CTRL_MISC_CONTROL_TYPE.
 *
 */
typedef union BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_s {
     uint32_t v[1];
     uint32_t ctrl_misc_control_type[1];
     uint32_t _ctrl_misc_control_type;
} BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_t;

#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_CLR(r) (r).ctrl_misc_control_type[0] = 0
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_SET(r,d) (r).ctrl_misc_control_type[0] = d
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_GET(r) (r).ctrl_misc_control_type[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_PLL_COM_CK_CTRLf_GET(r) ((((r).ctrl_misc_control_type[0]) >> 8) & 0x1)
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_PLL_COM_CK_CTRLf_SET(r,f) (r).ctrl_misc_control_type[0]=(((r).ctrl_misc_control_type[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_GET(r) ((((r).ctrl_misc_control_type[0]) >> 7) & 0x1)
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET(r,f) (r).ctrl_misc_control_type[0]=(((r).ctrl_misc_control_type[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_GET(r) ((((r).ctrl_misc_control_type[0]) >> 6) & 0x1)
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET(r,f) (r).ctrl_misc_control_type[0]=(((r).ctrl_misc_control_type[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCf_GET(r) ((((r).ctrl_misc_control_type[0]) >> 5) & 0x1)
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCf_SET(r,f) (r).ctrl_misc_control_type[0]=(((r).ctrl_misc_control_type[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCVALf_GET(r) ((((r).ctrl_misc_control_type[0]) >> 4) & 0x1)
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCVALf_SET(r,f) (r).ctrl_misc_control_type[0]=(((r).ctrl_misc_control_type[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_ACK_TIMER_CTRLf_GET(r) (((r).ctrl_misc_control_type[0]) & 0xf)
#define BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_ACK_TIMER_CTRLf_SET(r,f) (r).ctrl_misc_control_type[0]=(((r).ctrl_misc_control_type[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access CTRL_MISC_CONTROL_TYPE.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_MISC_CONTROL_TYPEr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr,(_r._ctrl_misc_control_type))
#define BCMI_APERTA_D_WRITE_CTRL_MISC_CONTROL_TYPEr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr,(_r._ctrl_misc_control_type))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_MISC_CONTROL_TYPEr BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr
#define CTRL_MISC_CONTROL_TYPEr_SIZE BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_SIZE
typedef BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_t CTRL_MISC_CONTROL_TYPEr_t;
#define CTRL_MISC_CONTROL_TYPEr_CLR BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_CLR
#define CTRL_MISC_CONTROL_TYPEr_SET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_SET
#define CTRL_MISC_CONTROL_TYPEr_GET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_GET
#define CTRL_MISC_CONTROL_TYPEr_PLL_COM_CK_CTRLf_GET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_PLL_COM_CK_CTRLf_GET
#define CTRL_MISC_CONTROL_TYPEr_PLL_COM_CK_CTRLf_SET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_PLL_COM_CK_CTRLf_SET
#define CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_GET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_GET
#define CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET
#define CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_GET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_GET
#define CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET
#define CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCf_GET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCf_GET
#define CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCf_SET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCf_SET
#define CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCVALf_GET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCVALf_GET
#define CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCVALf_SET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_IO_OD_EN_FRCVALf_SET
#define CTRL_MISC_CONTROL_TYPEr_ACK_TIMER_CTRLf_GET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_ACK_TIMER_CTRLf_GET
#define CTRL_MISC_CONTROL_TYPEr_ACK_TIMER_CTRLf_SET BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_ACK_TIMER_CTRLf_SET
#define READ_CTRL_MISC_CONTROL_TYPEr BCMI_APERTA_D_READ_CTRL_MISC_CONTROL_TYPEr
#define WRITE_CTRL_MISC_CONTROL_TYPEr BCMI_APERTA_D_WRITE_CTRL_MISC_CONTROL_TYPEr
#define MODIFY_CTRL_MISC_CONTROL_TYPEr BCMI_APERTA_D_MODIFY_CTRL_MISC_CONTROL_TYPEr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_CHIP_ACK_TIMER
 * BLOCKS:   CTRL
 * REGADDR:  0x8b22
 * DEVAD:    1
 * DESC:     chip reg ack timer for delayed ack to arbiter for chip regsiters
 * RESETVAL: 0x80 (128)
 * ACCESS:   R/W
 * FIELDS:
 *     CHIP_REG_ACK_TIMER_DIS chip ack timer disableWhen set to 1, the chip register timer does not startThe old behavior of holding the ack high immediately will occur.
 *     CHIP_REG_ACK_TIMER_VALUE chip ack timer valueThis timer is used to delay the ack from address decoder to the arbiter.The rising edge of any of the 3 reg_sel bits from the arbiter will start a counterthat will create an ack to the arbiter when it reaches the chip_reg_ack_timer_value.The idea is to wait a certain number of cycles after one of the reg_sel bits assertsinstead of immediately sending an ack when any of the chip regsiter addresses is selected.
 *
 ******************************************************************************/

#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr (0x000182ff | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SIZE 4

/*
 * This structure should be used to declare and program MICRO_BOOT_BOOT_POR.
 *
 */
typedef union BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_s {
     uint32_t v[1];
     uint32_t micro_boot_boot_por[1];
     uint32_t _micro_boot_boot_por;
} BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t;

#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_CLR(r) (r).micro_boot_boot_por[0] = 0
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SET(r,d) (r).micro_boot_boot_por[0] = d
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_GET(r) (r).micro_boot_boot_por[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_GET(r) ((((r).micro_boot_boot_por[0]) >> 15) & 0x1)
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_SET(r,f) (r).micro_boot_boot_por[0]=(((r).micro_boot_boot_por[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_GET(r) ((((r).micro_boot_boot_por[0]) >> 14) & 0x1)
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(r,f) (r).micro_boot_boot_por[0]=(((r).micro_boot_boot_por[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14)) | (1 << (16 + 14))
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_LARGE_MEMf_GET(r) ((((r).micro_boot_boot_por[0]) >> 13) & 0x1)
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_LARGE_MEMf_SET(r,f) (r).micro_boot_boot_por[0]=(((r).micro_boot_boot_por[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_GET(r) ((((r).micro_boot_boot_por[0]) >> 12) & 0x1)
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET(r,f) (r).micro_boot_boot_por[0]=(((r).micro_boot_boot_por[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MDIO_BRDCST_ENf_GET(r) ((((r).micro_boot_boot_por[0]) >> 8) & 0x1)
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MDIO_BRDCST_ENf_SET(r,f) (r).micro_boot_boot_por[0]=(((r).micro_boot_boot_por[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8)) | (1 << (16 + 8))
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_GET(r) ((((r).micro_boot_boot_por[0]) >> 6) & 0x3)
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET(r,f) (r).micro_boot_boot_por[0]=(((r).micro_boot_boot_por[0] & ~((uint32_t)0x3 << 6)) | ((((uint32_t)f) & 0x3) << 6)) | (3 << (16 + 6))
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_GET(r) (((r).micro_boot_boot_por[0]) & 0x3)
#define BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_SET(r,f) (r).micro_boot_boot_por[0]=(((r).micro_boot_boot_por[0] & ~((uint32_t)0x3)) | (((uint32_t)f) & 0x3)) | (0x3 << 16)

/*
 * These macros can be used to access MICRO_BOOT_BOOT_POR.
 *
 */
#define BCMI_APERTA_D_READ_MICRO_BOOT_BOOT_PORr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr,(_r._micro_boot_boot_por))
#define BCMI_APERTA_D_WRITE_MICRO_BOOT_BOOT_PORr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr,(_r._micro_boot_boot_por))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define MICRO_BOOT_BOOT_PORr BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr
#define MICRO_BOOT_BOOT_PORr_SIZE BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SIZE
typedef BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t MICRO_BOOT_BOOT_PORr_t;
#define MICRO_BOOT_BOOT_PORr_CLR BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_CLR
#define MICRO_BOOT_BOOT_PORr_SET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SET
#define MICRO_BOOT_BOOT_PORr_GET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_GET
#define MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_GET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_GET
#define MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_SET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_SET
#define MICRO_BOOT_BOOT_PORr_SERBOOTf_GET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_GET
#define MICRO_BOOT_BOOT_PORr_SERBOOTf_SET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET
#define MICRO_BOOT_BOOT_PORr_LARGE_MEMf_GET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_LARGE_MEMf_GET
#define MICRO_BOOT_BOOT_PORr_LARGE_MEMf_SET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_LARGE_MEMf_SET
#define MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_GET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_GET
#define MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET
#define MICRO_BOOT_BOOT_PORr_MDIO_BRDCST_ENf_GET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MDIO_BRDCST_ENf_GET
#define MICRO_BOOT_BOOT_PORr_MDIO_BRDCST_ENf_SET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MDIO_BRDCST_ENf_SET
#define MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_GET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_GET
#define MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET
#define MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_GET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_GET
#define MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_SET BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_RST_ENf_SET
#define READ_MICRO_BOOT_BOOT_PORr BCMI_APERTA_D_READ_MICRO_BOOT_BOOT_PORr
#define WRITE_MICRO_BOOT_BOOT_PORr BCMI_APERTA_D_WRITE_MICRO_BOOT_BOOT_PORr
#define MODIFY_MICRO_BOOT_BOOT_PORr BCMI_APERTA_D_MODIFY_MICRO_BOOT_BOOT_PORr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  M0ACCESS_ADDR_MST_CRAM_MEM_ADD_CTRL
 * BLOCKS:   M0ACCESS_ADDR
 * REGADDR:  0x8400
 * DEVAD:    1
 * DESC:     Address for backdoor access to master code ram
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MST_CRAM_MEM_ADD Byte address for back-door access to the master code ram. For code ram access, initialize this with an address that is a multiple of 4 (0,4,8,....). The correct sequence for back door write to code ram is to initalize the address, write into the lsb data register and then write into msb data register. The address register will automatically increment after the msb write by 4.The correct sequence for back door read to code ram is to initalize the address, read from the lsb data register and then read from msb data register. The address register will automatically increment after the lsb read by 4.Note: It is illegal to program the address register to an out-of-bounds location or with a value that is a non-multiple of 4. No hardware checks are present for the illegal scenarios.
 *
 ******************************************************************************/

#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr (0x00018a42 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SIZE 4

/*
 * This structure should be used to declare and program PAD_CNTRL_SERBOOT_STATUS.
 *
 */
typedef union BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_s {
     uint32_t v[1];
     uint32_t pad_cntrl_serboot_status[1];
     uint32_t _pad_cntrl_serboot_status;
} BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_t;

#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_CLR(r) (r).pad_cntrl_serboot_status[0] = 0
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SET(r,d) (r).pad_cntrl_serboot_status[0] = d
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_GET(r) (r).pad_cntrl_serboot_status[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAWf_GET(r) ((((r).pad_cntrl_serboot_status[0]) >> 5) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAWf_SET(r,f) (r).pad_cntrl_serboot_status[0]=(((r).pad_cntrl_serboot_status[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LLf_GET(r) ((((r).pad_cntrl_serboot_status[0]) >> 4) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LLf_SET(r,f) (r).pad_cntrl_serboot_status[0]=(((r).pad_cntrl_serboot_status[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4)) | (1 << (16 + 4))
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LHf_GET(r) ((((r).pad_cntrl_serboot_status[0]) >> 3) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LHf_SET(r,f) (r).pad_cntrl_serboot_status[0]=(((r).pad_cntrl_serboot_status[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNCf_GET(r) ((((r).pad_cntrl_serboot_status[0]) >> 2) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNCf_SET(r,f) (r).pad_cntrl_serboot_status[0]=(((r).pad_cntrl_serboot_status[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LLf_GET(r) ((((r).pad_cntrl_serboot_status[0]) >> 1) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LLf_SET(r,f) (r).pad_cntrl_serboot_status[0]=(((r).pad_cntrl_serboot_status[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LHf_GET(r) (((r).pad_cntrl_serboot_status[0]) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LHf_SET(r,f) (r).pad_cntrl_serboot_status[0]=(((r).pad_cntrl_serboot_status[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access PAD_CNTRL_SERBOOT_STATUS.
 *
 */
#define BCMI_APERTA_D_READ_PAD_CNTRL_SERBOOT_STATUSr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr,(_r._pad_cntrl_serboot_status))
#define BCMI_APERTA_D_WRITE_PAD_CNTRL_SERBOOT_STATUSr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr,(_r._pad_cntrl_serboot_status))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define PAD_CNTRL_SERBOOT_STATUSr BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr
#define PAD_CNTRL_SERBOOT_STATUSr_SIZE BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SIZE
typedef BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_t PAD_CNTRL_SERBOOT_STATUSr_t;
#define PAD_CNTRL_SERBOOT_STATUSr_CLR BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_CLR
#define PAD_CNTRL_SERBOOT_STATUSr_SET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SET
#define PAD_CNTRL_SERBOOT_STATUSr_GET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_GET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAWf_GET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAWf_GET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAWf_SET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAWf_SET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LLf_GET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LLf_GET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LLf_SET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LLf_SET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LHf_GET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LHf_GET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LHf_SET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAW_LHf_SET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNCf_GET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNCf_GET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNCf_SET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNCf_SET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LLf_GET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LLf_GET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LLf_SET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LLf_SET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LHf_GET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LHf_GET
#define PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LHf_SET BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_SYNC_LHf_SET
#define READ_PAD_CNTRL_SERBOOT_STATUSr BCMI_APERTA_D_READ_PAD_CNTRL_SERBOOT_STATUSr
#define WRITE_PAD_CNTRL_SERBOOT_STATUSr BCMI_APERTA_D_WRITE_PAD_CNTRL_SERBOOT_STATUSr
#define MODIFY_PAD_CNTRL_SERBOOT_STATUSr BCMI_APERTA_D_MODIFY_PAD_CNTRL_SERBOOT_STATUSr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  PAD_CNTRL_EXT_UC_RSTB_IN_CONTROL_0
 * BLOCKS:   PAD_CNTRL
 * REGADDR:  0x8a44
 * DEVAD:    1
 * DESC:     PAD ext_uc_rstb_in control 0 register
 * RESETVAL: 0x2 (2)
 * ACCESS:   R/W
 * FIELDS:
 *     EXT_UC_RSTB_IN_PUP ext_uc_rstb_in pull-up
 *     EXT_UC_RSTB_IN_PDN ext_uc_rstb_in pull down
 *     EXT_UC_RSTB_IN_IND ext_uc_rstb_in ind
 *     EXT_UC_RSTB_IN_DIN_SYNC_FRCVAL ext_uc_rstb_in din_sync force value
 *     EXT_UC_RSTB_IN_DIN_SYNC_FRC ext_uc_rstb_in din_sync force enable
 *     EXT_UC_RSTB_IN_DIN_RAW_FRCVAL ext_uc_rstb_in din_raw force value
 *     EXT_UC_RSTB_IN_DIN_RAW_FRC ext_uc_rstb_in din_raw force enable
 *     EXT_UC_RSTB_IN_DIN_SYNC_INVERT_EN ext_uc_rstb_in din_sync polarity inversion enable
 *     EXT_UC_RSTB_IN_DIN_RAW_INVERT_EN ext_uc_rstb_in din_raw polarity inversion enable
 *     EXT_UC_RSTB_IN_DIN_RAW_OR_SYNC_MUX ext_uc_rstb_in mux select for which version of din goes to gpio input muxes0 raw1 sync
 *     EXT_UC_RSTB_IN_DG_BYPASS ext_uc_rstb_in deglitch bypassThis disables the deglitch logic built into the pad_ctrl that uses dg_cnt
 *     EXT_UC_RSTB_IN_DG_CNT This register is only valid when pad is input, i.e. oeb is highWhen deglitch_bypass is set to 0, this register configuresthe number of consecutive common cycle for din deglitch{ext_uc_rstb_in_dg_bypass, ext_uc_rstb_in_dg_cnt}: number of stable common_ck cycle needed to deglitch{1'b0, 3'h0}     2{1'b0, 3'h1}     3{1'b0, 3'h2}     4{1'b0, 3'h3}     5{1'b0, 3'h4}     6{1'b0, 3'h5}     7{1'b0, 3'h6}     8{1'b0, 3'h7}     9
 *
 ******************************************************************************/

#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r (0x0001814c | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_SIZE 4

/*
 * This structure should be used to declare and program ANA_PLPPLL_CTRL12.
 *
 */
typedef union BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_s {
     uint32_t v[1];
     uint32_t ana_plppll_ctrl12[1];
     uint32_t _ana_plppll_ctrl12;
} BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_t;

#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CLR(r) (r).ana_plppll_ctrl12[0] = 0
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_SET(r,d) (r).ana_plppll_ctrl12[0] = d
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_GET(r) (r).ana_plppll_ctrl12[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_165_165_PD_CML_LCREFOUT2f_GET(r) ((((r).ana_plppll_ctrl12[0]) >> 15) & 0x1)
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_165_165_PD_CML_LCREFOUT2f_SET(r,f) (r).ana_plppll_ctrl12[0]=(((r).ana_plppll_ctrl12[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_164_164_PD_CML_LCREFOUTf_GET(r) ((((r).ana_plppll_ctrl12[0]) >> 14) & 0x1)
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_164_164_PD_CML_LCREFOUTf_SET(r,f) (r).ana_plppll_ctrl12[0]=(((r).ana_plppll_ctrl12[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14)) | (1 << (16 + 14))
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_163_163_PD_CML_REFCLK_CHOUTf_GET(r) ((((r).ana_plppll_ctrl12[0]) >> 13) & 0x1)
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_163_163_PD_CML_REFCLK_CHOUTf_SET(r,f) (r).ana_plppll_ctrl12[0]=(((r).ana_plppll_ctrl12[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_162_162_EN_CK10T_OUTf_GET(r) ((((r).ana_plppll_ctrl12[0]) >> 12) & 0x1)
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_162_162_EN_CK10T_OUTf_SET(r,f) (r).ana_plppll_ctrl12[0]=(((r).ana_plppll_ctrl12[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_161_161_EN_CK40T_OUTf_GET(r) ((((r).ana_plppll_ctrl12[0]) >> 11) & 0x1)
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_161_161_EN_CK40T_OUTf_SET(r,f) (r).ana_plppll_ctrl12[0]=(((r).ana_plppll_ctrl12[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_160_160_EN_CK400T_OUTf_GET(r) ((((r).ana_plppll_ctrl12[0]) >> 10) & 0x1)
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_160_160_EN_CK400T_OUTf_SET(r,f) (r).ana_plppll_ctrl12[0]=(((r).ana_plppll_ctrl12[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10)) | (1 << (16 + 10))
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_175_166_RESERVED_PCIE3f_GET(r) (((r).ana_plppll_ctrl12[0]) & 0x3ff)
#define BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_175_166_RESERVED_PCIE3f_SET(r,f) (r).ana_plppll_ctrl12[0]=(((r).ana_plppll_ctrl12[0] & ~((uint32_t)0x3ff)) | (((uint32_t)f) & 0x3ff)) | (0x3ff << 16)

/*
 * These macros can be used to access ANA_PLPPLL_CTRL12.
 *
 */
#define BCMI_APERTA_D_READ_ANA_PLPPLL_CTRL12r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_ANA_PLPPLL_CTRL12r,(_r._ana_plppll_ctrl12))
#define BCMI_APERTA_D_WRITE_ANA_PLPPLL_CTRL12r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_ANA_PLPPLL_CTRL12r,(_r._ana_plppll_ctrl12))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define ANA_PLPPLL_CTRL12r BCMI_APERTA_D_ANA_PLPPLL_CTRL12r
#define ANA_PLPPLL_CTRL12r_SIZE BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_SIZE
typedef BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_t ANA_PLPPLL_CTRL12r_t;
#define ANA_PLPPLL_CTRL12r_CLR BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CLR
#define ANA_PLPPLL_CTRL12r_SET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_SET
#define ANA_PLPPLL_CTRL12r_GET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_GET
#define ANA_PLPPLL_CTRL12r_CTRL_165_165_PD_CML_LCREFOUT2f_GET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_165_165_PD_CML_LCREFOUT2f_GET
#define ANA_PLPPLL_CTRL12r_CTRL_165_165_PD_CML_LCREFOUT2f_SET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_165_165_PD_CML_LCREFOUT2f_SET
#define ANA_PLPPLL_CTRL12r_CTRL_164_164_PD_CML_LCREFOUTf_GET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_164_164_PD_CML_LCREFOUTf_GET
#define ANA_PLPPLL_CTRL12r_CTRL_164_164_PD_CML_LCREFOUTf_SET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_164_164_PD_CML_LCREFOUTf_SET
#define ANA_PLPPLL_CTRL12r_CTRL_163_163_PD_CML_REFCLK_CHOUTf_GET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_163_163_PD_CML_REFCLK_CHOUTf_GET
#define ANA_PLPPLL_CTRL12r_CTRL_163_163_PD_CML_REFCLK_CHOUTf_SET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_163_163_PD_CML_REFCLK_CHOUTf_SET
#define ANA_PLPPLL_CTRL12r_CTRL_162_162_EN_CK10T_OUTf_GET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_162_162_EN_CK10T_OUTf_GET
#define ANA_PLPPLL_CTRL12r_CTRL_162_162_EN_CK10T_OUTf_SET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_162_162_EN_CK10T_OUTf_SET
#define ANA_PLPPLL_CTRL12r_CTRL_161_161_EN_CK40T_OUTf_GET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_161_161_EN_CK40T_OUTf_GET
#define ANA_PLPPLL_CTRL12r_CTRL_161_161_EN_CK40T_OUTf_SET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_161_161_EN_CK40T_OUTf_SET
#define ANA_PLPPLL_CTRL12r_CTRL_160_160_EN_CK400T_OUTf_GET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_160_160_EN_CK400T_OUTf_GET
#define ANA_PLPPLL_CTRL12r_CTRL_160_160_EN_CK400T_OUTf_SET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_160_160_EN_CK400T_OUTf_SET
#define ANA_PLPPLL_CTRL12r_CTRL_175_166_RESERVED_PCIE3f_GET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_175_166_RESERVED_PCIE3f_GET
#define ANA_PLPPLL_CTRL12r_CTRL_175_166_RESERVED_PCIE3f_SET BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_CTRL_175_166_RESERVED_PCIE3f_SET
#define READ_ANA_PLPPLL_CTRL12r BCMI_APERTA_D_READ_ANA_PLPPLL_CTRL12r
#define WRITE_ANA_PLPPLL_CTRL12r BCMI_APERTA_D_WRITE_ANA_PLPPLL_CTRL12r
#define MODIFY_ANA_PLPPLL_CTRL12r BCMI_APERTA_D_MODIFY_ANA_PLPPLL_CTRL12r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_ANA_PLPPLL_CTRL12r'
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  PLLCAL_PLL_CALCTL_0
 * BLOCKS:   PLLCAL
 * REGADDR:  0x8150
 * DEVAD:    1
 * DESC:     PLL_CALCTL_0
 * RESETVAL: 0x1ffa (8186)
 * ACCESS:   R/W
 * FIELDS:
 *     CALIB_STEP_TIME  The number of divided vco/ref calibration clocks to wait after the pll range has been changed
 *     CAL_TH           programmable PLL calibration threshold for best, typical and worst case
 *
 ******************************************************************************/

/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_CHIP_REVISION
 * BLOCKS:   CTRL
 * REGADDR:  0x8b01
 * DEVAD:    1
 * DESC:     Chip REV Register
 * RESETVAL: 0x80a0 (32928)
 * ACCESS:   R/W
 * FIELDS:
 *     CHIP_REV         Rev ID
 *     CHIP_ID_19_16    Chip ID[19:16]
 *
 ******************************************************************************/
#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr (0x00018b01 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr_SIZE 4

/*
 * This structure should be used to declare and program CTRL_CHIP_REVISION.
 *
 */
typedef union BCMI_APERTA_D_CTRL_CHIP_REVISIONr_s {
     uint32_t v[1];
     uint32_t ctrl_chip_revision[1];
     uint32_t _ctrl_chip_revision;
} BCMI_APERTA_D_CTRL_CHIP_REVISIONr_t;

#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CLR(r) (r).ctrl_chip_revision[0] = 0
#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr_SET(r,d) (r).ctrl_chip_revision[0] = d
#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr_GET(r) (r).ctrl_chip_revision[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_GET(r) ((((r).ctrl_chip_revision[0]) >> 12) & 0xf)
#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_SET(r,f) (r).ctrl_chip_revision[0]=(((r).ctrl_chip_revision[0] & ~((uint32_t)0xf << 12)) | ((((uint32_t)f) & 0xf) << 12)) | (15 << (16 + 12))
#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(r) (((r).ctrl_chip_revision[0]) & 0xff)
#define BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_REVf_SET(r,f) (r).ctrl_chip_revision[0]=(((r).ctrl_chip_revision[0] & ~((uint32_t)0xff)) | (((uint32_t)f) & 0xff)) | (0xff << 16)

/*
 * These macros can be used to access CTRL_CHIP_REVISION.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_CHIP_REVISIONr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_CHIP_REVISIONr,(_r._ctrl_chip_revision))
#define BCMI_APERTA_D_WRITE_CTRL_CHIP_REVISIONr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_CHIP_REVISIONr,(_r._ctrl_chip_revision))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_CHIP_REVISIONr BCMI_APERTA_D_CTRL_CHIP_REVISIONr
#define CTRL_CHIP_REVISIONr_SIZE BCMI_APERTA_D_CTRL_CHIP_REVISIONr_SIZE
typedef BCMI_APERTA_D_CTRL_CHIP_REVISIONr_t CTRL_CHIP_REVISIONr_t;
#define CTRL_CHIP_REVISIONr_CLR BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CLR
#define CTRL_CHIP_REVISIONr_SET BCMI_APERTA_D_CTRL_CHIP_REVISIONr_SET
#define CTRL_CHIP_REVISIONr_GET BCMI_APERTA_D_CTRL_CHIP_REVISIONr_GET
#define CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_GET BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_GET
#define CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_SET BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_SET
#define CTRL_CHIP_REVISIONr_CHIP_REVf_GET BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_REVf_GET
#define CTRL_CHIP_REVISIONr_CHIP_REVf_SET BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_REVf_SET
#define READ_CTRL_CHIP_REVISIONr BCMI_APERTA_D_READ_CTRL_CHIP_REVISIONr
#define WRITE_CTRL_CHIP_REVISIONr BCMI_APERTA_D_WRITE_CTRL_CHIP_REVISIONr
#define MODIFY_CTRL_CHIP_REVISIONr BCMI_APERTA_D_MODIFY_CTRL_CHIP_REVISIONr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_CHIP_REVISIONr'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_PORT0_CONFIG
 * BLOCKS:   CTRL
 * REGADDR:  0x8b03
 * DEVAD:    1
 * DESC:     Port0 Configuration Register
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     PORT_SPEED_0     Port speed
 *     PTP_ENABLED_0    PTP enabled.PTP_enabled = 0 : PTP is disabledPTP_enabled = 1 : PTP is enabled
 *     FAULT_OPTION_0   Fault option.Fault_option = 0 : Option-1 (Terminate & generate)Fault_option = 1 : Option-2 (Pass through)
 *     FC_OPTION_0      Flow control option.FC_option = 0 : Option-1 (Terminate & generate)FC_option = 1 : Option-2 (Pass through)
 *     SF_ENABLED_0     Store & Forward mode enabled.Enabled = 1When enabled store & forward buffers are activeWhen disabled, store & forward buffers are not active and the cut-through mode is enabled
 *     PORT_ACTIVE_0    port is active
 *
 ******************************************************************************/
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr (0x00018b36 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_SIZE 4

/*
 * This structure should be used to declare and program CTRL_PORT0_CONFIG.
 *
 */
typedef union BCMI_APERTA_D_CTRL_PORT0_CONFIGr_s {
     uint32_t v[1];
     uint32_t ctrl_port0_config[1];
     uint32_t _ctrl_port0_config;
} BCMI_APERTA_D_CTRL_PORT0_CONFIGr_t;

#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_CLR(r) (r).ctrl_port0_config[0] = 0
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_SET(r,d) (r).ctrl_port0_config[0] = d
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_GET(r) (r).ctrl_port0_config[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PORT_ACTIVE_0f_GET(r) ((((r).ctrl_port0_config[0]) >> 15) & 0x1)
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PORT_ACTIVE_0f_SET(r,f) (r).ctrl_port0_config[0]=(((r).ctrl_port0_config[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15)) | (1 << (16 + 15))
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_SF_ENABLED_0f_GET(r) ((((r).ctrl_port0_config[0]) >> 14) & 0x1)
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_SF_ENABLED_0f_SET(r,f) (r).ctrl_port0_config[0]=(((r).ctrl_port0_config[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14)) | (1 << (16 + 14))
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_FC_OPTION_0f_GET(r) ((((r).ctrl_port0_config[0]) >> 13) & 0x1)
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_FC_OPTION_0f_SET(r,f) (r).ctrl_port0_config[0]=(((r).ctrl_port0_config[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13)) | (1 << (16 + 13))
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_FAULT_OPTION_0f_GET(r) ((((r).ctrl_port0_config[0]) >> 12) & 0x1)
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_FAULT_OPTION_0f_SET(r,f) (r).ctrl_port0_config[0]=(((r).ctrl_port0_config[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12)) | (1 << (16 + 12))
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PTP_ENABLED_0f_GET(r) ((((r).ctrl_port0_config[0]) >> 11) & 0x1)
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PTP_ENABLED_0f_SET(r,f) (r).ctrl_port0_config[0]=(((r).ctrl_port0_config[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11)) | (1 << (16 + 11))
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PORT_SPEED_0f_GET(r) (((r).ctrl_port0_config[0]) & 0xf)
#define BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PORT_SPEED_0f_SET(r,f) (r).ctrl_port0_config[0]=(((r).ctrl_port0_config[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf)) | (0xf << 16)

/*
 * These macros can be used to access CTRL_PORT0_CONFIG.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_PORT0_CONFIGr(_pc,_r)  PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_PORT0_CONFIGr,(_r._ctrl_port0_config))
#define BCMI_APERTA_D_WRITE_CTRL_PORT0_CONFIGr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_PORT0_CONFIGr,(_r._ctrl_port0_config))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_PORT0_CONFIGr BCMI_APERTA_D_CTRL_PORT0_CONFIGr
#define CTRL_PORT0_CONFIGr_SIZE BCMI_APERTA_D_CTRL_PORT0_CONFIGr_SIZE
typedef BCMI_APERTA_D_CTRL_PORT0_CONFIGr_t CTRL_PORT0_CONFIGr_t;
#define CTRL_PORT0_CONFIGr_CLR BCMI_APERTA_D_CTRL_PORT0_CONFIGr_CLR
#define CTRL_PORT0_CONFIGr_SET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_SET
#define CTRL_PORT0_CONFIGr_GET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_GET
#define CTRL_PORT0_CONFIGr_PORT_ACTIVE_0f_GET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PORT_ACTIVE_0f_GET
#define CTRL_PORT0_CONFIGr_PORT_ACTIVE_0f_SET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PORT_ACTIVE_0f_SET
#define CTRL_PORT0_CONFIGr_SF_ENABLED_0f_GET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_SF_ENABLED_0f_GET
#define CTRL_PORT0_CONFIGr_SF_ENABLED_0f_SET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_SF_ENABLED_0f_SET
#define CTRL_PORT0_CONFIGr_FC_OPTION_0f_GET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_FC_OPTION_0f_GET
#define CTRL_PORT0_CONFIGr_FC_OPTION_0f_SET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_FC_OPTION_0f_SET
#define CTRL_PORT0_CONFIGr_FAULT_OPTION_0f_GET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_FAULT_OPTION_0f_GET
#define CTRL_PORT0_CONFIGr_FAULT_OPTION_0f_SET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_FAULT_OPTION_0f_SET
#define CTRL_PORT0_CONFIGr_PTP_ENABLED_0f_GET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PTP_ENABLED_0f_GET
#define CTRL_PORT0_CONFIGr_PTP_ENABLED_0f_SET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PTP_ENABLED_0f_SET
#define CTRL_PORT0_CONFIGr_PORT_SPEED_0f_GET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PORT_SPEED_0f_GET
#define CTRL_PORT0_CONFIGr_PORT_SPEED_0f_SET BCMI_APERTA_D_CTRL_PORT0_CONFIGr_PORT_SPEED_0f_SET
#define READ_CTRL_PORT0_CONFIGr BCMI_APERTA_D_READ_CTRL_PORT0_CONFIGr
#define WRITE_CTRL_PORT0_CONFIGr BCMI_APERTA_D_WRITE_CTRL_PORT0_CONFIGr
#define MODIFY_CTRL_PORT0_CONFIGr BCMI_APERTA_D_MODIFY_CTRL_PORT0_CONFIGr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_PORT0_CONFIGr'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_SWGPREG00
 * BLOCKS:   CTRL
 * REGADDR:  0x8b30
 * DEVAD:    1
 * DESC:     Software General Puropose Register 00
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SWGPREG00_DATA   Data
 *
 ******************************************************************************/
#define BCMI_APERTA_D_CTRL_SWGPREG00r (0x00018b30 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_SWGPREG00r_SIZE 4

/*
 * This structure should be used to declare and program CTRL_SWGPREG00.
 *
 */
typedef union BCMI_APERTA_D_CTRL_SWGPREG00r_s {
     uint32_t v[1];
     uint32_t ctrl_swgpreg00[1];
     uint32_t _ctrl_swgpreg00;
} BCMI_APERTA_D_CTRL_SWGPREG00r_t;

#define BCMI_APERTA_D_CTRL_SWGPREG00r_CLR(r) (r).ctrl_swgpreg00[0] = 0
#define BCMI_APERTA_D_CTRL_SWGPREG00r_SET(r,d) (r).ctrl_swgpreg00[0] = d
#define BCMI_APERTA_D_CTRL_SWGPREG00r_GET(r) (r).ctrl_swgpreg00[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_SWGPREG00r_SWGPREG00_DATAf_GET(r) (((r).ctrl_swgpreg00[0]) & 0xffff)
#define BCMI_APERTA_D_CTRL_SWGPREG00r_SWGPREG00_DATAf_SET(r,f) (r).ctrl_swgpreg00[0]=(((r).ctrl_swgpreg00[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access CTRL_SWGPREG00.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_SWGPREG00r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_SWGPREG00r,(_r._ctrl_swgpreg00))
#define BCMI_APERTA_D_WRITE_CTRL_SWGPREG00r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_SWGPREG00r,(_r._ctrl_swgpreg00))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_SWGPREG00r BCMI_APERTA_D_CTRL_SWGPREG00r
#define CTRL_SWGPREG00r_SIZE BCMI_APERTA_D_CTRL_SWGPREG00r_SIZE
typedef BCMI_APERTA_D_CTRL_SWGPREG00r_t CTRL_SWGPREG00r_t;
#define CTRL_SWGPREG00r_CLR BCMI_APERTA_D_CTRL_SWGPREG00r_CLR
#define CTRL_SWGPREG00r_SET BCMI_APERTA_D_CTRL_SWGPREG00r_SET
#define CTRL_SWGPREG00r_GET BCMI_APERTA_D_CTRL_SWGPREG00r_GET
#define CTRL_SWGPREG00r_SWGPREG00_DATAf_GET BCMI_APERTA_D_CTRL_SWGPREG00r_SWGPREG00_DATAf_GET
#define CTRL_SWGPREG00r_SWGPREG00_DATAf_SET BCMI_APERTA_D_CTRL_SWGPREG00r_SWGPREG00_DATAf_SET
#define READ_CTRL_SWGPREG00r BCMI_APERTA_D_READ_CTRL_SWGPREG00r
#define WRITE_CTRL_SWGPREG00r BCMI_APERTA_D_WRITE_CTRL_SWGPREG00r
#define MODIFY_CTRL_SWGPREG00r BCMI_APERTA_D_MODIFY_CTRL_SWGPREG00r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_SWGPREG00r'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_SWGPREG04
 * BLOCKS:   CTRL
 * REGADDR:  0x8b34
 * DEVAD:    1
 * DESC:     Software General Puropose Register 04
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SWGPREG04_DATA   Data
 *
 ******************************************************************************/
#define BCMI_APERTA_D_CTRL_SWGPREG04r (0x00018b34 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_SWGPREG04r_SIZE 4

/*
 * This structure should be used to declare and program CTRL_SWGPREG04.
 *
 */
typedef union BCMI_APERTA_D_CTRL_SWGPREG04r_s {
     uint32_t v[1];
     uint32_t ctrl_swgpreg04[1];
     uint32_t _ctrl_swgpreg04;
} BCMI_APERTA_D_CTRL_SWGPREG04r_t;

#define BCMI_APERTA_D_CTRL_SWGPREG04r_CLR(r) (r).ctrl_swgpreg04[0] = 0
#define BCMI_APERTA_D_CTRL_SWGPREG04r_SET(r,d) (r).ctrl_swgpreg04[0] = d
#define BCMI_APERTA_D_CTRL_SWGPREG04r_GET(r) (r).ctrl_swgpreg04[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_SWGPREG04r_SWGPREG04_DATAf_GET(r) (((r).ctrl_swgpreg04[0]) & 0xffff)
#define BCMI_APERTA_D_CTRL_SWGPREG04r_SWGPREG04_DATAf_SET(r,f) (r).ctrl_swgpreg04[0]=(((r).ctrl_swgpreg04[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access CTRL_SWGPREG04.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_SWGPREG04r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_SWGPREG04r,(_r._ctrl_swgpreg04))
#define BCMI_APERTA_D_WRITE_CTRL_SWGPREG04r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_SWGPREG04r,(_r._ctrl_swgpreg04))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_SWGPREG04r BCMI_APERTA_D_CTRL_SWGPREG04r
#define CTRL_SWGPREG04r_SIZE BCMI_APERTA_D_CTRL_SWGPREG04r_SIZE
typedef BCMI_APERTA_D_CTRL_SWGPREG04r_t CTRL_SWGPREG04r_t;
#define CTRL_SWGPREG04r_CLR BCMI_APERTA_D_CTRL_SWGPREG04r_CLR
#define CTRL_SWGPREG04r_SET BCMI_APERTA_D_CTRL_SWGPREG04r_SET
#define CTRL_SWGPREG04r_GET BCMI_APERTA_D_CTRL_SWGPREG04r_GET
#define CTRL_SWGPREG04r_SWGPREG04_DATAf_GET BCMI_APERTA_D_CTRL_SWGPREG04r_SWGPREG04_DATAf_GET
#define CTRL_SWGPREG04r_SWGPREG04_DATAf_SET BCMI_APERTA_D_CTRL_SWGPREG04r_SWGPREG04_DATAf_SET
#define READ_CTRL_SWGPREG04r BCMI_APERTA_D_READ_CTRL_SWGPREG04r
#define WRITE_CTRL_SWGPREG04r BCMI_APERTA_D_WRITE_CTRL_SWGPREG04r
#define MODIFY_CTRL_SWGPREG04r BCMI_APERTA_D_MODIFY_CTRL_SWGPREG04r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_SWGPREG04r'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  CTRL_SWGPREG1E
 * BLOCKS:   CTRL
 * REGADDR:  0x8b4e
 * DEVAD:    1
 * DESC:     Software General Puropose Register 1E
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     SWGPREG1E_DATA   Data
 *
 ******************************************************************************/
#define BCMI_APERTA_D_CTRL_SWGPREG1Er (0x00018b4e | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_CTRL_SWGPREG1Er_SIZE 4

/*
 * This structure should be used to declare and program CTRL_SWGPREG1E.
 *
 */
typedef union BCMI_APERTA_D_CTRL_SWGPREG1Er_s {
     uint32_t v[1];
     uint32_t ctrl_swgpreg1e[1];
     uint32_t _ctrl_swgpreg1e;
} BCMI_APERTA_D_CTRL_SWGPREG1Er_t;

#define BCMI_APERTA_D_CTRL_SWGPREG1Er_CLR(r) (r).ctrl_swgpreg1e[0] = 0
#define BCMI_APERTA_D_CTRL_SWGPREG1Er_SET(r,d) (r).ctrl_swgpreg1e[0] = d
#define BCMI_APERTA_D_CTRL_SWGPREG1Er_GET(r) (r).ctrl_swgpreg1e[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_CTRL_SWGPREG1Er_SWGPREG1E_DATAf_GET(r) (((r).ctrl_swgpreg1e[0]) & 0xffff)
#define BCMI_APERTA_D_CTRL_SWGPREG1Er_SWGPREG1E_DATAf_SET(r,f) (r).ctrl_swgpreg1e[0]=(((r).ctrl_swgpreg1e[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access CTRL_SWGPREG1E.
 *
 */
#define BCMI_APERTA_D_READ_CTRL_SWGPREG1Er(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_CTRL_SWGPREG1Er,(_r._ctrl_swgpreg1e))
#define BCMI_APERTA_D_WRITE_CTRL_SWGPREG1Er(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_CTRL_SWGPREG1Er,(_r._ctrl_swgpreg1e))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define CTRL_SWGPREG1Er BCMI_APERTA_D_CTRL_SWGPREG1Er
#define CTRL_SWGPREG1Er_SIZE BCMI_APERTA_D_CTRL_SWGPREG1Er_SIZE
typedef BCMI_APERTA_D_CTRL_SWGPREG1Er_t CTRL_SWGPREG1Er_t;
#define CTRL_SWGPREG1Er_CLR BCMI_APERTA_D_CTRL_SWGPREG1Er_CLR
#define CTRL_SWGPREG1Er_SET BCMI_APERTA_D_CTRL_SWGPREG1Er_SET
#define CTRL_SWGPREG1Er_GET BCMI_APERTA_D_CTRL_SWGPREG1Er_GET
#define CTRL_SWGPREG1Er_SWGPREG1E_DATAf_GET BCMI_APERTA_D_CTRL_SWGPREG1Er_SWGPREG1E_DATAf_GET
#define CTRL_SWGPREG1Er_SWGPREG1E_DATAf_SET BCMI_APERTA_D_CTRL_SWGPREG1Er_SWGPREG1E_DATAf_SET
#define READ_CTRL_SWGPREG1Er BCMI_APERTA_D_READ_CTRL_SWGPREG1Er
#define WRITE_CTRL_SWGPREG1Er BCMI_APERTA_D_WRITE_CTRL_SWGPREG1Er
#define MODIFY_CTRL_SWGPREG1Er BCMI_APERTA_D_MODIFY_CTRL_SWGPREG1Er

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_CTRL_SWGPREG1Er'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_GEN_CONTROL2
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8202
 * DEVAD:    1
 * DESC:     General control register 2.
 * RESETVAL: 0x3 (3)
 * ACCESS:   R/W
 * FIELDS:
 *     MST_RSTB         Reset for Master M0 micro (Active low).
 *     MST_UCP_RSTB     Reset for Master M0 micro peripherals(Active low).
 *
 ******************************************************************************/
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r (0x00018202 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_GEN_CONTROL2.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_s {
     uint32_t v[1];
     uint32_t gen_cntrls_gen_control2[1];
     uint32_t _gen_cntrls_gen_control2;
} BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_t;

#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_CLR(r) (r).gen_cntrls_gen_control2[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_SET(r,d) (r).gen_cntrls_gen_control2[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_GET(r) (r).gen_cntrls_gen_control2[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_GET(r) ((((r).gen_cntrls_gen_control2[0]) >> 1) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(r,f) (r).gen_cntrls_gen_control2[0]=(((r).gen_cntrls_gen_control2[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_GET(r) (((r).gen_cntrls_gen_control2[0]) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(r,f) (r).gen_cntrls_gen_control2[0]=(((r).gen_cntrls_gen_control2[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access GEN_CNTRLS_GEN_CONTROL2.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r,(_r._gen_cntrls_gen_control2))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r,(_r._gen_cntrls_gen_control2))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_GEN_CONTROL2r BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r
#define GEN_CNTRLS_GEN_CONTROL2r_SIZE BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_t GEN_CNTRLS_GEN_CONTROL2r_t;
#define GEN_CNTRLS_GEN_CONTROL2r_CLR BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_CLR
#define GEN_CNTRLS_GEN_CONTROL2r_SET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_SET
#define GEN_CNTRLS_GEN_CONTROL2r_GET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_GET
#define GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_GET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_GET
#define GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET
#define GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_GET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_GET
#define GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET
#define READ_GEN_CNTRLS_GEN_CONTROL2r BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL2r
#define WRITE_GEN_CNTRLS_GEN_CONTROL2r BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL2r
#define MODIFY_GEN_CNTRLS_GEN_CONTROL2r BCMI_APERTA_D_MODIFY_GEN_CNTRLS_GEN_CONTROL2r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL2r'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_MST_MSGIN
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8222
 * DEVAD:    1
 * DESC:     Incoming message from external world to Master micro
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MST_MSGIN_VAL    message from external world to Master micro.
 *
 ******************************************************************************/
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr (0x00018222 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_MST_MSGIN.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_s {
     uint32_t v[1];
     uint32_t gen_cntrls_mst_msgin[1];
     uint32_t _gen_cntrls_mst_msgin;
} BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_t;

#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_CLR(r) (r).gen_cntrls_mst_msgin[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET(r,d) (r).gen_cntrls_mst_msgin[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_GET(r) (r).gen_cntrls_mst_msgin[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_MST_MSGIN_VALf_GET(r) (((r).gen_cntrls_mst_msgin[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_MST_MSGIN_VALf_SET(r,f) (r).gen_cntrls_mst_msgin[0]=(((r).gen_cntrls_mst_msgin[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_MST_MSGIN.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGINr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr,(_r._gen_cntrls_mst_msgin))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr,(_r._gen_cntrls_mst_msgin))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_MST_MSGINr BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr
#define GEN_CNTRLS_MST_MSGINr_SIZE BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_t GEN_CNTRLS_MST_MSGINr_t;
#define GEN_CNTRLS_MST_MSGINr_CLR BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_CLR
#define GEN_CNTRLS_MST_MSGINr_SET BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_SET
#define GEN_CNTRLS_MST_MSGINr_GET BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_GET
#define GEN_CNTRLS_MST_MSGINr_MST_MSGIN_VALf_GET BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_MST_MSGIN_VALf_GET
#define GEN_CNTRLS_MST_MSGINr_MST_MSGIN_VALf_SET BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr_MST_MSGIN_VALf_SET
#define READ_GEN_CNTRLS_MST_MSGINr BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGINr
#define WRITE_GEN_CNTRLS_MST_MSGINr BCMI_APERTA_D_WRITE_GEN_CNTRLS_MST_MSGINr
#define MODIFY_GEN_CNTRLS_MST_MSGINr BCMI_APERTA_D_MODIFY_GEN_CNTRLS_MST_MSGINr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_SPI_CODE_LOAD_EN
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8210
 * DEVAD:    1
 * DESC:     M0 SPI code download control
 * RESETVAL: 0x33 (51)
 * ACCESS:   R/W
 * FIELDS:
 *     MST_CODE_DOWNLOAD_EN Setting this will enable loading of code into master code ram during SPI download. The start pointer needs to be programmed first. A rising edge on this signal"will initialize the start pointer".
 *     SPI_MST_OEB      Outputs of SPI master - mosi, ssn and sclk can be tristated when" not in use. 0 - outputs are enabled. 1 - Outputs are tristated".
 *     SPI_ROM_MODE     This bit selects the 2nd timing mode for the SPIROM SPI master1'b1 - Better hold time1'b0 - Original SPISROM master timing.
 *     SLV_CODE_DOWNLOAD_EN Setting this will enable loading of code into slave M0 code rambit 6:4 corresponding to slave 6:1slave  bit 5: System side Blackhawk M0slave  bit 4: Line side Blackhawk M0
 *
 ******************************************************************************/
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr (0x00018210 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_SPI_CODE_LOAD_EN.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_s {
     uint32_t v[1];
     uint32_t gen_cntrls_spi_code_load_en[1];
     uint32_t _gen_cntrls_spi_code_load_en;
} BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t;

#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_CLR(r) (r).gen_cntrls_spi_code_load_en[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SET(r,d) (r).gen_cntrls_spi_code_load_en[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_GET(r) (r).gen_cntrls_spi_code_load_en[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_GET(r) ((((r).gen_cntrls_spi_code_load_en[0]) >> 4) & 0x3)
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_SET(r,f) (r).gen_cntrls_spi_code_load_en[0]=(((r).gen_cntrls_spi_code_load_en[0] & ~((uint32_t)0x3 << 4)) | ((((uint32_t)f) & 0x3) << 4)) | (3 << (16 + 4))
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_ROM_MODEf_GET(r) ((((r).gen_cntrls_spi_code_load_en[0]) >> 2) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_ROM_MODEf_SET(r,f) (r).gen_cntrls_spi_code_load_en[0]=(((r).gen_cntrls_spi_code_load_en[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_GET(r) ((((r).gen_cntrls_spi_code_load_en[0]) >> 1) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_SET(r,f) (r).gen_cntrls_spi_code_load_en[0]=(((r).gen_cntrls_spi_code_load_en[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_GET(r) (((r).gen_cntrls_spi_code_load_en[0]) & 0x1)
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_SET(r,f) (r).gen_cntrls_spi_code_load_en[0]=(((r).gen_cntrls_spi_code_load_en[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1)) | (0x1 << 16)

/*
 * These macros can be used to access GEN_CNTRLS_SPI_CODE_LOAD_EN.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr,(_r._gen_cntrls_spi_code_load_en))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr,(_r._gen_cntrls_spi_code_load_en))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_SPI_CODE_LOAD_ENr BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_SIZE BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t GEN_CNTRLS_SPI_CODE_LOAD_ENr_t;
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_CLR BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_CLR
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_SET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_GET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_GET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_GET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_GET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_SET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_SET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_ROM_MODEf_GET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_ROM_MODEf_GET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_ROM_MODEf_SET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_ROM_MODEf_SET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_GET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_GET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_SET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_SET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_GET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_GET
#define GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_SET BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_SET
#define READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr
#define WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr
#define MODIFY_GEN_CNTRLS_SPI_CODE_LOAD_ENr BCMI_APERTA_D_MODIFY_GEN_CNTRLS_SPI_CODE_LOAD_ENr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_SPI_CODE_LOAD_ENr'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  GEN_CNTRLS_SPI_MST_CODE_START_PTR
 * BLOCKS:   GEN_CNTRLS
 * REGADDR:  0x8220
 * DEVAD:    1
 * DESC:     Master M0 SPI download start Pointer
 * RESETVAL: 0x0 (0)
 * ACCESS:   R/W
 * FIELDS:
 *     MST_CODE_START_PTR Byte address in the M0 Master code RAM to start loading the program from external EEPROM. Complete 32-bit M0 address for this is obtained by prepending  16'b0001_0000_0000_0000 (code ram base)
 *
 ******************************************************************************/
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr (0x00018220 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SIZE 4

/*
 * This structure should be used to declare and program GEN_CNTRLS_SPI_MST_CODE_START_PTR.
 *
 */
typedef union BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_s {
     uint32_t v[1];
     uint32_t gen_cntrls_spi_mst_code_start_ptr[1];
     uint32_t _gen_cntrls_spi_mst_code_start_ptr;
} BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t;

#define BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_CLR(r) (r).gen_cntrls_spi_mst_code_start_ptr[0] = 0
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SET(r,d) (r).gen_cntrls_spi_mst_code_start_ptr[0] = d
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_GET(r) (r).gen_cntrls_spi_mst_code_start_ptr[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_MST_CODE_START_PTRf_GET(r) (((r).gen_cntrls_spi_mst_code_start_ptr[0]) & 0xffff)
#define BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_MST_CODE_START_PTRf_SET(r,f) (r).gen_cntrls_spi_mst_code_start_ptr[0]=(((r).gen_cntrls_spi_mst_code_start_ptr[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff)) | (0xffff << 16)

/*
 * These macros can be used to access GEN_CNTRLS_SPI_MST_CODE_START_PTR.
 *
 */
#define BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_MST_CODE_START_PTRr(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr,(_r._gen_cntrls_spi_mst_code_start_ptr))
#define BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_MST_CODE_START_PTRr(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr,(_r._gen_cntrls_spi_mst_code_start_ptr))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define GEN_CNTRLS_SPI_MST_CODE_START_PTRr BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr
#define GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SIZE BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SIZE
typedef BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t;
#define GEN_CNTRLS_SPI_MST_CODE_START_PTRr_CLR BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_CLR
#define GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SET BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SET
#define GEN_CNTRLS_SPI_MST_CODE_START_PTRr_GET BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_GET
#define GEN_CNTRLS_SPI_MST_CODE_START_PTRr_MST_CODE_START_PTRf_GET BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_MST_CODE_START_PTRf_GET
#define GEN_CNTRLS_SPI_MST_CODE_START_PTRr_MST_CODE_START_PTRf_SET BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_MST_CODE_START_PTRf_SET
#define READ_GEN_CNTRLS_SPI_MST_CODE_START_PTRr BCMI_APERTA_D_READ_GEN_CNTRLS_SPI_MST_CODE_START_PTRr
#define WRITE_GEN_CNTRLS_SPI_MST_CODE_START_PTRr BCMI_APERTA_D_WRITE_GEN_CNTRLS_SPI_MST_CODE_START_PTRr
#define MODIFY_GEN_CNTRLS_SPI_MST_CODE_START_PTRr BCMI_APERTA_D_MODIFY_GEN_CNTRLS_SPI_MST_CODE_START_PTRr

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_GEN_CNTRLS_SPI_MST_CODE_START_PTRr'
 ******************************************************************************/
/*******************************************************************************
 * CHIP:  BCMI_APERTA_D
 * REGISTER:  PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1
 * BLOCKS:   PAD_CNTRL
 * REGADDR:  0x8a49
 * DEVAD:    1
 * DESC:     PAD ext_uc_rstb_out control 1 register
 * RESETVAL: 0x2 (2)
 * ACCESS:   R/W
 * FIELDS:
 *     EXT_UC_RSTB_OUT_SEL0 ext_uc_rstb_out sel0
 *     EXT_UC_RSTB_OUT_SEL1 ext_uc_rstb_out sel1
 *     EXT_UC_RSTB_OUT_SEL2 ext_uc_rstb_out sel2
 *     EXT_UC_RSTB_OUT_DOUT_FRCVAL ext_uc_rstb_out dout force value
 *     EXT_UC_RSTB_OUT_DOUT_FRC ext_uc_rstb_out dout force enable
 *     EXT_UC_RSTB_OUT_DOUT_INVERT_EN ext_uc_rstb_out dout polarity inversion enable
 *
 ******************************************************************************/
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r (0x00018a49 | PHYMOD_APERTA_DIRECT_BASE_ADR)

#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_SIZE 4

/*
 * This structure should be used to declare and program PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1.
 *
 */
typedef union BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_s {
     uint32_t v[1];
     uint32_t pad_cntrl_ext_uc_rstb_out_control_1[1];
     uint32_t _pad_cntrl_ext_uc_rstb_out_control_1;
} BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_t;

#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_CLR(r) (r).pad_cntrl_ext_uc_rstb_out_control_1[0] = 0
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_SET(r,d) (r).pad_cntrl_ext_uc_rstb_out_control_1[0] = d
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_GET(r) (r).pad_cntrl_ext_uc_rstb_out_control_1[0]

/*
 * These macros can be used to access individual fields.
 *
 */
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_INVERT_ENf_GET(r) ((((r).pad_cntrl_ext_uc_rstb_out_control_1[0]) >> 7) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_INVERT_ENf_SET(r,f) (r).pad_cntrl_ext_uc_rstb_out_control_1[0]=(((r).pad_cntrl_ext_uc_rstb_out_control_1[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7)) | (1 << (16 + 7))
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCf_GET(r) ((((r).pad_cntrl_ext_uc_rstb_out_control_1[0]) >> 6) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCf_SET(r,f) (r).pad_cntrl_ext_uc_rstb_out_control_1[0]=(((r).pad_cntrl_ext_uc_rstb_out_control_1[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6)) | (1 << (16 + 6))
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCVALf_GET(r) ((((r).pad_cntrl_ext_uc_rstb_out_control_1[0]) >> 5) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCVALf_SET(r,f) (r).pad_cntrl_ext_uc_rstb_out_control_1[0]=(((r).pad_cntrl_ext_uc_rstb_out_control_1[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5)) | (1 << (16 + 5))
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL2f_GET(r) ((((r).pad_cntrl_ext_uc_rstb_out_control_1[0]) >> 3) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL2f_SET(r,f) (r).pad_cntrl_ext_uc_rstb_out_control_1[0]=(((r).pad_cntrl_ext_uc_rstb_out_control_1[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3)) | (1 << (16 + 3))
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL1f_GET(r) ((((r).pad_cntrl_ext_uc_rstb_out_control_1[0]) >> 2) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL1f_SET(r,f) (r).pad_cntrl_ext_uc_rstb_out_control_1[0]=(((r).pad_cntrl_ext_uc_rstb_out_control_1[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2)) | (1 << (16 + 2))
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL0f_GET(r) ((((r).pad_cntrl_ext_uc_rstb_out_control_1[0]) >> 1) & 0x1)
#define BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL0f_SET(r,f) (r).pad_cntrl_ext_uc_rstb_out_control_1[0]=(((r).pad_cntrl_ext_uc_rstb_out_control_1[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1)) | (1 << (16 + 1))

/*
 * These macros can be used to access PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1.
 *
 */
#define BCMI_APERTA_D_READ_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r(_pc,_r) PHYMOD_BUS_READ(_pc,BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r,(_r._pad_cntrl_ext_uc_rstb_out_control_1))
#define BCMI_APERTA_D_WRITE_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r(_pc,_r) PHYMOD_BUS_WRITE(_pc,BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r,(_r._pad_cntrl_ext_uc_rstb_out_control_1))

/*
 * Unless PHYMOD_EXCLUDE_CHIPLESS_TYPES is defined, all of the above types
 * will be redefined without the chip prefix for easier programming.
 * If multiple chips will be programmed in the same source file, then you should
 * define PHYMOD_EXCLUDE_CHIPLESS_TYPES before including all chip header files
 * and refer to the fully qualified versions.
 *
 */
#ifndef PHYMOD_EXCLUDE_CHIPLESS_TYPES

#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_SIZE BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_SIZE
typedef BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_t PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_t;
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_CLR BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_CLR
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_SET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_SET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_GET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_GET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_INVERT_ENf_GET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_INVERT_ENf_GET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_INVERT_ENf_SET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_INVERT_ENf_SET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCf_GET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCf_GET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCf_SET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCf_SET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCVALf_GET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCVALf_GET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCVALf_SET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_DOUT_FRCVALf_SET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL2f_GET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL2f_GET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL2f_SET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL2f_SET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL1f_GET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL1f_GET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL1f_SET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL1f_SET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL0f_GET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL0f_GET
#define PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL0f_SET BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r_EXT_UC_RSTB_OUT_SEL0f_SET
#define READ_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r BCMI_APERTA_D_READ_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r
#define WRITE_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r BCMI_APERTA_D_WRITE_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r
#define MODIFY_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r BCMI_APERTA_D_MODIFY_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r

#endif /* PHYMOD_EXCLUDE_CHIPLESS_TYPES */
/*******************************************************************************
 * End of 'BCMI_APERTA_D_PAD_CNTRL_EXT_UC_RSTB_OUT_CONTROL_1r'
 ******************************************************************************/
#endif /* BCMI_APERTA_D_DEFS_H */
