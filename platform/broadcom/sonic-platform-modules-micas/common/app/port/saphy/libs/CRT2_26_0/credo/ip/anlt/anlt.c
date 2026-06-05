#include "project.h"

#include "anlt/anlt.h"

#include "fort.h"
#include "sbs.h"
#include "stringify.h"
#include "utility.h"

#include <stdlib.h>
#include <string.h>

#if HAL_SUPPORT_ANLT
static const char* ieee_speed_cap[20] = {
    "1G-KX",        "10G-KX4",      "10G-KR",       "40G-KR4",      "40G-CR4",      "100G-CR10",   "100G-KP4",
    "100G-KR4",     "100G-CR4",     "25G-KR/CR",    "25G-KR/CR-S",  "2.5G-KX",      "5G-KR",       "50G-KR/CR",
    "100G-KR2/CR2", "200G-KR4/CR4", "100G-KR1/CR1", "200G-KR2/CR2", "400G-KR4/CR4", "800G-KR8/CR8"};
static CredoError_t common_anlt_speed_capability_str(uint64_t bp_page, uint64_t np2, char* speed_cap_str,
                                                     unsigned size) {
    unsigned ieee_ability_bits = bp_page >> 21;
    sbs* b = SBSFROMBUF("", speed_cap_str, size);
    for (unsigned i = 0; i < COUNT_OF(ieee_speed_cap); i++) {
        if ((ieee_ability_bits >> i) & 0x1) {
            sbscatprintf(b, "%s,", ieee_speed_cap[i]);
        }
    }
    if ((np2 >> 20) & 0x1) {
        sbscat(b, "25G-KR1,");
    }
    if ((np2 >> 21) & 0x1) {
        sbscat(b, "25G-CR1,");
    }
    if ((np2 >> 24) & 0x1) {
        sbscat(b, "50G-KR2,");
    }
    if ((np2 >> 25) & 0x1) {
        sbscat(b, "50G-CR2,");
    }
    if ((np2 >> 31) & 0x1) {
        sbscat(b, "800G-ETC-KR8/CR8,");
    }
    if ((np2 >> 34) & 0x1) {
        sbscat(b, "400G-KR8/CR8,");
    }
    sbsrange(b, 0, -2);
    return CR_OK;
}
static const char* ieee_fec_str[32] = {"100G RS-FEC interleaved", "25G RS-FEC", "25G BASE-R FEC", "10G FEC capable",
                                       "10G FEC req"};

static CredoError_t common_anlt_fec_str(uint64_t bp_page, char* fec_str, unsigned size) {
    unsigned bp_fec_bits = bp_page >> 43;

    sbs* b = SBSFROMBUF("", fec_str, size);
    for (unsigned i = 0; i < COUNT_OF(ieee_fec_str); i++) {
        if ((bp_fec_bits >> i) & 0x1) {
            sbscatprintf(b, "%s,", ieee_fec_str[i]);
        }
    }
    sbsrange(b, 0, -2);
    return CR_OK;
}

CredoError_t common_set_autoneg_pages(CredoSlice_t* slice, int lane, int page_id, uint64_t page) {
    unsigned address;
    switch (page_id) {
        case 0:
            address = addrRegLane(slice, lane, REG_AUTONEG_BP_15_0);
            break;
        case 1:
            address = addrRegLane(slice, lane, REG_AUTONEG_NP1_15_0);
            break;
        case 2:
            address = addrRegLane(slice, lane, REG_AUTONEG_NP2_15_0);
            break;
        default:
            return CR_UNSUPPORTED;
    }

    unsigned p15_0 = page & 0xffff;
    unsigned p31_16 = (page >> 16) & 0xffff;
    unsigned p47_32 = (page >> 32) & 0xffff;
    ERR_PROPS(writeTop(slice, address + 1, p31_16));
    ERR_PROPS(writeTop(slice, address + 2, p47_32));
    ERR_PROPS(writeTop(slice, address, p15_0));
    return writeRegLane(slice, lane, REG_AUTONEG_IEEE_MODE, page_id == 0 ? 1 : 0);
}

CredoError_t common_get_autoneg_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                                uint64_t transmitted_pages[9], uint64_t received_pages[9]) {
    unsigned page, i, address, p[3];
    uint64_t one_page;
    /* Read page_received twice, this is sticky high */
    ERR_PROPS(readRegLane(slice, lane, REG_AUTONEG_PAGE_RECEIVED, &page));
    ERR_PROPS(readRegLane(slice, lane, REG_AUTONEG_PAGE_RECEIVED, &page));
    if (page == 0) {
        *page_count = 0;
        return CR_OK;
    }
    ERR_PROPS(readRegLane(slice, lane, REG_AUTONEG_PAGE_NUMBER, &page));
    page += 1;
    *page_count = page;
    address = addrRegLane(slice, lane, REG_AUTONEG_TX_PAGE0_15_0);
    for (i = 0; i < page; i++) {
        ERR_PROPS(readTop(slice, address, p + 0));
        ERR_PROPS(readTop(slice, address + 1, p + 1));
        ERR_PROPS(readTop(slice, address + 2, p + 2));
        address += 3;
        one_page = ((uint64_t)p[2] << 32) | ((uint64_t)p[1] << 16) | p[0];
        transmitted_pages[i] = one_page;
    }
    address = addrRegLane(slice, lane, REG_AUTONEG_RX_PAGE0_15_0);
    for (i = 0; i < page; i++) {
        ERR_PROPS(readTop(slice, address, p + 0));
        ERR_PROPS(readTop(slice, address + 1, p + 1));
        ERR_PROPS(readTop(slice, address + 2, p + 2));
        address += 3;
        one_page = ((uint64_t)p[2] << 32) | ((uint64_t)p[1] << 16) | p[0];
        received_pages[i] = one_page;
    }
    return CR_OK;
}

CredoError_t common_dump_anlt_pages(CredoSlice_t* slice, int* lane_list, unsigned* an_state, uint64_t tx_pages[][9],
                                    uint64_t rx_pages[][9], const DisplayState_t* D) {
    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_FAIL;
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_printf_ln(table, "|Local Device|||Link Partner||");
    ft_printf_ln(table, "Ln|BP|NP1|NP2|BP|NP1|NP2");
    ft_set_cell_span(table, 0, 1, 3);
    ft_set_cell_span(table, 0, 4, 3);
    ft_add_separator(table);

    int lane = 0;
    for (int idx = 0; idx < slice->desc->lane_count; idx++) {
        if ((lane = lane_list[idx]) >= slice->desc->lane_count) break;
        uint64_t* tx_page = tx_pages[idx];
        uint64_t* rx_page = rx_pages[idx];
        ft_printf_ln(
            table, "%s(%d)|%04X%08X|%04X%08X|%04X%08X|%04X%08X|%04X%08X|%04X%08X", stringify_lane_id(slice, lane), lane,
            (uint32_t)(tx_page[0] >> 32), (uint32_t)tx_page[0], (uint32_t)(tx_page[1] >> 32), (uint32_t)tx_page[1],
            (uint32_t)(tx_page[2] >> 32), (uint32_t)tx_page[2], (uint32_t)(rx_page[0] >> 32), (uint32_t)rx_page[0],
            (uint32_t)(rx_page[1] >> 32), (uint32_t)rx_page[1], (uint32_t)(rx_page[2] >> 32), (uint32_t)rx_page[2]);
    }

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

CredoError_t common_dump_anlt_detail(CredoSlice_t* slice, int* lane_list, unsigned* an_state, uint64_t tx_pages[][9],
                                     uint64_t rx_pages[][9], const DisplayState_t* D) {
    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_FAIL;

    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_RIGHT_PADDING, 0);
    ft_printf_ln(table, "||Local Device||||Link Partner|||");
    ft_printf_ln(table, "Ln|AN State|NP|RF|Speed Capability|FEC Capability|NP|RF|Speed Capability|FEC Capability");
    ft_set_cell_span(table, 0, 0, 2);
    ft_set_cell_span(table, 0, 2, 4);
    ft_set_cell_span(table, 0, 6, 4);
    ft_add_separator(table);

    CredoError_t ret = CR_FAIL;
    unsigned np, rf;
    char speed_cap_str[256] = {0};
    char fec_str[128] = {0};
    int lane = 0;
    for (unsigned idx = 0; idx < slice->desc->lane_count; idx++) {
        if ((lane = lane_list[idx]) >= slice->desc->lane_count) break;
        ft_set_cell_prop(table, FT_CUR_ROW, 4, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_LEFT);
        ft_set_cell_prop(table, FT_CUR_ROW, 8, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_LEFT);

        ft_printf(table, "%s(%d)|%4X", stringify_lane_id(slice, lane), lane, an_state[idx]);

        np = (tx_pages[idx][0] >> 15) & 0x1;
        rf = (tx_pages[idx][0] >> 13) & 0x1;
        ERR_CATCH(common_anlt_speed_capability_str(tx_pages[idx][0], tx_pages[idx][2], speed_cap_str, 256),
                  goto cleanup);
        ERR_CATCH(common_anlt_fec_str(tx_pages[idx][0], fec_str, 128), goto cleanup);
        ft_printf(table, "%X|%X|%s|%s", np, rf, speed_cap_str, fec_str);

        np = (rx_pages[idx][0] >> 15) & 0x1;
        rf = (rx_pages[idx][0] >> 13) & 0x1;
        ERR_CATCH(common_anlt_speed_capability_str(rx_pages[idx][0], rx_pages[idx][2], speed_cap_str, 256),
                  goto cleanup);
        ERR_CATCH(common_anlt_fec_str(rx_pages[idx][0], fec_str, 128), goto cleanup);
        ft_printf(table, "%X|%X|%s|%s", np, rf, speed_cap_str, fec_str);

        ft_ln(table);
        ft_add_separator(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    ret = CR_OK;
cleanup:
    ft_destroy_table(table);
    return ret;
}

#endif  // HAL_SUPPORT_ANLT
