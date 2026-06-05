#include "common/common_efuse.h"

#include "utility.h"
#include "sdk.h"

#ifdef EFUSE_BASE_REG

CredoError_t common_efuse_read_bank(CredoSlice_t *slice, int bank, uint32_t *value) {
    if (bank > 16) {
        return CR_INVALID_ARGS;
    }
    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xF9, 0xAAAA));
    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFF, 0x0));
    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFE, 0x0));
    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFD, 0x103));

    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFB, bank));
    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFC, 0x1));
    // sleep_ms(1);
    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFC, 0x0));
    // sleep_ms(3);

    unsigned data_h = 0, data_l = 0;
    ERR_PROPS(cr_slice_read(slice, EFUSE_BASE_REG + 0xFA, &data_h));
    ERR_PROPS(cr_slice_read(slice, EFUSE_BASE_REG + 0xF7, &data_l));

    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFD, 0x104));
    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFE, 0x0));
    ERR_PROPS(cr_slice_write(slice, EFUSE_BASE_REG + 0xFF, 0x1));

    *value = (data_h << 16) | data_l;

    return CR_OK;
}

CredoError_t common_efuse_read_ecid(CredoSlice_t *slice, uint32_t ecid[2]) {
    if (!slice->data->ecid_captured) {
        ERR_PROPS(common_efuse_read_bank(slice, 1, &ecid[0]));
        ERR_PROPS(common_efuse_read_bank(slice, 2, &ecid[1]));
        slice->data->ecid_raw[0] = ecid[0];
        slice->data->ecid_raw[1] = ecid[1];
        slice->data->ecid_captured = true;
    } else {
        ecid[0] = slice->data->ecid_raw[0];
        ecid[1] = slice->data->ecid_raw[1];
    }
    return CR_OK;
}

#endif
