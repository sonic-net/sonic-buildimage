#ifndef __PHYMOD_DUMP_REG_H__
#define __PHYMOD_DUMP_REG_H__

#ifdef PHYMOD_APERTA_SUPPORT
void plp_aperta_phymod_dump_display_debug_regs(char *templatefile, char *dumpfile, char *dictfile, char *die_selection, char *verbosity, char *opfile);
void plp_aperta_phymod_dump_analyse_dumpfile (char *dumpfile, char *die_selection, char *** lpm_lookup, char *** spm_lookup,
                       char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup);
void plp_aperta_phymod_dump_print_chip_config (FILE *ip_file, char *curr_line, char *** others_lookup, int others_lookup_noofrows, FILE *op_file);
void plp_aperta_phymod_dump_print_port_snapshot (FILE *ip_file, char *** others_lookup, int others_lookup_noofrows, FILE *op_file);
void plp_aperta_phymod_dump_print_pmd_snapshot (FILE *ip_file, char *** others_lookup, int others_lookup_noofrows, 
                         char *** lpm_lookup, int lpm_lookup_noofrows, 
                         char *** spm_lookup, int spm_lookup_noofrows, FILE *op_file, int l2_display);
void plp_aperta_phymod_dump_print_serdes_info (FILE *ip_file, FILE *op_file, char *** others_lookup, int others_lookup_noofrows);
void plp_aperta_phymod_dump_print_core_info (FILE *op_file, char *** pm_lookup, int pm_lookup_noofrows, char *** others_lookup, int others_lookup_noofrows);
void plp_aperta_phymod_dump_parse_display_templatefile (char *die_selection, char *templatefile, char *dictfile, char *dumpfile, FILE *op_file,
                                 char *** lpm_lookup, char *** spm_lookup, 
                                 char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup,
                                 int lpm_lookup_noofrows, int spm_lookup_noofrows,
                                 int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows, int l2_display);
void plp_aperta_phymod_dump_update_database (FILE *dump_file, char *die, char *** lpm_lookup, char *** spm_lookup, 
                      char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup);
void phymod_dump_parse_dumpfile_line(char *dumpfile_line, FILE *die_A_file, FILE *die_B_file);
void plp_aperta_phymod_dump_get_disp_reg_lines (char *die_selection, FILE *ip_file, char *curr_line, char *dictfile, char *dumpfile, char *pattern, FILE *op_file, 
                         char *** lpm_lookup, char *** spm_lookup,
                         char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup, 
                         int lpm_lookup_noofrows, int spm_lookup_noofrows,
                         int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows, int l2_display);
void plp_aperta_phymod_dump_extract_reg_name_addr (char *ip_line, char *dictfile, int pm_line, char *pm_string, int common_reg, char disp_reg_name_ptr[], 
                            char disp_reg_addr_ptr[][100], int *noofregentries, int *bit_pos, int *noofbits);
void plp_aperta_phymod_dump_compute_cdmib_addr (char *display_regs_addr, char *port_index, char *cdmib_regs_addr);
int plp_aperta_analyse_reg_type (char *disp_reg_name_ptr, char *disp_reg_addr_ptr, char display_regs_addr[][100], char *pm_string, int common_reg);
char* plp_aperta_get_disp_reg_name (char *ip_line);
void plp_aperta_phymod_dump_get_disp_reg_addr (char *ip_line, char *dictfile, char disp_reg_addr_ptr[][100], int *noofregentries, int *bit_pos, int *noofbits);
void plp_aperta_phymod_dump_analyse_dict_file_line (char * ip_line, char disp_reg_addr_ptr[][100], int *noofregentries, int *bit_pos, int *noofbits);
int plp_aperta_pattern_count (char pattern, char *ip_line);
long long unsigned int plp_aperta_get_disp_entry_val (char *base_addr, char *** lookup, int lookup_noofrows, int bit_pos, int noofbits, int *entry_found);
void plp_aperta_phymod_dump_rmv_strtng_whitespace (char *ip_line);
void plp_aperta_phymod_dump_rmv_pattern (char *ip_line, char pattern);
void plp_aperta_phymod_dump_rmv_comment_inline (char *ip_line);
int plp_aperta_check_l2_line (char *ip_line);
void plp_aperta_phymod_dump_extract_line_comment (char *ip_line, int *comment_position, char *line_comment);
void plp_aperta_phymod_dump_rplc_multiplespace_with_singlespace (char *ip_line);
void plp_aperta_phymod_dump_rmv_dieinfo (char *ip_line);
char plp_aperta_rmv_portinfo (char *ip_line);
char plp_aperta_rmv_sideinfo (char *ip_line);
void plp_aperta_phymod_dump_rmv_colon (char *ip_line);
void plp_aperta_phymod_dump_extract_entry_vals (char *ip_line, char port_info, char side_info, char *** entry_vals, int *noofreg_entries, int *pm_cdmib_entry);
void plp_aperta_phymod_dump_get_blk_addr (char *ip_line, char *blk_addr);
void plp_aperta_phymod_dump_init_lookup (char *** lookup, int lookup_noofrows, int lookup_noofcols);
void plp_aperta_phymod_dump_init_die_lookups (char *** lpm_lookup, char *** spm_lookup,
                       char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup,
                       int lpm_lookup_noofrows, int spm_lookup_noofrows,
                       int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows);
void plp_aperta_phymod_dump_print_die_lookups (FILE *op_file, char *** lpm_lookup, char *** spm_lookup,
                        char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup,
                        int lpm_lookup_noofrows, int spm_lookup_noofrows,
                        int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows);
void plp_aperta_phymod_dump_free_die_lookups (char *** lpm_lookup, char *** spm_lookup,
                       char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup,
                       int lpm_lookup_noofrows, int spm_lookup_noofrows,
                       int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows);
void plp_aperta_phymod_dump_print_lookup (char *** lookup, int lookup_noofrows, FILE *op_file);
void plp_aperta_phymod_dump_add_lookup (char *** lookup, int lookup_index, char *addr, char *reg_val);
void plp_aperta_phymod_dump_free_lookup (char *** lookup, int lookup_noofrows, int lookup_noofcols);

#endif /* PHYMOD_APERTA_SUPPORT */
#endif /* __PHYMOD_DUMP_REG_H__ */
