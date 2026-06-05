/* HAL physical operation. This is shared between all HAL
 * so we put it into SDK itself and export. */

#include "dii.h"

#include "crintl/logger.h"
#include "sdk.h"

#include <stdlib.h>

// internal use, access directly without slice lock. Usage for MCU sim server.
CredoError_t cri_slice_read_directly(CredoSlice_t* slice, unsigned address, unsigned* value) {
    return (slice->sdk->read_register)(slice->slice_context, address, value);
}

// internal use, access directly without slice lock. Usage for MCU sim server.
CredoError_t cri_slice_write_directly(CredoSlice_t* slice, unsigned address, unsigned value) {
    return (slice->sdk->write_register)(slice->slice_context, address, value);
}

CredoError_t cr_slice_read(CredoSlice_t* slice, unsigned address, unsigned* value) {
    if (slice == NULL || value == NULL) return CR_INVALID_ARGS;
    SLICE_LOCK_GUARD(slice);
    *value = 0;

    CredoSdk_t* sdk = slice->sdk;
    CredoError_t ret = (sdk->read_register)(slice->slice_context, address, value);

    if (sdk->post_read) {
        if ((sdk->post_read)(slice->slice_context, address, value) != CR_OK) {
            LOGS_ERROR("post register read fail, address=0x%04x", address);
        }
    }

    if (ret != CR_OK) {
        LOGS_ERROR("register read fail, address=0x%04x", address);
    } else if ((cr_logger_get_feature(CR_LOG_FEAT_REG) & CR_LOG_IO_READ) == CR_LOG_IO_READ) {
        const char* addr_str = hal_addr_stringify(slice, address);
        LOGS_DEBUG("[REG Read] addr: 0x%04X%s, data: 0x%04X", address, addr_str ? addr_str : "", *value);
    }

    SLICE_UNLOCK(slice);
    return ret;
}

CredoError_t cr_slice_write(CredoSlice_t* slice, unsigned address, unsigned value) {
    if (slice == NULL) return CR_INVALID_ARGS;
    SLICE_LOCK_GUARD(slice);

    CredoSdk_t* sdk = slice->sdk;
    CredoError_t ret = (sdk->write_register)(slice->slice_context, address, value);

    if (sdk->post_write) {
        if ((sdk->post_write)(slice->slice_context, address, value) != CR_OK) {
            LOGS_ERROR("post register write fail, address=0x%04x, value=0x%04x", address, value);
        }
    }

    if (ret != CR_OK) {
        LOGS_ERROR("register write fail, address=0x%04x, value=0x%04x", address, value);
    } else if ((cr_logger_get_feature(CR_LOG_FEAT_REG) & CR_LOG_IO_WRITE) == CR_LOG_IO_WRITE) {
        const char* addr_str = hal_addr_stringify(slice, address);
        LOGS_DEBUG("[REG Write] addr: 0x%04X%s, data: 0x%04X", address, addr_str ? addr_str : "", value);
    }
    SLICE_UNLOCK(slice);
    return ret;
}

CredoError_t cr_slice_burst_read(CredoSlice_t* slice, unsigned first_address, unsigned value[], unsigned count) {
    if (slice == NULL) return CR_INVALID_ARGS;
    SLICE_LOCK_GUARD(slice);

    CredoSdk_t* sdk = slice->sdk;
    CredoError_t ret = CR_OK;
    if (sdk->burst_read_register) {
        ret = (sdk->burst_read_register)(slice->slice_context, first_address, value, count);
    } else {
        /* simply read one by one */
        unsigned i;
        for (i = 0; i < count; i++) {
            ret = (sdk->read_register)(slice->slice_context, first_address + i, value + i);
            if (ret != CR_OK) break;
        }
    }
    if (ret != CR_OK) {
        LOGS_ERROR("register burst read fail, first address=0x%04x, count=%u", first_address, count);
    }
    SLICE_UNLOCK(slice);
    return ret;
}

CredoError_t cr_slice_burst_write(CredoSlice_t* slice, unsigned first_address, const unsigned value[], unsigned count) {
    if (slice == NULL) return CR_INVALID_ARGS;

    SLICE_LOCK_GUARD(slice);

    CredoSdk_t* sdk = slice->sdk;
    CredoError_t ret = CR_OK;
    if (sdk->burst_write_register) {
        ret = (sdk->burst_write_register)(slice->slice_context, first_address, value, count);
    } else {
        /* simply write one by one */
        unsigned i;
        for (i = 0; i < count; i++) {
            ret = (sdk->write_register)(slice->slice_context, first_address + i, value[i]);
            if (ret != CR_OK) break;
        }
    }
    if (ret != CR_OK) {
        LOGS_ERROR("register burst write fail, first address=0x%04x, count=%u", first_address, count);
    }
    SLICE_UNLOCK(slice);
    return ret;
}

CredoError_t cr_slice_broadcast_burst_write(CredoSlice_t* slice, unsigned first_address, const unsigned value[],
                                            unsigned count) {
    if (slice == NULL) return CR_INVALID_ARGS;

    SLICE_LOCK_GUARD(slice);

    CredoSdk_t* sdk = slice->sdk;
    CredoError_t ret = CR_OK;
    if (sdk->burst_broadcast_write_register) {
        ret = (sdk->burst_broadcast_write_register)(slice->slice_context, first_address, value, count);
    } else {
        // ensure broadcast write exists otherwise we can't even call this function
        if (sdk->broadcast_write_register == NULL) return CR_NOTIMPLEMENTED;
        /* simply write one by one */
        unsigned i;
        for (i = 0; i < count; i++) {
            ret = (sdk->broadcast_write_register)(slice->slice_context, first_address + i, value[i]);
            if (ret != CR_OK) break;
        }
    }
    if (ret != CR_OK) {
        LOGS_ERROR("register burst write fail, first address=0x%04x, count=%u", first_address, count);
    }
    SLICE_UNLOCK(slice);
    return ret;
}

CredoError_t cr_slice_broadcast_write(CredoSlice_t* slice, unsigned address, unsigned value) {
    if (slice == NULL) return CR_INVALID_ARGS;
    SLICE_LOCK_GUARD(slice);

    CredoSdk_t* sdk = slice->sdk;
    if (sdk->broadcast_write_register == NULL) return CR_NOTIMPLEMENTED;
    CredoError_t ret = (sdk->broadcast_write_register)(slice->slice_context, address, value);
    if (ret != CR_OK) {
        LOGS_ERROR("register write fail, address=0x%04x, value=0x%04x", address, value);
    }
    SLICE_UNLOCK(slice);
    return ret;
}

/* Register partial write */
CredoError_t cr_write_reg(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned offset, unsigned mask, int lsb,
                          unsigned value) {
    unsigned old_val;
    unsigned address;
    address = cr_addr_reg(slice, lane, hive, offset);
    if (mask != 0xFFFF) {
        /* Partial register write, read old one first */
        ERR_PROPS(cr_slice_read(slice, address, &old_val));
        old_val &= ~(mask << lsb);
        old_val |= (value & mask) << lsb;
    } else {
        /* Full register write, skip read */
        old_val = value;
    }
    return cr_slice_write(slice, address, old_val);
}

static unsigned addr_reg(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned offset) {
    unsigned base_addr = 0;
    if (hive->shift == BASE_ADDRESS_EXTENDED) {
        base_addr = hive->base_addr[lane];
    } else {
        base_addr = hive->base_addr[0];
        if (lane < hive->count) base_addr += (lane << hive->shift);
    }

    // if hal_get_base_address is defined, base_addr may be replaced
    hal_get_base_address(slice, hive, &base_addr);

    return (base_addr + offset);
}

unsigned cr_addr_reg(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned int offset) {
    // if support multiple device, let hal to select which hive to use
    unsigned hive_index = 0;
    hal_get_hive_index(slice, &hive_index);
    hive += hive_index;
    return addr_reg(slice, lane, hive, offset);
}

unsigned cri_slice_get_addr(CredoSlice_t* slice, int lane, const CrIntlRegHive_t* reghive, unsigned int offset) {
    const RegHive_t* hive = (const RegHive_t*)reghive->handler;
    // no need to get hive_index because got already from `hal_slice_get_reghive()` and `hal_slice_index_reghive()`
    return addr_reg(slice, lane, hive, offset);
}

CredoError_t cr_read_reg(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned offset, unsigned mask, int lsb,
                         unsigned* value) {
    unsigned address;
    address = cr_addr_reg(slice, lane, hive, offset);
    ERR_PROPS(cr_slice_read(slice, address, value));
    *value >>= lsb;
    *value &= mask;
    return CR_OK;
}

CredoError_t cr_read_reg_signed(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned offset, unsigned mask,
                                int msb, int lsb, int* value) {
    unsigned value_int;
    unsigned address;
    address = cr_addr_reg(slice, lane, hive, offset);
    ERR_PROPS(cr_slice_read(slice, address, &value_int));
    *value = value_int >> lsb;
    *value &= mask;
    *value <<= sizeof(unsigned) * 8 - msb - 1 + lsb;
    *value >>= sizeof(unsigned) * 8 - msb - 1 + lsb;
    return CR_OK;
}

/* TCM */
CredoError_t cr_read_tcm_reg(CredoSlice_t* slice, const RegHive_t* hive, unsigned offset, unsigned mask, int lsb,
                             unsigned* value) {
    unsigned tcmAddr = hive->base_addr[0];

    // if hal_tcm_get_base_address is defined, tcmAddr may be replaced
    hal_tcm_get_base_address(slice, hive, &tcmAddr);

    tcmAddr += offset;

    ERR_PROPS(hal_tcm_read(slice, tcmAddr, value));
    *value = *value >> lsb;
    *value &= mask;

    return CR_OK;
}

CredoError_t cr_write_tcm_reg(CredoSlice_t* slice, const RegHive_t* hive, unsigned offset, unsigned mask, int lsb,
                              unsigned value) {
    unsigned tcmAddr = hive->base_addr[0];

    // if hal_tcm_get_base_address is defined, tcmAddr may be replaced
    hal_tcm_get_base_address(slice, hive, &tcmAddr);

    tcmAddr += offset;

    /* partial TCM register write, read old one */
    unsigned int old_val;
    ERR_PROPS(hal_tcm_read(slice, tcmAddr, &old_val));
    old_val &= ~(mask << lsb);
    old_val |= (value & mask) << lsb;

    ERR_PROPS(hal_tcm_write(slice, tcmAddr, old_val));
    return CR_OK;
}

CredoError_t cri_slice_get_reghive(CredoSlice_t* slice, const char* hivename, CrIntlRegHive_t* reghive) {
    const RegHive_t* hive = NULL;
    SLICE_LOCK_GUARD(slice);
    CredoError_t err = hal_slice_get_reghive(slice, hivename, &hive);
    if (err != CR_OK) {
        SLICE_UNLOCK(slice);
        return err;
    }
    if (reghive && hive) {
        reghive->hivename = hive->hivename;
        reghive->base_addr = *hive->base_addr;
        reghive->base_shift = hive->base_shift;
        reghive->shift = hive->shift;
        reghive->count = hive->count;
        reghive->handler = (const void*)hive;
    }
    SLICE_UNLOCK(slice);
    return CR_OK;
}

CredoError_t cri_slice_index_reghive(CredoSlice_t* slice, int index, CrIntlRegHive_t* reghive) {
    const RegHive_t* hive = NULL;
    SLICE_LOCK_GUARD(slice);
    CredoError_t err = hal_slice_index_reghive(slice, index, &hive);
    if (err != CR_OK) {
        SLICE_UNLOCK(slice);
        return err;
    }
    if (reghive && hive) {
        reghive->hivename = hive->hivename;
        reghive->base_addr = *hive->base_addr;
        reghive->base_shift = hive->base_shift;
        reghive->shift = hive->shift;
        reghive->count = hive->count;
        reghive->handler = (const void*)hive;
    }
    SLICE_UNLOCK(slice);
    return CR_OK;
}

CredoError_t cri_slice_get_reghive_count(CredoSlice_t* slice, unsigned* count) {
    CALL_HAL(slice, hal_slice_get_reghive_count(slice, count));
}
