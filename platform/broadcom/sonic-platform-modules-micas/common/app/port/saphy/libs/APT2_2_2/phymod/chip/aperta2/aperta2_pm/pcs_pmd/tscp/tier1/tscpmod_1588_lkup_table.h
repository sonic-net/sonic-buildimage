#include <phymod/phymod.h>

#define TSCPMOD_TS_DEFAULT_TABLE_SIZE       40
#define TSCPMOD_TS_TABLE_SIZE               80
#define TSCPMOD_TS_ENTRY_SIZE               3
#define TSCPMOD_TS_PSLL_BASED_ENTRY_SIZE    4
#define TSCPMOD_TS_TX_MPP_MEM_SIZE          160
#define TSCPMOD_TS_TX_AUX_MEM_SIZE          80
#define TSCPMOD_TS_RX_MPP_MEM_SIZE          240
typedef enum {
    TSCPMOD_SPEED_10G_IEEE_KR1,
    TSCPMOD_SPEED_25G_IEEE_KS1_CS1,
    TSCPMOD_SPEED_25G_IEEE_KR1_CR1,
    TSCPMOD_SPEED_50G_IEEE_KR1_CR1,
    TSCPMOD_SPEED_50G_IEEE_NRZ_KR1_CR1,
    TSCPMOD_SPEED_100G_IEEE_KR4,
    TSCPMOD_SPEED_100G_IEEE_CR4,
    TSCPMOD_SPEED_100G_IEEE_KR2_CR2,
    TSCPMOD_SPEED_100G_IEEE_NRZ_KR2_CR2,
    TSCPMOD_SPEED_100G_IEEE_KR1_CR1_OPT,
    TSCPMOD_SPEED_100G_IEEE_KR1_CR1,
    TSCPMOD_SPEED_200G_IEEE_KR4_CR4,
    TSCPMOD_SPEED_200G_IEEE_NRZ_KR4_CR4,
    TSCPMOD_SPEED_200G_IEEE_KR2_CR2,
    TSCPMOD_SPEED_400G_IEEE_KR4_CR4,
    TSCPMOD_SPEED_25G_BRCM_CR1,
    TSCPMOD_SPEED_25G_BRCM_KR1,
    TSCPMOD_SPEED_25G_BRCM_NO_FEC_KR1_CR1,
    TSCPMOD_SPEED_25G_BRCM_FEC_528_KR1_CR1,
    TSCPMOD_SPEED_50G_BRCM_CR2_KR2_NO_FEC,
    TSCPMOD_SPEED_50G_BRCM_CR2_KR2_RS_FEC,
    TSCPMOD_SPEED_50G_BRCM_FEC_544_CR2_KR2,
    TSCPMOD_SPEED_50G_BRCM_FEC_528_CR1_KR1,
    TSCPMOD_SPEED_50G_BRCM_FEC_272_CR1_KR1,
    TSCPMOD_SPEED_50G_BRCM_NO_FEC_NRZ_CR1_KR1,
    TSCPMOD_SPEED_50G_BRCM_FEC_528_NRZ_CR1_KR1,
    TSCPMOD_SPEED_50G_BRCM_FEC_272_NRZ_CR1_KR1,
    TSCPMOD_SPEED_100G_BRCM_NO_FEC_X4,
    TSCPMOD_SPEED_100G_BRCM_NO_FEC_KR2_CR2,
    TSCPMOD_SPEED_100G_BRCM_FEC_528_KR2_CR2,
    TSCPMOD_SPEED_100G_BRCM_FEC_272_CR2_KR2,
    TSCPMOD_SPEED_100G_BRCM_NO_FEC_NRZ_KR2_CR2,
    TSCPMOD_SPEED_100G_BRCM_FEC_528_NRZ_KR2_CR2,
    TSCPMOD_SPEED_100G_BRCM_FEC_272_NRZ_KR2_CR2,
    TSCPMOD_SPEED_100G_BRCM_KR1_CR1,
    TSCPMOD_SPEED_100G_BRCM_FEC_272_KR1_CR1,
    TSCPMOD_SPEED_200G_BRCM_NO_FEC_KR4_CR4,
    TSCPMOD_SPEED_200G_BRCM_KR4_CR4,
    TSCPMOD_SPEED_200G_BRCM_FEC_272_CR4_KR4,
    TSCPMOD_SPEED_200G_BRCM_FEC_272_N4,
    TSCPMOD_SPEED_200G_BRCM_NO_FEC_NRZ_KR4_CR4,
    TSCPMOD_SPEED_200G_BRCM_FEC_272_NRZ_CR4_KR4,
    TSCPMOD_SPEED_200G_BRCM_FEC_272_NRZ_N4,
    TSCPMOD_SPEED_200G_BRCM_NRZ_KR4_CR4,
    TSCPMOD_SPEED_200G_BRCM_KR2_CR2,
    TSCPMOD_SPEED_200G_BRCM_FEC_272_KR2_CR2,
    TSCPMOD_SPEED_200G_BRCM_FEC_272_N2,
    TSCPMOD_SPEED_400G_BRCM_FEC_KR8_CR8,
    TSCPMOD_SPEED_400G_BRCM_FEC_272_N8,
    TSCPMOD_SPEED_400G_BRCM_NO_FEC_NRZ_KR8_CR8,
    TSCPMOD_SPEED_400G_BRCM_FEC_NRZ_KR8_CR8,
    TSCPMOD_SPEED_400G_BRCM_FEC_272_NRZ_N8,
    TSCPMOD_SPEED_400G_BRCM_FEC_272_KR4_CR4,
    TSCPMOD_SPEED_800G_BRCM_KR8_CR8,
    TSCPMOD_SPEED_MODE_COUNT
} tscpmod_1588_table_index_t;

typedef struct tscpmod_1588_lkup_table_entry_s {
    /* Index for the default speed entry. */
    uint32_t mapped_spd_table_index;
    phymod_fec_type_t fec_type;
    /* Index for 1588 table. */
    tscpmod_1588_table_index_t ts_table_index;
    uint32_t table_size;
} tscpmod_1588_lkup_table_entry_t;

typedef uint32_t tscpmod_ts_table_entry[TSCPMOD_TS_TABLE_SIZE][TSCPMOD_TS_ENTRY_SIZE];
typedef uint32_t tscpmod_ts_table_entry_mem[TSCPMOD_TS_ENTRY_SIZE];

int plp_aperta2_tscpmod_1588_table_index_get(uint32_t speed_index, int *ts_table_index, uint32_t *table_size);
int plp_aperta2_tscpmod_rx_tbl_entry_to_psll_entry_map(uint32_t *tbl_entry, uint32_t *psll_entry);
int plp_aperta2_tscpmod_psll_entry_to_rx_tbl_entry_map(uint32_t *psll_entry, uint32_t *tbl_entry);

extern tscpmod_ts_table_entry* plp_aperta2_tscpmod_ts_table_tx_sop_get(int is_cx);
extern tscpmod_ts_table_entry* plp_aperta2_tscpmod_ts_table_rx_sop_get(int is_cx);
extern tscpmod_ts_table_entry* plp_aperta2_tscpmod_ts_table_tx_sfd_get(int is_cx);
extern tscpmod_ts_table_entry* plp_aperta2_tscpmod_ts_table_rx_sfd_get(int is_cx);
