#include <phymod/phymod.h>

#define TBHMOD_TS_MPP_MEM_SIZE 160
#define APERTA_TBHMOD_TS_DEFAULT_TABLE_SIZE 40
#define APERTA_TBHMOD_TS_TABLE_SIZE  60
#define TBHMOD_TS_MPP_MEM_TABLE_CNT 4
#define TBHMOD_TS_ENTRY_SIZE  3
#define APERTA_TBHMOD_TS_PSLL_BASED_ENTRY_SIZE  4

typedef enum {
    APERTA_TBHMOD_SPEED_10G_IEEE_KR1 = 0,
    APERTA_TBHMOD_SPEED_10G_IEEE_KR1_CL74,
    APERTA_TBHMOD_SPEED_25G_IEEE_KS1_CS1,
    APERTA_TBHMOD_SPEED_25G_IEEE_KS1_CS1_CL74,
    APERTA_TBHMOD_SPEED_25G_IEEE_KR1_CR1,
    APERTA_TBHMOD_SPEED_25G_IEEE_KR1_CR1_CL74,
    APERTA_TBHMOD_SPEED_25G_IEEE_KR1_CR1_RS528,
    APERTA_TBHMOD_SPEED_40G_IEEE_KR4,
    APERTA_TBHMOD_SPEED_40G_IEEE_KR4_CL74,
    APERTA_TBHMOD_SPEED_40G_IEEE_CR4,
    APERTA_TBHMOD_SPEED_40G_IEEE_CR4_CL74,
    APERTA_TBHMOD_SPEED_50G_IEEE_KR1_CR1,
    APERTA_TBHMOD_SPEED_100G_IEEE_KR4,
    APERTA_TBHMOD_SPEED_100G_IEEE_CR4,
    APERTA_TBHMOD_SPEED_100G_IEEE_KR2_CR2,
    APERTA_TBHMOD_SPEED_200G_IEEE_KR4_CR4,
    APERTA_TBHMOD_SPEED_400G_IEEE_X8,
    APERTA_TBHMOD_SPEED_20G_BRCM_KR1,
    APERTA_TBHMOD_SPEED_20G_BRCM_KR1_CL74,
    APERTA_TBHMOD_SPEED_20G_BRCM_CR1,
    APERTA_TBHMOD_SPEED_20G_BRCM_CR1_CL74,
    APERTA_TBHMOD_SPEED_25G_BRCM_KR1,
    APERTA_TBHMOD_SPEED_25G_BRCM_KR1_CL74,
    APERTA_TBHMOD_SPEED_25G_BRCM_KR1_RS528,
    APERTA_TBHMOD_SPEED_25G_BRCM_CR1,
    APERTA_TBHMOD_SPEED_25G_BRCM_CR1_CL74,
    APERTA_TBHMOD_SPEED_25G_BRCM_CR1_RS528,
    APERTA_TBHMOD_SPEED_40G_BRCM_KR2,
    APERTA_TBHMOD_SPEED_40G_BRCM_CR2,
    APERTA_TBHMOD_SPEED_50G_BRCM_CR2_KR2_NO_FEC,
    APERTA_TBHMOD_SPEED_50G_BRCM_CR2_KR2_RS_FEC,
    APERTA_TBHMOD_SPEED_50G_BRCM_FEC_544_CR2_KR2,
    APERTA_TBHMOD_SPEED_50G_BRCM_NO_FEC_CR1_KR1,
    APERTA_TBHMOD_SPEED_50G_BRCM_FEC_528_CR1_KR1,
    APERTA_TBHMOD_SPEED_50G_BRCM_FEC_272_CR1_KR1,
    APERTA_TBHMOD_SPEED_100G_BRCM_NO_FEC_X4,
    APERTA_TBHMOD_SPEED_100G_BRCM_KR4_CR4,
    APERTA_TBHMOD_SPEED_100G_BRCM_NO_FEC_KR2_CR2,
    APERTA_TBHMOD_SPEED_100G_BRCM_FEC_528_KR2_CR2,
    APERTA_TBHMOD_SPEED_100G_BRCM_FEC_272_CR2_KR2,
    APERTA_TBHMOD_SPEED_200G_BRCM_NO_FEC_KR4_CR4,
    APERTA_TBHMOD_SPEED_200G_BRCM_KR4_CR4,
    APERTA_TBHMOD_SPEED_200G_BRCM_FEC_272_N4,
    APERTA_TBHMOD_SPEED_400G_BRCM_FEC_KR8_CR8,
    APERTA_TBHMOD_SPEED_MODE_COUNT
} tbhmod_1588_table_index_t;

typedef struct tbhmod_1588_lkup_table_entry_s {
    uint32_t mapped_spd_table_index; /* The index for the default speed entry */
    phymod_fec_type_t fec_type;
    tbhmod_1588_table_index_t ts_table_index; /* The index of the 1588 table */
    uint32_t table_size;
} tbhmod_1588_lkup_table_entry_t;

typedef uint32_t aperta_ts_table_entry[APERTA_TBHMOD_TS_TABLE_SIZE][TBHMOD_TS_ENTRY_SIZE];

int plp_aperta_tbhmod_1588_table_index_get(uint32_t speed_index, phymod_fec_type_t fec, int *ts_table_index, uint32_t* table_size);
int plp_aperta_tbhmod_tbl_entry_to_psll_entry_map(uint32_t* tbl_entry, uint32_t* psll_entry);
int plp_aperta_tbhmod_psll_entry_to_tbl_entry_map(uint32_t* psll_entry, uint32_t* tbl_entry);
int tbhmod_ts_tx_table_get(int index, int is_sfd, aperta_ts_table_entry** tbl);
int tbhmod_ts_rx_table_get(int index, int is_sfd, aperta_ts_table_entry** tbl);
extern aperta_ts_table_entry* plp_aperta_ts_table_tx_sop_get(void);
extern aperta_ts_table_entry* plp_aperta_ts_table_rx_sop_get(void);
extern aperta_ts_table_entry* plp_aperta_ts_table_tx_sfd_get(void);
extern aperta_ts_table_entry* plp_aperta_ts_table_rx_sfd_get(void);
