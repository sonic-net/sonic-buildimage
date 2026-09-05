/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018-2019, Pensando Systems Inc.
 */

#ifndef __PCIETLP_H__
#define __PCIETLP_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

/*
 * PCIe Transaction Layer Protocol, based on
 *     PCI Express Base Specification
 *     Revision 4.0 Version 1.0
 *     September 27, 2017
 */

typedef enum pcie_stlp_type_e {
    PCIE_STLP_MALFORMED,        /* malformed tlp */
    PCIE_STLP_CFGRD,            /* cfg (type 0) read */
    PCIE_STLP_CFGWR,            /* cfg (type 0) write */
    PCIE_STLP_CFGRD1,           /* cfg (type 1) read */
    PCIE_STLP_CFGWR1,           /* cfg (type 1) write */
    PCIE_STLP_MEMRD,            /* memory read */
    PCIE_STLP_MEMWR,            /* memory write */
    PCIE_STLP_MEMRD64,          /* memory read - 64-bit addr */
    PCIE_STLP_MEMWR64,          /* memory write - 64-bit addr */
    PCIE_STLP_IORD,             /* I/O space read */
    PCIE_STLP_IOWR,             /* I/O space write */
    PCIE_STLP_MSG,              /* message */
    PCIE_STLP_MSGD,             /* message with data */
} pcie_stlp_type_t;

typedef struct pcie_stlp_s {
    u_int8_t type;              /* tlp type PCIE_STLP_* */
    u_int16_t reqid;            /* requester id */
    u_int16_t tag;              /* tag of request */
    u_int16_t bdf;              /* bus,dev,fun of request */
    u_int16_t size;             /* size of request */
    u_int64_t addr;             /* address */
    u_int64_t data;             /* payload data */
} pcie_stlp_t;

int pcietlp_decode(pcie_stlp_t *stlp, const void *rtlp, const size_t rtlpsz);
int pcietlp_encode(const pcie_stlp_t *stlp, void *rtlp, const size_t rtlpsz);
char *pcietlp_get_error(void);
char *pcietlp_buf(const pcie_stlp_t *stlp, void *buf, const size_t bufsz);
char *pcietlp_str(const pcie_stlp_t *stlp);

/*
 * PCIe Base Spec, Table 2-2
 */
#define PCIE_TLP_FMT_3DW        0x0     /* 3 DW header, no data (read) */
#define PCIE_TLP_FMT_4DW        0x1     /* 4 DW header, no data (read) */
#define PCIE_TLP_FMT_3DWD       0x2     /* 3 DW header, with data (write) */
#define PCIE_TLP_FMT_4DWD       0x3     /* 4 DW header, with data (write) */
#define PCIE_TLP_FMT_PREF       0x4     /* TLP prefix */

#define mk_tlp_type(fmt, type)    (PCIE_TLP_FMT_##fmt << 5 | ((type) & 0x1f))

/*
 * PCIe Base Spec, Table 2-3
 */
typedef enum pcie_tlp_type_e {
    PCIE_TLP_TYPE_MEMRD   = mk_tlp_type(3DW,  0x0),
    PCIE_TLP_TYPE_MEMRD64 = mk_tlp_type(4DW,  0x0),
    PCIE_TLP_TYPE_MEMWR   = mk_tlp_type(3DWD, 0x0),
    PCIE_TLP_TYPE_MEMWR64 = mk_tlp_type(4DWD, 0x0),
    PCIE_TLP_TYPE_IORD    = mk_tlp_type(3DW,  0x2),
    PCIE_TLP_TYPE_IOWR    = mk_tlp_type(3DWD, 0x2),
    PCIE_TLP_TYPE_CFGRD0  = mk_tlp_type(3DW,  0x4),
    PCIE_TLP_TYPE_CFGWR0  = mk_tlp_type(3DWD, 0x4),
    PCIE_TLP_TYPE_CFGRD1  = mk_tlp_type(3DW,  0x5),
    PCIE_TLP_TYPE_CFGWR1  = mk_tlp_type(3DWD, 0x5),
} pcie_tlp_type_t;

typedef struct pcie_tlp_common_hdr_s {
    /* dword 0 */
    u_int32_t type:8;           /* transaction type */

    u_int32_t th:1;             /* tlp hint */
    u_int32_t ln:1;             /* lightweight notification */
    u_int32_t attr_hi:1;        /* attributes[2] */
    u_int32_t t8:1;             /* tag[8] */
    u_int32_t tc:3;             /* traffic class */
    u_int32_t t9:1;             /* tag[9] */

    u_int32_t len_hi:2;         /* length[8:9] (dw) */
    u_int32_t at:2;             /* at[0:1] */
    u_int32_t attr_lo:2;        /* attributes[0:1] */
    u_int32_t ep:1;             /* error poisoned */
    u_int32_t td:1;             /* tlp digest */

    u_int32_t len_lo:8;         /* length[0:7] (dw) */

    /* dword 1 */
    u_int32_t reqid:16;         /* requester id */

    u_int32_t tag:8;            /* transaction tag */

    u_int32_t fbe:4;            /* first dw byte enable */
    u_int32_t lbe:4;            /* last dw byte enable */
} __attribute__((packed)) pcie_tlp_common_hdr_t;

typedef struct pcie_tlp_cfg_s {
    /* dword 0 */
    u_int32_t type:8;           /* transaction type */

    u_int32_t th:1;             /* tlp hint */
    u_int32_t ln:1;             /* lightweight notification */
    u_int32_t attr_hi:1;        /* attributes[2] */
    u_int32_t t8:1;             /* tag[8] */
    u_int32_t tc:3;             /* traffic class */
    u_int32_t t9:1;             /* tag[9] */

    u_int32_t len_hi:2;         /* length[8:9] (dw) */
    u_int32_t at:2;             /* at[0:1] */
    u_int32_t attr_lo:2;        /* attributes[0:1] */
    u_int32_t ep:1;             /* error poisoned */
    u_int32_t td:1;             /* tlp digest */

    u_int32_t len_lo:8;         /* length[0:7] (dw) */

    /* dword 1 */
    u_int32_t reqid:16;         /* requester id */

    u_int32_t tag:8;            /* transaction tag */

    u_int32_t fbe:4;            /* first dw byte enable */
    u_int32_t lbe:4;            /* last dw byte enable */

    /* dword 2 */
    u_int32_t bdf:16;           /* bus,dev,fun target */

    u_int32_t extreg:4;         /* extended register number */
    u_int32_t rsrv:4;           /* reserved */

    u_int32_t reg:8;            /* register number */
} __attribute__((packed)) pcie_tlp_cfg_t;

typedef struct pcie_tlp_mem32_s {
    /* dword 0 */
    u_int32_t type:8;           /* transaction type */

    u_int32_t th:1;             /* tlp hint */
    u_int32_t ln:1;             /* lightweight notification */
    u_int32_t attr_hi:1;        /* attributes[2] */
    u_int32_t t8:1;             /* tag[8] */
    u_int32_t tc:3;             /* traffic class */
    u_int32_t t9:1;             /* tag[9] */

    u_int32_t len_hi:2;         /* length[8:9] (dw) */
    u_int32_t at:2;             /* at[0:1] */
    u_int32_t attr_lo:2;        /* attributes[0:1] */
    u_int32_t ep:1;             /* error poisoned */
    u_int32_t td:1;             /* tlp digest */

    u_int32_t len_lo:8;         /* length[0:7] (dw) */

    /* dword 1 */
    u_int32_t reqid:16;         /* requester id */

    u_int32_t tag:8;            /* transaction tag */

    u_int32_t fbe:4;            /* first dw byte enable */
    u_int32_t lbe:4;            /* last dw byte enable */

    /* dword 2 */
    u_int32_t addr;             /* address[31:2] */
} __attribute__((packed)) pcie_tlp_mem32_t;

/* I/O similar to mem32 */
typedef pcie_tlp_mem32_t pcie_tlp_io_t;

typedef struct pcie_tlp_mem64_s {
    /* dword 0 */
    u_int32_t type:8;           /* transaction type */

    u_int32_t th:1;             /* tlp hint */
    u_int32_t ln:1;             /* lightweight notification */
    u_int32_t attr_hi:1;        /* attributes[2] */
    u_int32_t t8:1;             /* tag[8] */
    u_int32_t tc:3;             /* traffic class */
    u_int32_t t9:1;             /* tag[9] */

    u_int32_t len_hi:2;         /* length[8:9] (dw) */
    u_int32_t at:2;             /* at[0:1] */
    u_int32_t attr_lo:2;        /* attributes[0:1] */
    u_int32_t ep:1;             /* error poisoned */
    u_int32_t td:1;             /* tlp digest */

    u_int32_t len_lo:8;         /* length[0:7] (dw) */

    /* dword 1 */
    u_int32_t reqid:16;         /* requester id */

    u_int32_t tag:8;            /* transaction tag */

    u_int32_t fbe:4;            /* first dw byte enable */
    u_int32_t lbe:4;            /* last dw byte enable */

    /* dword 2 */
    u_int32_t addr_hi;          /* address[63:32] */

    /* dword 3 */
    u_int32_t addr_lo;          /* address[31:2] */
} __attribute__((packed)) pcie_tlp_mem64_t;

#ifdef __cplusplus
}
#endif

#endif /* __PCIETLP_H__ */
