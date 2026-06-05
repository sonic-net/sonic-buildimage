#include <phymod/phymod.h>
#include <phymod/phymod_util.h>

#ifdef PHYMOD_APERTA_SUPPORT
#include "phymod_dump_reg.h"


void plp_aperta_phymod_dump_display_debug_regs (char *templatefile, char *dumpfile, char *dictfile, char *die_selection, char *verbosity, char *opfile) 
{
    /* The number of rows indicated below are calculated based on number of registers */
    int const die_0_lpm_lookup_noofrows    = 2600;
    int const die_0_spm_lookup_noofrows    = 2600;
    int const die_0_lpm_cdmib_lookup_noofrows    = 2168;
    int const die_0_spm_cdmib_lookup_noofrows    = 2168;
    int const die_0_others_lookup_noofrows = 6100;
    int const die_1_lpm_lookup_noofrows    = 2600;
    int const die_1_spm_lookup_noofrows    = 2600;
    int const die_1_lpm_cdmib_lookup_noofrows    = 2168;
    int const die_1_spm_cdmib_lookup_noofrows    = 2168;
    int const die_1_others_lookup_noofrows = 6100;
    char *** die_0_lpm_lookup       = (char ***)PHYMOD_IF_MALLOC(die_0_lpm_lookup_noofrows * sizeof(char **));
    char *** die_0_spm_lookup       = (char ***)PHYMOD_IF_MALLOC(die_0_spm_lookup_noofrows * sizeof(char **));
    char *** die_0_lpm_cdmib_lookup = (char ***)PHYMOD_IF_MALLOC(die_0_lpm_cdmib_lookup_noofrows * sizeof(char **));
    char *** die_0_spm_cdmib_lookup = (char ***)PHYMOD_IF_MALLOC(die_0_spm_cdmib_lookup_noofrows * sizeof(char **));
    char *** die_0_others_lookup    = (char ***)PHYMOD_IF_MALLOC(die_0_others_lookup_noofrows * sizeof(char **));
    char *** die_1_lpm_lookup       = (char ***)PHYMOD_IF_MALLOC(die_1_lpm_lookup_noofrows * sizeof(char **));
    char *** die_1_spm_lookup       = (char ***)PHYMOD_IF_MALLOC(die_1_spm_lookup_noofrows * sizeof(char **));
    char *** die_1_lpm_cdmib_lookup = (char ***)PHYMOD_IF_MALLOC(die_1_lpm_cdmib_lookup_noofrows * sizeof(char **));
    char *** die_1_spm_cdmib_lookup = (char ***)PHYMOD_IF_MALLOC(die_1_spm_cdmib_lookup_noofrows * sizeof(char **));
    char *** die_1_others_lookup    = (char ***)PHYMOD_IF_MALLOC(die_1_others_lookup_noofrows * sizeof(char **));
    int   l2_display = 0;
    char die_A_string[2] = "A";
    char die_B_string[2] = "B";
    char die_Both_string[5] = "Both";
    PHYMOD_FILE *op_file;
    char die[2];

    PHYMOD_DIAG_OUT(("dumpfile: %s\n", dumpfile));
    PHYMOD_DIAG_OUT(("Selected Die: %s\n",die_selection));
    PHYMOD_DIAG_OUT(("opfile: %s\n", opfile));

    op_file = PHYMOD_FOPEN(opfile, "w");

    if (PHYMOD_STRCMP(verbosity, "L2") == 0) {
        l2_display = 1;
    } else {
        l2_display = 0;
    }

    if (PHYMOD_STRCMP(die_selection,die_A_string) == 0) {
        plp_aperta_phymod_dump_init_die_lookups(die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
    } else if (PHYMOD_STRCMP(die_selection,die_B_string) == 0) {
        plp_aperta_phymod_dump_init_die_lookups(die_1_lpm_lookup, die_1_spm_lookup,
                die_1_lpm_cdmib_lookup, die_1_spm_cdmib_lookup, die_1_others_lookup, 
                die_1_lpm_lookup_noofrows, die_1_spm_lookup_noofrows,
                die_1_lpm_cdmib_lookup_noofrows, die_1_spm_cdmib_lookup_noofrows, die_1_others_lookup_noofrows);
    } else if (PHYMOD_STRCMP(die_selection,die_Both_string) == 0) {
        plp_aperta_phymod_dump_init_die_lookups(die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
        plp_aperta_phymod_dump_init_die_lookups(die_1_lpm_lookup, die_1_spm_lookup,
                die_1_lpm_cdmib_lookup, die_1_spm_cdmib_lookup, die_1_others_lookup, 
                die_1_lpm_lookup_noofrows, die_1_spm_lookup_noofrows,
                die_1_lpm_cdmib_lookup_noofrows, die_1_spm_cdmib_lookup_noofrows, die_1_others_lookup_noofrows);
    } else {
        plp_aperta_phymod_dump_init_die_lookups(die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
    }
    die[1]='\0';
    if (PHYMOD_STRCMP(die_selection,die_A_string) == 0) {
        die[0] = '0';
        plp_aperta_phymod_dump_analyse_dumpfile(dumpfile, die, die_0_lpm_lookup, die_0_spm_lookup, die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup);
    } else if (PHYMOD_STRCMP(die_selection,die_B_string) == 0) {
        die[0] = '1';
        plp_aperta_phymod_dump_analyse_dumpfile(dumpfile, die, die_1_lpm_lookup, die_1_spm_lookup, die_1_lpm_cdmib_lookup, die_1_spm_cdmib_lookup, die_1_others_lookup);
    } else if (PHYMOD_STRCMP(die_selection,die_Both_string) == 0) {
        die[0] = '0';
        plp_aperta_phymod_dump_analyse_dumpfile(dumpfile, die, die_0_lpm_lookup, die_0_spm_lookup, die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup);
        die[0] = '1';
        plp_aperta_phymod_dump_analyse_dumpfile(dumpfile, die, die_1_lpm_lookup, die_1_spm_lookup, die_1_lpm_cdmib_lookup, die_1_spm_cdmib_lookup, die_1_others_lookup);
    } else {
        die[0] = '0';
        plp_aperta_phymod_dump_analyse_dumpfile(dumpfile, die, die_0_lpm_lookup, die_0_spm_lookup, die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup);
    }

    if (PHYMOD_STRCMP(die_selection,die_A_string) == 0) {
        plp_aperta_phymod_dump_print_die_lookups(op_file, die_0_lpm_lookup, die_0_spm_lookup, die_0_lpm_cdmib_lookup, 
                die_0_spm_cdmib_lookup, die_0_others_lookup,
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
    } else if (PHYMOD_STRCMP(die_selection,die_B_string) == 0) {
        plp_aperta_phymod_dump_print_die_lookups(op_file, die_0_lpm_lookup, die_0_spm_lookup, die_1_lpm_cdmib_lookup, 
                die_1_spm_cdmib_lookup, die_1_others_lookup,
                die_1_lpm_lookup_noofrows, die_1_spm_lookup_noofrows,
                die_1_lpm_cdmib_lookup_noofrows, die_1_spm_cdmib_lookup_noofrows, die_1_others_lookup_noofrows);
    } else if (PHYMOD_STRCMP(die_selection,die_Both_string) == 0) {
        plp_aperta_phymod_dump_print_die_lookups(op_file, die_0_lpm_lookup, die_0_spm_lookup, die_0_lpm_cdmib_lookup, 
                die_0_spm_cdmib_lookup, die_0_others_lookup,
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
        plp_aperta_phymod_dump_print_die_lookups(op_file, die_0_lpm_lookup, die_0_spm_lookup, die_1_lpm_cdmib_lookup, 
                die_1_spm_cdmib_lookup, die_1_others_lookup,
                die_1_lpm_lookup_noofrows, die_1_spm_lookup_noofrows,
                die_1_lpm_cdmib_lookup_noofrows, die_1_spm_cdmib_lookup_noofrows, die_1_others_lookup_noofrows);
    } else {
        plp_aperta_phymod_dump_print_die_lookups(op_file, die_0_lpm_lookup, die_0_spm_lookup, die_0_lpm_cdmib_lookup, 
                die_0_spm_cdmib_lookup, die_0_others_lookup,
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
    }

    if (PHYMOD_STRCMP(die_selection,die_A_string) == 0) {
        plp_aperta_phymod_dump_parse_display_templatefile (die_A_string, templatefile, dictfile, dumpfile, op_file,
                die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows, l2_display);
    } else if (PHYMOD_STRCMP(die_selection,die_B_string) == 0) {
        plp_aperta_phymod_dump_parse_display_templatefile (die_B_string, templatefile, dictfile, dumpfile, op_file,
                die_1_lpm_lookup, die_1_spm_lookup,
                die_1_lpm_cdmib_lookup, die_1_spm_cdmib_lookup, die_1_others_lookup, 
                die_1_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_1_lpm_cdmib_lookup_noofrows, die_1_spm_cdmib_lookup_noofrows, die_1_others_lookup_noofrows, l2_display);
    } else if (PHYMOD_STRCMP(die_selection,die_Both_string) == 0) {
        plp_aperta_phymod_dump_parse_display_templatefile (die_A_string, templatefile, dictfile, dumpfile, op_file,
                die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows, l2_display);
        plp_aperta_phymod_dump_parse_display_templatefile (die_B_string, templatefile, dictfile, dumpfile, op_file,
                die_1_lpm_lookup, die_1_spm_lookup,
                die_1_lpm_cdmib_lookup, die_1_spm_cdmib_lookup, die_1_others_lookup, 
                die_1_lpm_lookup_noofrows, die_1_spm_lookup_noofrows,
                die_1_lpm_cdmib_lookup_noofrows, die_1_spm_cdmib_lookup_noofrows, die_1_others_lookup_noofrows, l2_display);
    } else {
        plp_aperta_phymod_dump_parse_display_templatefile (die_A_string, templatefile, dictfile, dumpfile, op_file,
                die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows, l2_display);
    }

    if (PHYMOD_STRCMP(die_selection,die_A_string) == 0) {
        plp_aperta_phymod_dump_free_die_lookups(die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
        PHYMOD_IF_FREE(die_1_lpm_lookup);
        PHYMOD_IF_FREE(die_1_spm_lookup);
        PHYMOD_IF_FREE(die_1_lpm_cdmib_lookup);
        PHYMOD_IF_FREE(die_1_spm_cdmib_lookup);
        PHYMOD_IF_FREE(die_1_others_lookup);
    } else if (PHYMOD_STRCMP(die_selection,die_B_string) == 0) {
        plp_aperta_phymod_dump_free_die_lookups(die_1_lpm_lookup, die_1_spm_lookup,
                die_1_lpm_cdmib_lookup, die_1_spm_cdmib_lookup, die_1_others_lookup, 
                die_1_lpm_lookup_noofrows, die_1_spm_lookup_noofrows,
                die_1_lpm_cdmib_lookup_noofrows, die_1_spm_cdmib_lookup_noofrows, die_1_others_lookup_noofrows);
        PHYMOD_IF_FREE(die_0_lpm_lookup);
        PHYMOD_IF_FREE(die_0_spm_lookup);
        PHYMOD_IF_FREE(die_0_lpm_cdmib_lookup);
        PHYMOD_IF_FREE(die_0_spm_cdmib_lookup);
        PHYMOD_IF_FREE(die_0_others_lookup);
    } else if (PHYMOD_STRCMP(die_selection,die_Both_string) == 0) {
        plp_aperta_phymod_dump_free_die_lookups(die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
        plp_aperta_phymod_dump_free_die_lookups(die_1_lpm_lookup, die_1_spm_lookup,
                die_1_lpm_cdmib_lookup, die_1_spm_cdmib_lookup, die_1_others_lookup, 
                die_1_lpm_lookup_noofrows, die_1_spm_lookup_noofrows,
                die_1_lpm_cdmib_lookup_noofrows, die_1_spm_cdmib_lookup_noofrows, die_1_others_lookup_noofrows);
    } else {
        plp_aperta_phymod_dump_free_die_lookups(die_0_lpm_lookup, die_0_spm_lookup,
                die_0_lpm_cdmib_lookup, die_0_spm_cdmib_lookup, die_0_others_lookup, 
                die_0_lpm_lookup_noofrows, die_0_spm_lookup_noofrows,
                die_0_lpm_cdmib_lookup_noofrows, die_0_spm_cdmib_lookup_noofrows, die_0_others_lookup_noofrows);
        PHYMOD_IF_FREE(die_1_lpm_lookup);
        PHYMOD_IF_FREE(die_1_spm_lookup);
        PHYMOD_IF_FREE(die_1_lpm_cdmib_lookup);
        PHYMOD_IF_FREE(die_1_spm_cdmib_lookup);
        PHYMOD_IF_FREE(die_1_others_lookup);
    }

    PHYMOD_FCLOSE(op_file);    
    return;
}

void plp_aperta_phymod_dump_init_die_lookups (char *** lpm_lookup, char *** spm_lookup,
                       char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup,
                       int lpm_lookup_noofrows, int spm_lookup_noofrows,
                       int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows) {
    plp_aperta_phymod_dump_init_lookup(lpm_lookup, lpm_lookup_noofrows, 2);
    plp_aperta_phymod_dump_init_lookup(spm_lookup, spm_lookup_noofrows, 2);
    plp_aperta_phymod_dump_init_lookup(lpm_cdmib_lookup, lpm_cdmib_lookup_noofrows, 2);
    plp_aperta_phymod_dump_init_lookup(spm_cdmib_lookup, spm_cdmib_lookup_noofrows, 2);
    plp_aperta_phymod_dump_init_lookup(others_lookup, others_lookup_noofrows, 2);
    return;
}

void plp_aperta_phymod_dump_print_die_lookups (PHYMOD_FILE *op_file, char *** lpm_lookup, char *** spm_lookup,
                        char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup,
                        int lpm_lookup_noofrows, int spm_lookup_noofrows,
                        int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows) {

    return;
}

void plp_aperta_phymod_dump_free_die_lookups (char *** lpm_lookup, char *** spm_lookup,
                       char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup,
                       int lpm_lookup_noofrows, int spm_lookup_noofrows,
                       int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows) 
{
    plp_aperta_phymod_dump_free_lookup(lpm_lookup, lpm_lookup_noofrows, 2);
    plp_aperta_phymod_dump_free_lookup(spm_lookup, spm_lookup_noofrows, 2);
    plp_aperta_phymod_dump_free_lookup(lpm_cdmib_lookup, lpm_cdmib_lookup_noofrows, 2);
    plp_aperta_phymod_dump_free_lookup(spm_cdmib_lookup, spm_cdmib_lookup_noofrows, 2);
    plp_aperta_phymod_dump_free_lookup(others_lookup, others_lookup_noofrows, 2);
    return;
}

void plp_aperta_phymod_dump_analyse_dumpfile (char *dumpfile, char *die_selection, char *** lpm_lookup, char *** spm_lookup,
        char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup) 
{
    PHYMOD_FILE *dump_file;
    dump_file = PHYMOD_FOPEN(dumpfile, "r");
    plp_aperta_phymod_dump_update_database(dump_file, die_selection, lpm_lookup, spm_lookup, lpm_cdmib_lookup, spm_cdmib_lookup, others_lookup);
    PHYMOD_FCLOSE(dump_file);
    return;
}

void plp_aperta_phymod_dump_print_lookup (char *** lookup, int lookup_noofrows, PHYMOD_FILE *op_file) 
{
    int i = 0;
    for (i = 0; i < lookup_noofrows; i++) {
        PHYMOD_FPRINTF(op_file, "%s %s\n", lookup[i][0], lookup[i][1]);
    }
    return;
}

void plp_aperta_phymod_dump_init_lookup (char *** lookup, int lookup_noofrows, int lookup_noofcols) 
{
    int i, j = 0;
    int lookup_count = 0;
    for (i = 0; i < lookup_noofrows; i++) {
        lookup[i] = (char **)PHYMOD_IF_MALLOC(lookup_noofcols * sizeof(char *));
        for (j = 0; j < lookup_noofcols; j++) {
            lookup[i][j] = (char *)PHYMOD_IF_MALLOC(100 * sizeof(char));
            lookup_count++;
        }
    }
    for (i = 0; i < lookup_noofrows; i++) {
        for (j = 0; j < lookup_noofcols; j++) {
            PHYMOD_SPRINTF(lookup[i][j], "%s", "\0");
        }
    }
    return;
}

void plp_aperta_phymod_dump_free_lookup (char *** lookup, int lookup_noofrows, int lookup_noofcols) 
{
    int i, j = 0;
    int lookup_count = 0;
    for (i = 0; i < lookup_noofrows; i++) {
        for (j = 0; j < lookup_noofcols; j++) {
            PHYMOD_IF_FREE(lookup[i][j]);
            lookup_count++;
        }
        PHYMOD_IF_FREE(lookup[i]);
    }
    PHYMOD_IF_FREE(lookup);
    return;
}

void plp_aperta_phymod_dump_add_lookup (char *** lookup, int lookup_index, char *addr, char *reg_val) 
{
    PHYMOD_STRCPY(lookup[lookup_index][0], addr);
    PHYMOD_STRCPY(lookup[lookup_index][1], reg_val);
    return;
}

void plp_aperta_phymod_dump_update_database (PHYMOD_FILE *dump_file, char *die, char *** lpm_lookup, char *** spm_lookup, 
                      char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup) 
{

    const int MAXCHAR = PHYMOD_UTIL_STRING_MAX_SIZE;
    char dumpfile_line[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    char die_string[3] = "\0";
    char dumpfileline_die_string[3] = "\0";
    int  pm_cdmib_entry = 0;
    int  noofreg_entries = 0;
    int  lpm_entry_index = 0;
    int  spm_entry_index = 0;
    int  lpm_cdmib_entry_index = 0;
    int  spm_cdmib_entry_index = 0;
    int  others_entry_index = 0;
    int  i = 0;
    char port_info = '\0';
    char side_info = '\0';
    char *** entry_vals = (char ***)PHYMOD_IF_MALLOC(20 * sizeof(char **));

    plp_aperta_phymod_dump_init_lookup(entry_vals, 20, 2);
    PHYMOD_STRNCPY(die_string, die, 1);
    PHYMOD_STRCAT(die_string,".");
    PHYMOD_DIAG_OUT(("die: %s\n",die));
    PHYMOD_DIAG_OUT(("die_string: %s\n",die_string));

    while (PHYMOD_FGETS(dumpfile_line, MAXCHAR, dump_file)) {
        PHYMOD_STRNCPY(dumpfileline_die_string,dumpfile_line,2);
        dumpfileline_die_string[2] = '\0';
        if (PHYMOD_STRCMP(dumpfileline_die_string, die_string) == 0) {
            plp_aperta_phymod_dump_rmv_comment_inline(dumpfile_line);
            plp_aperta_phymod_dump_rplc_multiplespace_with_singlespace(dumpfile_line);
            plp_aperta_phymod_dump_rmv_dieinfo(dumpfile_line);
            if (PHYMOD_STRSTR(dumpfile_line,".") != NULL) {
                port_info = plp_aperta_rmv_portinfo(dumpfile_line);
            } else {
                port_info = 'N';
            }
            if ((PHYMOD_STRSTR(dumpfile_line,"(") != NULL) && (PHYMOD_STRSTR(dumpfile_line,")") != NULL)) {
                side_info = plp_aperta_rmv_sideinfo(dumpfile_line);
            } else {
                side_info = 'N';
            }
            plp_aperta_phymod_dump_rmv_colon(dumpfile_line);
            plp_aperta_phymod_dump_extract_entry_vals(dumpfile_line, port_info, side_info, entry_vals, &noofreg_entries, &pm_cdmib_entry);

            if (pm_cdmib_entry == 1) {
                if (side_info == 'L') {
                    for (i = 0; i < noofreg_entries; i++) {
                        plp_aperta_phymod_dump_add_lookup(lpm_cdmib_lookup, lpm_cdmib_entry_index, entry_vals[i][0], entry_vals[i][1]);
                        lpm_cdmib_entry_index++;
                    }
                } else {
                    for (i = 0; i < noofreg_entries; i++) {
                        plp_aperta_phymod_dump_add_lookup(spm_cdmib_lookup, spm_cdmib_entry_index, entry_vals[i][0], entry_vals[i][1]);
                        spm_cdmib_entry_index++;
                    }
                }
            } else {
                if (side_info == 'L') {
                    for (i = 0; i < noofreg_entries; i++) {
                        plp_aperta_phymod_dump_add_lookup(lpm_lookup, lpm_entry_index, entry_vals[i][0], entry_vals[i][1]);
                        lpm_entry_index++;
                    }
                } else if (side_info == 'S') {
                    for (i = 0; i < noofreg_entries; i++) {
                        plp_aperta_phymod_dump_add_lookup(spm_lookup, spm_entry_index, entry_vals[i][0], entry_vals[i][1]);
                        spm_entry_index++;
                    }
                } else {
                    for (i = 0; i < noofreg_entries; i++) {
                        plp_aperta_phymod_dump_add_lookup(others_lookup, others_entry_index, entry_vals[i][0], entry_vals[i][1]);
                        others_entry_index++;
                    }
                }
            }
        }
    }
    plp_aperta_phymod_dump_free_lookup(entry_vals, 16, 2);
}

void plp_aperta_phymod_dump_parse_display_templatefile (char *die_selection, char *templatefile, char *dictfile, char *dumpfile, PHYMOD_FILE *op_file,
                                 char *** lpm_lookup, char *** spm_lookup,
                                 char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup,
                                 int lpm_lookup_noofrows, int spm_lookup_noofrows,
                                 int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows, int l2_display) 
{
    const int MAXCHAR = PHYMOD_UTIL_STRING_MAX_SIZE;
    char templatefile_line[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    char first_line_identifier[50] = "DISPLAYING CHIP CONFIGURATION on DIE-";
    char reg_start_identifer[50]         = "  REG-NAME";
    char commented_line_identifier1[2] = "#";
    char chipinfoline[50] = "*CHIP INFO*";
    char pmdinfoline[50] = "*PMD CORE/LANE INFO*";
    char portstateline[50] = "*PORT CONFIG SNAPSHOT*";
    int  l2_line = 0;
    PHYMOD_FILE *template_file;
    PHYMOD_DIAG_OUT(("templatefile: %s\n", templatefile));


    template_file = PHYMOD_FOPEN(templatefile, "r");
    while (PHYMOD_FGETS(templatefile_line, MAXCHAR, template_file)) {
        if (PHYMOD_STRSTR(templatefile_line,first_line_identifier) != NULL) {
            PHYMOD_FPRINTF(op_file,"*** %s%s ***", first_line_identifier, die_selection); 
        } else if (PHYMOD_STRSTR(templatefile_line,chipinfoline) != NULL) {
            l2_line = plp_aperta_check_l2_line(templatefile_line);
            if (((l2_line == 1) && (l2_display == 1)) || (l2_line == 0)) {
                plp_aperta_phymod_dump_print_chip_config(template_file, templatefile_line, others_lookup, others_lookup_noofrows, op_file);
            }
        } else if (PHYMOD_STRSTR(templatefile_line,pmdinfoline) != NULL) {
            l2_line = plp_aperta_check_l2_line(templatefile_line);
            if (((l2_line == 1) && (l2_display == 1)) || (l2_line == 0)) {
                PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
                PHYMOD_FGETS(templatefile_line, MAXCHAR, template_file);
                PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
                plp_aperta_phymod_dump_print_pmd_snapshot(template_file, others_lookup, others_lookup_noofrows, 
                        lpm_lookup, lpm_lookup_noofrows, spm_lookup, spm_lookup_noofrows, op_file, l2_display);
            }
        } else if (PHYMOD_STRSTR(templatefile_line,portstateline) != NULL) {
            l2_line = plp_aperta_check_l2_line(templatefile_line);
            if (((l2_line == 1) && (l2_display == 1)) || (l2_line == 0)) {
                PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
                PHYMOD_FGETS(templatefile_line, MAXCHAR, template_file);
                PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
                PHYMOD_FGETS(templatefile_line, MAXCHAR, template_file);
                PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
                PHYMOD_FGETS(templatefile_line, MAXCHAR, template_file);
                PHYMOD_FGETS(templatefile_line, MAXCHAR, template_file);
                PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
                PHYMOD_FGETS(templatefile_line, MAXCHAR, template_file);
                PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
                PHYMOD_FGETS(templatefile_line, MAXCHAR, template_file);
                PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
                plp_aperta_phymod_dump_print_port_snapshot(template_file, others_lookup, others_lookup_noofrows, op_file);
            }
        } else if (PHYMOD_STRSTR(templatefile_line,reg_start_identifer) != NULL) {
            plp_aperta_phymod_dump_get_disp_reg_lines(die_selection, template_file, templatefile_line, dictfile, dumpfile, commented_line_identifier1, op_file,
                    lpm_lookup, spm_lookup,
                    lpm_cdmib_lookup, spm_cdmib_lookup, others_lookup, lpm_lookup_noofrows, spm_lookup_noofrows,
                    lpm_cdmib_lookup_noofrows, spm_cdmib_lookup_noofrows, others_lookup_noofrows, l2_display);
        } else {
            PHYMOD_FPRINTF(op_file, "%s", templatefile_line);
        }
    }

    PHYMOD_FCLOSE(template_file);

}

void plp_aperta_phymod_dump_print_port_snapshot (PHYMOD_FILE *ip_file, char *** others_lookup, int others_lookup_noofrows, PHYMOD_FILE *op_file) 
{
    const int MAXCHAR = PHYMOD_UTIL_STRING_MAX_SIZE;
    char ip_file_line[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    long long unsigned int chip_info_val = 0;
    int   chip_entry_found = 0;
    char notfoundstring[4] = "???";
    unsigned int entry_addr = 0;
    char entry_addr_strng[20] = "-";
    char tmp_array[20][9];
    int i, j = 0;

    for (j = 0; j < 20; j++) {
        PHYMOD_STRCPY(tmp_array[j], "\0");
    }

    while (PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file)) {
        for (i = 0; i < 8; i ++) {

            /* PORT*/
            PHYMOD_FPRINTF(op_file, "  %-8d ", i);

            /* STATUS*/
            PHYMOD_STRCPY(tmp_array[0], "DISBLD");
            PHYMOD_STRCPY(tmp_array[1], "CONFIG");
            PHYMOD_STRCPY(tmp_array[2], "ENBLNG");
            PHYMOD_STRCPY(tmp_array[3], "ENBLD");
            PHYMOD_STRCPY(tmp_array[4], "PAUSED");
            PHYMOD_STRCPY(tmp_array[5], "FLSHNG");
            PHYMOD_STRCPY(tmp_array[6], "FLSHD");
            PHYMOD_STRCPY(tmp_array[7], "DISBLNG");
            PHYMOD_STRCPY(tmp_array[8], "SW_MUX");
            entry_addr = 0x100A100 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 0, 16, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            /* TYPE*/
            PHYMOD_STRCPY(tmp_array[0], "RPTR");
            PHYMOD_STRCPY(tmp_array[1], "GBOX");
            PHYMOD_STRCPY(tmp_array[2], "RGBOX");
            PHYMOD_STRCPY(tmp_array[3], "\0");
            PHYMOD_STRCPY(tmp_array[4], "\0");
            PHYMOD_STRCPY(tmp_array[5], "\0");
            PHYMOD_STRCPY(tmp_array[6], "\0");
            PHYMOD_STRCPY(tmp_array[7], "\0");
            PHYMOD_STRCPY(tmp_array[8], "\0");
            entry_addr = 0x100A102 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }

            /* SPEED*/
            PHYMOD_STRCPY(tmp_array[0], "10G");
            PHYMOD_STRCPY(tmp_array[1], "25G");
            PHYMOD_STRCPY(tmp_array[2], "40G");
            PHYMOD_STRCPY(tmp_array[3], "50GNRZ");
            PHYMOD_STRCPY(tmp_array[4], "50GPAM4");
            PHYMOD_STRCPY(tmp_array[5], "100GNRZ");
            PHYMOD_STRCPY(tmp_array[6], "100GPAM4");
            PHYMOD_STRCPY(tmp_array[7], "200G");
            PHYMOD_STRCPY(tmp_array[8], "400G");
            entry_addr = 0x100A103 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 0, 32, &chip_entry_found);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            /* DP-PORT*/
            entry_addr = 0x100A102 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 0, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8llX ", chip_info_val);
            }
            /* SPID*/
            entry_addr = 0x100A104 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 0, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8llX ", chip_info_val);
            }
            /* LPID*/
            entry_addr = 0x100A104 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8llX ", chip_info_val);
            }
            /* FLT-OPT*/
            PHYMOD_STRCPY(tmp_array[0], "TnG");
            PHYMOD_STRCPY(tmp_array[1], "PT");
            PHYMOD_STRCPY(tmp_array[2], "\0");
            PHYMOD_STRCPY(tmp_array[3], "\0");
            PHYMOD_STRCPY(tmp_array[4], "\0");
            PHYMOD_STRCPY(tmp_array[5], "\0");
            PHYMOD_STRCPY(tmp_array[6], "\0");
            PHYMOD_STRCPY(tmp_array[7], "\0");
            PHYMOD_STRCPY(tmp_array[8], "\0");
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 2, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            /* FC-OPT*/
            PHYMOD_STRCPY(tmp_array[0], "TnG");
            PHYMOD_STRCPY(tmp_array[1], "PT");
            PHYMOD_STRCPY(tmp_array[2], "\0");
            PHYMOD_STRCPY(tmp_array[3], "\0");
            PHYMOD_STRCPY(tmp_array[4], "\0");
            PHYMOD_STRCPY(tmp_array[5], "\0");
            PHYMOD_STRCPY(tmp_array[6], "\0");
            PHYMOD_STRCPY(tmp_array[7], "\0");
            PHYMOD_STRCPY(tmp_array[8], "\0");
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 1, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            /* SF-OPT*/
            PHYMOD_STRCPY(tmp_array[0], "DISBLD");
            PHYMOD_STRCPY(tmp_array[1], "ENBLD");
            PHYMOD_STRCPY(tmp_array[2], "\0");
            PHYMOD_STRCPY(tmp_array[3], "\0");
            PHYMOD_STRCPY(tmp_array[4], "\0");
            PHYMOD_STRCPY(tmp_array[5], "\0");
            PHYMOD_STRCPY(tmp_array[6], "\0");
            PHYMOD_STRCPY(tmp_array[7], "\0");
            PHYMOD_STRCPY(tmp_array[8], "\0");
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 0, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            /* PTP-OPT*/
            PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            /* FIX-LAT*/
            PHYMOD_STRCPY(tmp_array[0], "DISBLD");
            PHYMOD_STRCPY(tmp_array[1], "ENBLD");
            PHYMOD_STRCPY(tmp_array[2], "\0");
            PHYMOD_STRCPY(tmp_array[3], "\0");
            PHYMOD_STRCPY(tmp_array[4], "\0");
            PHYMOD_STRCPY(tmp_array[5], "\0");
            PHYMOD_STRCPY(tmp_array[6], "\0");
            PHYMOD_STRCPY(tmp_array[7], "\0");
            PHYMOD_STRCPY(tmp_array[8], "\0");
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 7, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            /* PORT Mode - FO-MUX*/
            PHYMOD_STRCPY(tmp_array[0], "OFF");
            PHYMOD_STRCPY(tmp_array[1], "ON");
            PHYMOD_STRCPY(tmp_array[2], "\0");
            PHYMOD_STRCPY(tmp_array[3], "\0");
            PHYMOD_STRCPY(tmp_array[4], "\0");
            PHYMOD_STRCPY(tmp_array[5], "\0");
            PHYMOD_STRCPY(tmp_array[6], "\0");
            PHYMOD_STRCPY(tmp_array[7], "\0");
            PHYMOD_STRCPY(tmp_array[8], "\0");
            entry_addr = 0x100A103 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 0, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            /* FOMUX_SIDE*/
            PHYMOD_STRCPY(tmp_array[0], "SYS");
            PHYMOD_STRCPY(tmp_array[1], "LINE");
            PHYMOD_STRCPY(tmp_array[2], "\0");
            PHYMOD_STRCPY(tmp_array[3], "\0");
            PHYMOD_STRCPY(tmp_array[4], "\0");
            PHYMOD_STRCPY(tmp_array[5], "\0");
            PHYMOD_STRCPY(tmp_array[6], "\0");
            PHYMOD_STRCPY(tmp_array[7], "\0");
            PHYMOD_STRCPY(tmp_array[8], "\0");
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            /* FO-DP-PORT*/
            entry_addr = 0x100A107 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 0, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8llX ", chip_info_val);
            }
            /* FO-PID*/
            entry_addr = 0x100A107 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8llX ", chip_info_val);
            }
            /* FO-PRI-PORT*/
            PHYMOD_STRCPY(tmp_array[0], "PRI");
            PHYMOD_STRCPY(tmp_array[1], "2NDARY");
            PHYMOD_STRCPY(tmp_array[2], "\0");
            PHYMOD_STRCPY(tmp_array[3], "\0");
            PHYMOD_STRCPY(tmp_array[4], "\0");
            PHYMOD_STRCPY(tmp_array[5], "\0");
            PHYMOD_STRCPY(tmp_array[6], "\0");
            PHYMOD_STRCPY(tmp_array[7], "\0");
            PHYMOD_STRCPY(tmp_array[8], "\0");
            entry_addr = 0x100A100 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 12, 4, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-9s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%-8s ", tmp_array[chip_info_val]);
            }
            PHYMOD_FPRINTF(op_file, "\n");
            PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file);
        }
        return;

    }

}
void plp_aperta_phymod_dump_print_pmd_snapshot (PHYMOD_FILE *ip_file, char *** others_lookup, int others_lookup_noofrows, 
                         char *** lpm_lookup, int lpm_lookup_noofrows, 
                         char *** spm_lookup, int spm_lookup_noofrows, PHYMOD_FILE *op_file, int l2_display) 
{
    char ip_file_line[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    char ip_file_line1[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    const int MAXCHAR = PHYMOD_UTIL_STRING_MAX_SIZE;
    char ip_fileline_strt_string[3] = "\0";
    char dash_line_identifier[3] = "--";
    char eq_line_identifier[3] = "==";
    int  cluster_index = 0;
    int  l2_line = 0;

    while (PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file)) {
        PHYMOD_STRCPY(ip_file_line1, ip_file_line);
        l2_line = plp_aperta_check_l2_line(ip_file_line);
        if (((l2_line == 1) && (l2_display == 1)) || (l2_line == 0)) {
            plp_aperta_phymod_dump_rmv_strtng_whitespace(ip_file_line);
            PHYMOD_STRNCPY(ip_fileline_strt_string, ip_file_line, 2);
            ip_fileline_strt_string[2] = '\0';
            if (PHYMOD_STRCMP(ip_fileline_strt_string, dash_line_identifier) == 0) {
                PHYMOD_STRCPY(ip_file_line1, ip_file_line);
                PHYMOD_FPRINTF(op_file, "%s", ip_file_line1);
                if (cluster_index == 0) {
                    plp_aperta_phymod_dump_print_core_info(op_file, spm_lookup, spm_lookup_noofrows, others_lookup, others_lookup_noofrows);
                } else if (cluster_index == 1) {
                    plp_aperta_phymod_dump_print_core_info(op_file, lpm_lookup, lpm_lookup_noofrows, others_lookup, others_lookup_noofrows);
                } else if (cluster_index == 2) {
                    plp_aperta_phymod_dump_print_serdes_info(ip_file, op_file, others_lookup, others_lookup_noofrows);
                } else {
                    plp_aperta_phymod_dump_print_serdes_info(ip_file, op_file, others_lookup, others_lookup_noofrows);
                }
                PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file);
                cluster_index++;
            } else if (PHYMOD_STRCMP(ip_fileline_strt_string, eq_line_identifier) == 0) {
                PHYMOD_FPRINTF(op_file, "%s", ip_file_line);
                return;
            } else {
                PHYMOD_FPRINTF(op_file, "%s", ip_file_line);
            }
        }
    }
    return;
}

void plp_aperta_phymod_dump_print_serdes_info (PHYMOD_FILE *ip_file, PHYMOD_FILE *op_file, char *** others_lookup, int others_lookup_noofrows) 
{
    const int MAXCHAR = PHYMOD_UTIL_STRING_MAX_SIZE;
    char ip_file_line[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    long long unsigned int chip_info_val = 0;
    int   chip_entry_found = 0;
    char notfoundstring[2] = "-";
    unsigned int entry_addr = 0;
    char entry_addr_strng[20] = "-";
    int i = 0;

    while (PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file)) {
        for (i = 0; i < 8; i ++) {

            /*SYS_LN*/
            PHYMOD_FPRINTF(op_file, "  %-8d ", i);

            /* SD - TBD*/
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%4s ", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%llX ", chip_info_val);
            }
            /* PMD_LCK - TBD*/
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%4s ", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%8llX ", chip_info_val);
            }
            /* PLL - TBD*/
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%4s ", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%8llX ", chip_info_val);
            }
            /* LN_MAP - TBD*/
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%4s ", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%8llX ", chip_info_val);
            }
            /*POL_SWP - TBD*/
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%4s ", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%8llX ", chip_info_val);
            }
            /* TXEQ - TBD*/
            entry_addr = 0x100A101 + i*16;
            PHYMOD_SPRINTF(entry_addr_strng, "%x", entry_addr);
            chip_info_val = plp_aperta_get_disp_entry_val(entry_addr_strng, others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%4s ", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%8llX ", chip_info_val);
            }
            PHYMOD_FPRINTF(op_file, "\n");
            PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file);
        }
        PHYMOD_FPRINTF(op_file, "\n");
        PHYMOD_FPRINTF(op_file, "\n");
        return;

    }

    return;

}

void plp_aperta_phymod_dump_print_core_info (PHYMOD_FILE *op_file, char *** pm_lookup, int pm_lookup_noofrows, char *** others_lookup, int others_lookup_noofrows) 
{
    long long unsigned int chip_info_val = 0;
    int   chip_entry_found = 0;
    char notfoundstring[2] = "-";

    /* Core*/
    PHYMOD_FPRINTF(op_file, "  BH1650");

    /* FW_VER*/
    chip_info_val = plp_aperta_get_disp_entry_val("1800d0d9", pm_lookup, pm_lookup_noofrows, 0, 8, &chip_entry_found);
    if (chip_entry_found == 0) {
        PHYMOD_FPRINTF(op_file, "%4s", notfoundstring);
    } else {
        PHYMOD_FPRINTF(op_file, "%3llX", chip_info_val);
    }
    /* COM_CK*/
    chip_info_val = plp_aperta_get_disp_entry_val("1008200", others_lookup, others_lookup_noofrows, 8, 4, &chip_entry_found);
    if (chip_entry_found == 0) {
        PHYMOD_FPRINTF(op_file, "%4s", notfoundstring);
    } else {
        if (chip_info_val == 6) {
            PHYMOD_FPRINTF(op_file, "        156.25M");
        } else {
            PHYMOD_FPRINTF(op_file, "        250M");
        }
    }
    chip_info_val = plp_aperta_get_disp_entry_val("1800d149", pm_lookup, pm_lookup_noofrows, 0, 8, &chip_entry_found);
    if (chip_entry_found == 0) {
        PHYMOD_FPRINTF(op_file, "%4s", notfoundstring);
    } else {
        PHYMOD_FPRINTF(op_file, "%2llX", chip_info_val);
    }
    /* PLL0LOCK*/
    chip_info_val = plp_aperta_get_disp_entry_val("1008158", others_lookup, others_lookup_noofrows, 9, 1, &chip_entry_found);
    if (chip_entry_found == 0) {
        PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
    } else {
        PHYMOD_FPRINTF(op_file, "%8llX", chip_info_val);
    }
    chip_info_val = plp_aperta_get_disp_entry_val("1008158", others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
    if ((chip_info_val == 1) && (chip_entry_found == 1)) {
        PHYMOD_FPRINTF(op_file, "*");
    }
    /* PLL1*/
    chip_info_val = plp_aperta_get_disp_entry_val("1800d149", pm_lookup, pm_lookup_noofrows, 0, 8, &chip_entry_found);
    if (chip_entry_found == 0) {
        PHYMOD_FPRINTF(op_file, "%8s", notfoundstring);
    } else {
        PHYMOD_FPRINTF(op_file, "%2llX", chip_info_val);
    }

    /* PLL1LOCK*/
    chip_info_val = plp_aperta_get_disp_entry_val("1008158", others_lookup, others_lookup_noofrows, 9, 1, &chip_entry_found);
    if (chip_entry_found == 0) {
        PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
    } else {
        PHYMOD_FPRINTF(op_file, "%8llX", chip_info_val);
    }
    chip_info_val = plp_aperta_get_disp_entry_val("1008158", others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
    if ((chip_info_val == 1) && (chip_entry_found == 1)) {
        PHYMOD_FPRINTF(op_file, "*");
    }
    return;
}

void plp_aperta_phymod_dump_print_chip_config (PHYMOD_FILE *ip_file, char *curr_line, char *** others_lookup, int others_lookup_noofrows, PHYMOD_FILE *op_file) 
{
    char ip_file_line[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    char ip_file_line1[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    const int MAXCHAR = PHYMOD_UTIL_STRING_MAX_SIZE;
    long long unsigned int chip_info_val = 0;
    long long unsigned int mst_sec, mst_dram_sec = 0;
    long long unsigned int mst_ded, mst_dram_ded, lmi_ded, lpmif_ded, spmif_ded = 0;
    int   chip_entry_found = 0;
    char ip_fileline_strt_string[3] = "\0";
    char dash_line_identifier[3] = "--";
    char notfoundstring[2] = "-";
    char eccsts[9] = "no-error";
#ifdef SERDES_API_FLOATING_POINT
    float vtmon= 0;
#endif

    PHYMOD_FPRINTF(op_file, "%s", curr_line);
    while (PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file)) {
        PHYMOD_STRCPY(ip_file_line1, ip_file_line);
        plp_aperta_phymod_dump_rmv_strtng_whitespace(ip_file_line);
        PHYMOD_STRNCPY(ip_fileline_strt_string, ip_file_line, 2);
        ip_fileline_strt_string[2] = '\0';
        if (PHYMOD_STRCMP(ip_fileline_strt_string, dash_line_identifier) == 0) {
            PHYMOD_FPRINTF(op_file, "%s", ip_file_line1);
            /* CHIP_ID  CHIP_FW  BH_FW    PLLLOCK  RESCAL   VTMON    COM_CK   REF_CK   LMI_sts  ECC_sts  
               -------  -------  -------  -------  -------  -------  -------  -------  -------  -------*/

            /* CHIP_ID*/
            chip_info_val = plp_aperta_get_disp_entry_val("1008b01", others_lookup, others_lookup_noofrows, 12, 4, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%3llX", chip_info_val);
            }
            chip_info_val = plp_aperta_get_disp_entry_val("1008b00", others_lookup, others_lookup_noofrows, 0, 16, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%llX", chip_info_val);
            }
            chip_info_val = plp_aperta_get_disp_entry_val("1008b01", others_lookup, others_lookup_noofrows, 0, 8, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%llX", chip_info_val);
            }
            /* CHIP_FW*/
            chip_info_val = plp_aperta_get_disp_entry_val("1008215", others_lookup, others_lookup_noofrows, 0, 16, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%6llX", chip_info_val);
            }

            /* BH_FW*/
            PHYMOD_FPRINTF(op_file, "     ??? ");

            /* PLLLOCK*/
            chip_info_val = plp_aperta_get_disp_entry_val("1008158", others_lookup, others_lookup_noofrows, 9, 1, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%6llX", chip_info_val);
            }
            chip_info_val = plp_aperta_get_disp_entry_val("1008158", others_lookup, others_lookup_noofrows, 8, 1, &chip_entry_found);
            if ((chip_info_val == 1) && (chip_entry_found == 1)) {
                PHYMOD_FPRINTF(op_file, "*");
            }
            /* RESCAL*/
            chip_info_val = plp_aperta_get_disp_entry_val("100811a", others_lookup, others_lookup_noofrows, 0, 4, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%8llX", chip_info_val);
            }
            /*VTMON*/
#ifdef SERDES_API_FLOATING_POINT
            chip_info_val = plp_aperta_get_disp_entry_val("1008129", others_lookup, others_lookup_noofrows, 0, 10, &chip_entry_found);
            vtmon = 434.1 - chip_info_val*0.53504;
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
            } else {
                PHYMOD_FPRINTF(op_file, "%13.2fc", vtmon);
            }
#else
            PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);

#endif
            /* COM_CK*/
            chip_info_val = plp_aperta_get_disp_entry_val("1008200", others_lookup, others_lookup_noofrows, 8, 4, &chip_entry_found);
            if (chip_entry_found == 0) {
                PHYMOD_FPRINTF(op_file, "%-8s", notfoundstring);
            } else {
                if (chip_info_val == 6) {
                    PHYMOD_FPRINTF(op_file, "   156.25M");
                } else {
                    PHYMOD_FPRINTF(op_file, "   250M");
                }
            }
            /* REF_CK*/
            PHYMOD_FPRINTF(op_file, "  156.25M");

            /* LMI_sts*/
            chip_info_val = plp_aperta_get_disp_entry_val("1008B88", others_lookup, others_lookup_noofrows, 1, 1, &chip_entry_found);
            if (chip_info_val == 1) {
                PHYMOD_FPRINTF(op_file, "  Error");
            } else {
                PHYMOD_FPRINTF(op_file, "  no-error");
            }
            /* ECC_sts*/
            mst_sec      = plp_aperta_get_disp_entry_val("1008b82", others_lookup, others_lookup_noofrows, 0, 1, &chip_entry_found);
            mst_dram_sec = plp_aperta_get_disp_entry_val("1008b82", others_lookup, others_lookup_noofrows, 2, 1, &chip_entry_found);
            mst_ded      = plp_aperta_get_disp_entry_val("1008b82", others_lookup, others_lookup_noofrows, 1, 1, &chip_entry_found);
            mst_dram_ded = plp_aperta_get_disp_entry_val("1008b82", others_lookup, others_lookup_noofrows, 3, 1, &chip_entry_found);
            lmi_ded      = plp_aperta_get_disp_entry_val("1008b88", others_lookup, others_lookup_noofrows, 0, 1, &chip_entry_found);
            lpmif_ded    = plp_aperta_get_disp_entry_val("1008b88", others_lookup, others_lookup_noofrows, 2, 1, &chip_entry_found);
            spmif_ded    = plp_aperta_get_disp_entry_val("1008b88", others_lookup, others_lookup_noofrows, 4, 1, &chip_entry_found);
            if ((mst_sec == 1) || (mst_dram_sec == 1)) {
                PHYMOD_STRCPY(eccsts, "SEC");
            } else if ((mst_ded == 1) || (mst_dram_ded == 1) || (lmi_ded == 1) || (lpmif_ded == 1) || (spmif_ded == 1)) {
                PHYMOD_STRCPY(eccsts, " DED");
            } else {
                PHYMOD_STRCPY(eccsts, "no-error");
            }
            PHYMOD_FPRINTF(op_file, " %-8s", eccsts);

            PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file);
            return;
        } else {
            PHYMOD_FPRINTF(op_file, "%s", ip_file_line1);
        }
    }
    return;
}

void plp_aperta_phymod_dump_get_disp_reg_lines (char *die_selection, PHYMOD_FILE *ip_file, char *curr_line, char *dictfile, char *dumpfile, char *pattern, PHYMOD_FILE *op_file, 
                         char *** lpm_lookup, char *** spm_lookup,
                         char *** lpm_cdmib_lookup, char *** spm_cdmib_lookup, char *** others_lookup, 
                         int lpm_lookup_noofrows, int spm_lookup_noofrows,
                         int lpm_cdmib_lookup_noofrows, int spm_cdmib_lookup_noofrows, int others_lookup_noofrows, int l2_display) 
{

    char ip_file_line[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    char ip_file_line1[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    const int MAXCHAR = PHYMOD_UTIL_STRING_MAX_SIZE;
    char commented_line_identifier[3] = "//";
    char dash_line_identifier[3] = "--";
    char eof_identifier[6] = "<EOF>";
    char ip_fileline_strt_string[2] = "\0";
    char ip_fileline_strt_string1[3] = "\0";
    char ip_fileline_strt_string2[6] = "\0";
    char pm_side_identifier[5] = "\0";
    char lpm_line_identifier[5] = "LPM_";
    char spm_line_identifier[5] = "SPM_";
    char  disp_reg_name_ptr[100];
    char  disp_reg_addr_ptr[10][100];
    char  display_regs_addr[8][100];
    char  cdmib_regs_addr[20] = "\0";
    char port_index[2] = "\0";
    long long unsigned int disp_entry_val = 0;
    char  commonreg_string[7] = "Common";
    int   pm_line = 0;
    int   common_reg = 0;
    int   i = 0;
    int   hex_addr = 0;
    int   entry_found = 0;
    int   l2_line = 0;
    int   noofregentries = 0;
    int   bit_pos[8];
    int   noofbits[8];

    for (i = 0; i < 8; i++) {
        bit_pos[i]  = 0;
        noofbits[i] = 32;
        PHYMOD_STRCPY(display_regs_addr[i], "\0");
    }
    PHYMOD_STRCPY(disp_reg_name_ptr, "\0");
    for (i = 0; i < 10; i++) {
        PHYMOD_STRCPY(disp_reg_addr_ptr[i], "\0");
    }

    PHYMOD_FPRINTF(op_file, "%s", curr_line);
    while (PHYMOD_FGETS(ip_file_line, MAXCHAR, ip_file)) {
        PHYMOD_STRCPY(ip_file_line1, ip_file_line);
        plp_aperta_phymod_dump_rmv_strtng_whitespace(ip_file_line);
        PHYMOD_STRNCPY(ip_fileline_strt_string, ip_file_line, 2);
        ip_fileline_strt_string[1] = '\0';
        PHYMOD_STRNCPY(ip_fileline_strt_string1, ip_file_line, 3);
        ip_fileline_strt_string1[2] = '\0';
        PHYMOD_STRNCPY(ip_fileline_strt_string2, ip_file_line, 6);
        ip_fileline_strt_string2[5] = '\0';
        PHYMOD_STRNCPY(pm_side_identifier, ip_file_line, 5);
        pm_side_identifier[4] = '\0';
        if (PHYMOD_STRCMP(ip_fileline_strt_string, pattern) == 0) {
            PHYMOD_FPRINTF(op_file, "%s", ip_file_line1);
            return;
        } else if (PHYMOD_STRCMP(ip_fileline_strt_string1, commented_line_identifier) == 0) {
        } else if ((PHYMOD_STRCMP(ip_fileline_strt_string1, dash_line_identifier) == 0) ||
                (PHYMOD_STRCMP(ip_file_line, "\n") == 0) || (PHYMOD_STRCMP(ip_file_line, "\r\n") == 0)) {
            PHYMOD_FPRINTF(op_file, "%s", ip_file_line1);
        } else if (PHYMOD_STRCMP(ip_fileline_strt_string2, eof_identifier) == 0) {
        } else {
            l2_line = plp_aperta_check_l2_line(ip_file_line);
            if (((l2_line == 1) && (l2_display == 1)) || (l2_line == 0)) {
                for (i = 0; i < 8; i++) {
                    PHYMOD_STRCPY(display_regs_addr[i],"\0");
                }
                if (PHYMOD_STRSTR(ip_file_line,commonreg_string) != NULL) {
                    common_reg = 1;
                } else {
                    common_reg = 0;
                }
                if ((PHYMOD_STRCMP(pm_side_identifier, lpm_line_identifier) == 0) ||
                        (PHYMOD_STRCMP(pm_side_identifier, spm_line_identifier) == 0)) {
                    pm_line = 1;
                } else {
                    pm_line = 0;
                }
                plp_aperta_phymod_dump_extract_reg_name_addr(ip_file_line, dictfile, pm_line, pm_side_identifier, common_reg, disp_reg_name_ptr, 
                        disp_reg_addr_ptr, &noofregentries, bit_pos, noofbits);
                hex_addr = plp_aperta_analyse_reg_type(disp_reg_name_ptr, disp_reg_addr_ptr[0], display_regs_addr, pm_side_identifier, common_reg);
                if (hex_addr == 1) {
                    PHYMOD_FPRINTF(op_file,"  %-44s ", disp_reg_name_ptr);
                    for (i = 0; i < 8; i++) {
                        if (pm_line == 1) {
                            if (PHYMOD_STRCMP(pm_side_identifier, lpm_line_identifier) == 0) {
                                disp_entry_val = plp_aperta_get_disp_entry_val(display_regs_addr[i], lpm_lookup, lpm_lookup_noofrows, 0, 32, &entry_found);
                            } else {
                                disp_entry_val = plp_aperta_get_disp_entry_val(display_regs_addr[i], spm_lookup, spm_lookup_noofrows, 0, 32, &entry_found);
                            }
                        } else {
                            disp_entry_val = plp_aperta_get_disp_entry_val(display_regs_addr[i], others_lookup, others_lookup_noofrows, 0, 32, &entry_found);
                        }
                        if (entry_found == 1) {
                            if ((pm_line == 0) && (common_reg == 1) && (i > 0)) {
                                PHYMOD_FPRINTF(op_file, "%-8s ", "-");
                            } else {
                                PHYMOD_FPRINTF(op_file, "%-8llX ", disp_entry_val);
                            }
                        } else {
                            PHYMOD_FPRINTF(op_file, "%-8s ", "-");
                        }
                    }
                    PHYMOD_FPRINTF(op_file, "\n");
                } else {
                    for (i = 0; i < 8; i++) {
                        if (i > 0) {
                            PHYMOD_SPRINTF(port_index,"%x",i);
                            plp_aperta_phymod_dump_compute_cdmib_addr(display_regs_addr[i], port_index, cdmib_regs_addr);
                            PHYMOD_STRCPY(display_regs_addr[i],cdmib_regs_addr);
                        }
                    }
                    if (PHYMOD_STRCMP(pm_side_identifier, lpm_line_identifier) == 0) {
                        PHYMOD_FPRINTF(op_file,"  %-44s ", disp_reg_name_ptr);
                        for (i = 0; i < 8; i++) {
                            disp_entry_val = plp_aperta_get_disp_entry_val(display_regs_addr[i], lpm_cdmib_lookup, lpm_cdmib_lookup_noofrows, 0, 32, &entry_found);
                            if (entry_found == 1) {
                                PHYMOD_FPRINTF(op_file, "%-8llX ", disp_entry_val);
                            } else {
                                PHYMOD_FPRINTF(op_file, "%-8s ", "-");
                            }
                        }
                        PHYMOD_FPRINTF(op_file, "\n");
                    } else {
                        PHYMOD_FPRINTF(op_file,"  %-44s ", disp_reg_name_ptr);
                        for (i = 0; i < 8; i++) {
                            disp_entry_val = plp_aperta_get_disp_entry_val(display_regs_addr[i], spm_cdmib_lookup, spm_cdmib_lookup_noofrows, 0, 32, &entry_found);
                            if (entry_found == 1) {
                                PHYMOD_FPRINTF(op_file, "%-8llX ", disp_entry_val);
                            } else {
                                PHYMOD_FPRINTF(op_file, "%-8s ", "-");
                            }
                        }
                        PHYMOD_FPRINTF(op_file, "\n");
                    }
                }
            }
        }
    }
}

void plp_aperta_phymod_dump_compute_cdmib_addr (char *display_regs_addr, char *port_index, char *cdmib_regs_addr) 
{
    int i, j = 0;
    int sqr_brkt_position = 0;

    for (i = 0; i < PHYMOD_STRLEN(display_regs_addr); i++) {
        if (display_regs_addr[i] == '[') {
            sqr_brkt_position = i;
        }
    }
    for (i = 0; i <= sqr_brkt_position; i++) {
        cdmib_regs_addr[i] = display_regs_addr[i];
    }
    j = sqr_brkt_position+1;
    for (i = 0; i < PHYMOD_STRLEN(port_index); i++) {
        cdmib_regs_addr[j] = port_index[i];
        j++;
    }
    for (i = sqr_brkt_position+1; i < PHYMOD_STRLEN(display_regs_addr); i++) {
        cdmib_regs_addr[j] = display_regs_addr[i];
        j++;
    }

    return;
}

int plp_aperta_analyse_reg_type (char *disp_reg_name_ptr, char *disp_reg_addr_ptr, char display_regs_addr[][100], char *pm_string, int common_reg) 
{
    char  disp_reg_addr1[3] = "\0";
    char  disp_reg_addr2[7] = "\0";
    char  hex_addr_identifier[3] = "0x";
    char  indirect_addr[7] = "0x4900";
    char  eip163e_addr[7]  = "0x4500";
    char  eip163i_addr[7]  = "0x4300";
    char  eip164e_addr[7]  = "0x4600";
    char  eip164i_addr[7]  = "0x4400";
    unsigned int base_addr = 0;
    char disp_base_addr[20] = "\0";
    int  i = 0;
    int  hex_addr = 0;
    PHYMOD_STRNCPY(disp_reg_addr1, disp_reg_addr_ptr, 2);
    disp_reg_addr1[2] = '\0';
    PHYMOD_STRNCPY(disp_reg_addr2, disp_reg_addr_ptr, 6);
    disp_reg_addr2[6] = '\0';
    if (PHYMOD_STRCMP(disp_reg_addr1, hex_addr_identifier) == 0) {
        hex_addr = 1;
        base_addr = (int)PHYMOD_STRTOL(disp_reg_addr_ptr, NULL, 16);
        PHYMOD_SPRINTF(disp_base_addr, "%x", base_addr);
        if (common_reg == 1) {
            for (i = 0; i < 8; i++) {
                PHYMOD_STRCPY(display_regs_addr[i],disp_base_addr);
            }
        } else {
            if (PHYMOD_STRCMP(disp_reg_addr2, indirect_addr) == 0) {
                for (i = 0; i < 8; i++) {
                    PHYMOD_SPRINTF(disp_base_addr, "%x.%d", base_addr + i*256, i);
                    PHYMOD_STRCPY(display_regs_addr[i],disp_base_addr);
                }
            } else if ((PHYMOD_STRCMP(disp_reg_addr2, eip163e_addr) == 0) ||
                    (PHYMOD_STRCMP(disp_reg_addr2, eip163i_addr) == 0) ||
                    (PHYMOD_STRCMP(disp_reg_addr2, eip164e_addr) == 0) ||
                    (PHYMOD_STRCMP(disp_reg_addr2, eip164i_addr) == 0)) {
                for (i = 0; i < 8; i++) {
                    PHYMOD_SPRINTF(disp_base_addr, "%x.%d", base_addr + i*64, i);
                    PHYMOD_STRCPY(display_regs_addr[i],disp_base_addr);
                }
            } else {
                for (i = 0; i < 8; i++) {
                    PHYMOD_SPRINTF(disp_base_addr, "%x.%d", base_addr,i);
                    PHYMOD_STRCPY(display_regs_addr[i],disp_base_addr);
                }
            }
        }
    } else {
        hex_addr = 0;
        PHYMOD_SPRINTF(disp_base_addr, "%s", disp_reg_addr_ptr);
        for (i = 0; i < 8; i++) {
            PHYMOD_STRCPY(display_regs_addr[i],disp_base_addr);
        }
    }
    return hex_addr;
}

void plp_aperta_phymod_dump_extract_reg_name_addr (char *ip_line, char *dictfile, int pm_line, char *pm_string, int common_reg, char disp_reg_name_ptr[], 
                            char disp_reg_addr_ptr[][100], int *noofregentries, int *bit_pos, int *noofbits) 
{
    char ip_line2[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    char *dispregname;
    int i, j = 0;
    dispregname = "\0";

    plp_aperta_phymod_dump_rmv_strtng_whitespace(ip_line);
    PHYMOD_STRCPY(disp_reg_name_ptr, "\0");

    if (pm_line == 1) {
        PHYMOD_STRNCPY(ip_line2, ip_line + 4, PHYMOD_STRLEN(ip_line));
        dispregname = plp_aperta_get_disp_reg_name(ip_line2);
    } else {
        dispregname = plp_aperta_get_disp_reg_name(ip_line);
    }
    plp_aperta_phymod_dump_get_disp_reg_addr(dispregname, dictfile, disp_reg_addr_ptr, noofregentries, bit_pos, noofbits);
    j = 0;
    if (pm_line == 1) {
        for (i = 0; i < PHYMOD_STRLEN(pm_string); i++ ) {
            disp_reg_name_ptr[j++] = pm_string[i];
        }
    }
    for (i = 0; i < PHYMOD_STRLEN(dispregname); i++ ) {
        disp_reg_name_ptr[j++] = dispregname[i];
    }
    disp_reg_name_ptr[j] = '\0';
    return;
}

char* plp_aperta_get_disp_reg_name (char *ip_line) 
{
  char* first_entry;
  first_entry = PHYMOD_STRTOK(ip_line, " ");
  return (char *) first_entry;
}

void plp_aperta_phymod_dump_get_disp_reg_addr (char *ip_line, char *dictfile, char disp_reg_addr_ptr[][100], int *noofregentries, int *bit_pos, int *noofbits) 
{

    char dict_file_line[PHYMOD_UTIL_STRING_MAX_SIZE] = "\0";
    const int MAXCHAR = PHYMOD_UTIL_STRING_MAX_SIZE;

    PHYMOD_FILE *dict_file;
    dict_file = PHYMOD_FOPEN(dictfile, "r");

    while (PHYMOD_FGETS(dict_file_line, MAXCHAR, dict_file)) {
        if (PHYMOD_STRSTR(dict_file_line,ip_line) != NULL) {
            plp_aperta_phymod_dump_analyse_dict_file_line(dict_file_line, disp_reg_addr_ptr, noofregentries, bit_pos, noofbits);
            PHYMOD_FCLOSE(dict_file);
            return;
        }
    }
    PHYMOD_FCLOSE(dict_file);
    return;
}

void plp_aperta_phymod_dump_analyse_dict_file_line (char *ip_line, char disp_reg_addr_ptr[][100], int *noofregentries, int *bit_pos, int *noofbits) 
{
    int curly_bracket_count = 0;
    int comma_count = 0;
    int i, j = 0;

    char dict_addr_entry[500] = "\0";
    int comma_found = 0;
    int first_comma_pos = 0;

    char *addr_entry;
    char *addr_entry_arrray[20];
    int position_index = 0;

    int line_length = 0;
    plp_aperta_phymod_dump_rmv_strtng_whitespace(ip_line);
    plp_aperta_phymod_dump_rplc_multiplespace_with_singlespace(ip_line);

    line_length = PHYMOD_STRLEN(ip_line);

    addr_entry ="\0";
    for (i = 0; i < 19; i++) {
        addr_entry_arrray[i] = "\0";
    }

    j = 0;
    for (i = 1; i < line_length - 3; i++) {
        ip_line[j] = ip_line[i];
        j++;
    }
    ip_line[j] = '\0';

    comma_found = 0;
    first_comma_pos = 0;
    for (i = 1; i < line_length; i++) {
        if ((comma_found == 0) && (ip_line[i] == ',')) {
            comma_found = 1;
            first_comma_pos = i;
        }
    }
    j = 0;
    for (i = first_comma_pos + 2; i < line_length; i++) {
        dict_addr_entry[j] = ip_line[i];
        j++;
    }
    dict_addr_entry[j] = '\0';

    comma_count = plp_aperta_pattern_count(',', dict_addr_entry);
    curly_bracket_count = plp_aperta_pattern_count('{', dict_addr_entry);

    if (curly_bracket_count > 0) {
        *noofregentries = (comma_count+1)/3;
    } else {
        *noofregentries = 1;
    }

    addr_entry = PHYMOD_STRTOK(dict_addr_entry, ",");
    position_index = 0;
    while (addr_entry != NULL) {
        plp_aperta_phymod_dump_rmv_pattern(addr_entry, '\'');
        plp_aperta_phymod_dump_rmv_pattern(addr_entry, ' ');
        plp_aperta_phymod_dump_rmv_pattern(addr_entry, '{');
        plp_aperta_phymod_dump_rmv_pattern(addr_entry, '}');
        addr_entry_arrray[position_index++] = addr_entry;
        addr_entry = PHYMOD_STRTOK(NULL, ",");
    }

    for (i = 0; i < *noofregentries; i++) {
        if (PHYMOD_STRLEN(addr_entry_arrray[i*3]) < 100) {
            PHYMOD_STRCPY(disp_reg_addr_ptr[i],addr_entry_arrray[i*3]);
        } else {
            PHYMOD_STRCPY(disp_reg_addr_ptr[i],"size exceeds");
        }
    }

    if (comma_count > 1) {
        for (i = 0; i < *noofregentries; i++) {
            bit_pos[i]  = (int)PHYMOD_STRTOL(addr_entry_arrray[i*3 + 1], NULL, 10);
            noofbits[i] = (int)PHYMOD_STRTOL(addr_entry_arrray[i*3 + 2], NULL, 10);
        }
    }

    return;
}

int plp_aperta_pattern_count (char pattern, char *ip_line)
{
    int i = 0;
    int pat_count = 0;

    for (i = 0; i < PHYMOD_STRLEN(ip_line); i++) {
        if (ip_line[i] == pattern) {
            pat_count++;
        }
    }

    return pat_count;
}

long long unsigned int plp_aperta_get_disp_entry_val (char *base_addr, char *** lookup, int lookup_noofrows, int bit_pos, int noofbits, int *entry_found) {

    int i = 0;
    long long unsigned int disp_entry_val = 0xFFFF;
    long long unsigned int disp_entry_mask = 0;
    long long unsigned int msb_pos = 0;
    char* lookup_addr;
    char* lookup_val;

    lookup_addr = "\0";
    lookup_val  = "\0";

    *entry_found = 0;
    msb_pos = bit_pos+noofbits-1;

    if (msb_pos == 31 || (msb_pos == 63)) {
        if (msb_pos == 31) {
            disp_entry_mask = 0xFFFFFFFF;
        } else {
            disp_entry_mask = 0xFFFFFFFFFFFFFFFFULL;
        }
    } else {
        disp_entry_mask = ~((~(unsigned long long)0) << (msb_pos-bit_pos+1));
    }
    for (i = 0; i < lookup_noofrows; i++) {
        lookup_addr = lookup[i][0];
        if (PHYMOD_STRSTR(lookup_addr,base_addr) != NULL) {
            *entry_found = 1;
            lookup_val = lookup[i][1];
            plp_aperta_phymod_dump_rmv_pattern(lookup_val, '-');
            disp_entry_val = (long long unsigned int)PHYMOD_STRTOLL(lookup_val, NULL, 16);
            disp_entry_val = disp_entry_val >> bit_pos;
            disp_entry_val = disp_entry_val & disp_entry_mask;
        }
    }
    if (*entry_found == 1) {
        return disp_entry_val;
    } else {
        return 0;
    }
}

void plp_aperta_phymod_dump_extract_entry_vals (char *ip_line, char port_info, char side_info, char *** entry_vals, int *noofreg_entries, int *pm_cdmib_entry) 
{
    int i = 0;
    int noofentries = 0;
    char *entry_val;
    char *entries_array[17];
    unsigned int base_address[16]; 
    char cdmib_addr[6] = "CDMIB";
    char spdid_addr[6] = "SPDID";
    char line_entry_blkname[6];
    char line_entry_blkname1[6];
    int  use_blkname = 0;
    char blk_addr[2] = "\0";
    char eip_218_ingress_upper_addr[7]  = "0x2100";
    char eip_218_egress_upper_addr[7]   = "0x2200";
    char eip_163_ingress_upper_addr[7]  = "0x4300";
    char eip_164_ingress_upper_addr[7]  = "0x4400";
    char eip_163_egress_upper_addr[7]   = "0x4500";
    char eip_164_egress_upper_addr[7]   = "0x4600";
    char line_entry_upper_base_addr[7] = "\0";
    unsigned int strt_base_addr = 0;

    int  addr_inc_val = 0;
    entry_val = "\0";
    for (i = 0; i < 17; i++) {
        entries_array[i] = "\0";
    }
    for (i = 0; i < 16; i++) {
        base_address[i] = 0;
    }

    for (i = 0, noofentries = 0; ip_line[i]; i++) {
        noofentries += (ip_line[i] == 'x');
    }

    i = 0;
    entry_val = PHYMOD_STRTOK (ip_line, " ");
    while (entry_val != NULL) {
        entries_array[i++] = entry_val;
        entry_val = PHYMOD_STRTOK(NULL, " ");
    }

    PHYMOD_STRNCPY(line_entry_blkname, entries_array[0], 5);
    line_entry_blkname[5] = '\0';
    PHYMOD_STRNCPY(line_entry_blkname1, entries_array[0], 5);
    line_entry_blkname1[5] = '\0';
    PHYMOD_STRNCPY(line_entry_upper_base_addr, entries_array[0], 6);
    line_entry_upper_base_addr[6] = '\0';

    if ((PHYMOD_STRCMP(line_entry_blkname1, cdmib_addr) == 0) ||
            (PHYMOD_STRCMP(line_entry_blkname1, spdid_addr) == 0)) {
        use_blkname = 1;  
        *noofreg_entries = noofentries;
        *pm_cdmib_entry = 1;
    } else {
        use_blkname = 0;  
        *noofreg_entries = noofentries - 1;
        *pm_cdmib_entry = 0;
    }

    if ((PHYMOD_STRCMP(line_entry_upper_base_addr, eip_218_ingress_upper_addr) == 0) ||
            (PHYMOD_STRCMP(line_entry_upper_base_addr, eip_218_egress_upper_addr)  == 0) ||
            (PHYMOD_STRCMP(line_entry_upper_base_addr, eip_163_ingress_upper_addr) == 0) ||
            (PHYMOD_STRCMP(line_entry_upper_base_addr, eip_164_ingress_upper_addr) == 0) ||
            (PHYMOD_STRCMP(line_entry_upper_base_addr, eip_163_egress_upper_addr)  == 0) ||
            (PHYMOD_STRCMP(line_entry_upper_base_addr, eip_164_egress_upper_addr)  == 0)) {
        addr_inc_val = 4;  
    } else {
        addr_inc_val = 1;  
    }

    if (use_blkname == 1) {
        plp_aperta_phymod_dump_get_blk_addr(ip_line, blk_addr);
        for (i = 0; i < noofentries; i++) {
            PHYMOD_SPRINTF(entry_vals[i][0], "%s[%s.%d]", line_entry_blkname, blk_addr, i);
            PHYMOD_SPRINTF(entry_vals[i][1], "%s", entries_array[i+1]);
        }
    } else {
        strt_base_addr = (int)PHYMOD_STRTOL(entries_array[0], NULL, 16);
        base_address[0] = strt_base_addr;
        for (i = 1; i < noofentries - 1; i++) {
            base_address[i] = base_address[i-1] + addr_inc_val;
        }
        for (i = 0; i < *noofreg_entries; i++) {
            if (port_info != 'N') {
                PHYMOD_SPRINTF(entry_vals[i][0], "%x.%c", base_address[i],port_info);
            } else {
                PHYMOD_SPRINTF(entry_vals[i][0], "%x", base_address[i]);
            }
            PHYMOD_SPRINTF(entry_vals[i][1], "%s", entries_array[i+1]);
        }
    }
}

void plp_aperta_phymod_dump_get_blk_addr (char *ip_line, char *blk_addr) {
    int i = 0;
    int j = 0;
    int open_sqr_bracket_position = 0;
    int close_sqr_bracket_position = 0;

    for (i = 0; i < PHYMOD_STRLEN(ip_line); i++) {
        if (ip_line[i] == '[') {
            open_sqr_bracket_position = i;
        }
        if (ip_line[i] == ']') {
            close_sqr_bracket_position = i;
        }
    }

    j = 0;
    for (i = open_sqr_bracket_position + 1; i < close_sqr_bracket_position; i++) {
        blk_addr[j] = ip_line[i];
        j++;
    }
    blk_addr[j] = '\0';

    return; 
}

void plp_aperta_phymod_dump_rmv_comment_inline (char *ip_line) 
{
    int i = 0;
    for (i = 0; i < PHYMOD_STRLEN(ip_line); i++) {
        if (ip_line[i] == ' ' && ip_line[i+1] == '/' && ip_line[i+2] == '/') {
            ip_line[i] = '\0';
            return;
        }
    }
    return;
}

void plp_aperta_phymod_dump_rmv_dieinfo (char *ip_line) 
{
    int i = 0;
    int j = 0;
    int line_length = 0;
    line_length = PHYMOD_STRLEN(ip_line);

    for (i = 2; i < line_length; i++) {
        if (i == line_length - 1) {
            ip_line[j] = '\0';
            return;
        } else {
            ip_line[j] = ip_line[i];
            j++;
        }
    }
    return;
}

void plp_aperta_phymod_dump_rmv_colon (char *ip_line) 
{
    int i = 0;
    int j = 0;
    int colon_position = 0;

    int line_length = 0;
    line_length = PHYMOD_STRLEN(ip_line);

    for (i = 0; i < line_length; i++) {
        if (ip_line[i] == ':') {
            colon_position = i;
        }
    }
    j = colon_position;
    for (i = colon_position + 2; i < line_length; i++) {
        if (i == line_length-1) {
            ip_line[j] = ip_line[i];
            ip_line[j+1] = '\0';
            return;
        } else {
            ip_line[j] = ip_line[i];
            j++;
        }
    }
    return;
}  

char plp_aperta_rmv_sideinfo (char *ip_line) 
{
    int i = 0;
    int j = 0;
    int parethesis_position = 0;

    char side_info = '\0';

    int line_length = 0;
    line_length = PHYMOD_STRLEN(ip_line);

    for (i = 0; i < line_length; i++) {
        if (ip_line[i] == '(') {
            parethesis_position = i;
        }
    }
    j = parethesis_position;
    side_info = ip_line[parethesis_position + 1];

    for (i = parethesis_position + 3; i < line_length; i++) {
        if (i == line_length-2) {
            ip_line[j] = ip_line[i];
            ip_line[j+1] = ip_line[i+1];
            ip_line[j+2] = '\0';
            return side_info;
        } else {
            ip_line[j] = ip_line[i];
            j++;
        }
    }
    return side_info;
}  

char plp_aperta_rmv_portinfo (char *ip_line) {
    int i = 0;
    int j = 0;
    int dot_position = 0;
    char port_info = '\0';
    int line_length = 0;
    line_length = PHYMOD_STRLEN(ip_line);

    for (i = 0; i < line_length; i++) {
        if (ip_line[i] == '.') {
            dot_position = i;
        }
    }
    j = dot_position;
    port_info = ip_line[dot_position + 1];

    for (i = dot_position + 2; i < line_length; i++) {
        if (i == line_length-1) {
            ip_line[j] = ip_line[i];
            ip_line[j+1] = '\0';
            return port_info;
        } else {
            ip_line[j] = ip_line[i];
            j++;
        }
    }
    return port_info;
}

int plp_aperta_check_l2_line (char *ip_line) 
{
    int comment_position = 0;
    char comment_string[3] = "//";
    char l2_string[3]      = "L2";
    char only_l2_comment[6]      = "// L2";
    char line_comment[100] = "\0";

    if (PHYMOD_STRSTR(ip_line, comment_string)) {
        PHYMOD_STRCPY(line_comment, "\0");
        plp_aperta_phymod_dump_extract_line_comment(ip_line, &comment_position, line_comment);
        if (PHYMOD_STRSTR(line_comment, only_l2_comment)) {
            ip_line[comment_position - 1] = '\n';
            ip_line[comment_position] = '\0';
        }
        if (PHYMOD_STRSTR(line_comment, l2_string)) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

void plp_aperta_phymod_dump_extract_line_comment (char *ip_line, int *comment_position, char *line_comment) 
{
    int i, j = 0;
    char comment_string[3] = "//";
    int line_length = 0;
    PHYMOD_STRCPY(line_comment, "\0");
    line_length = PHYMOD_STRLEN(ip_line);
    if (PHYMOD_STRSTR(ip_line, comment_string)) {
        for (i = 0; i < line_length; i++) {
            if ((ip_line[i] == '/') && (ip_line[i+1] == '/')) {
                *comment_position = i;
            }
        }
        j = 0;
        for (i = *comment_position; i < line_length; i++) {
            line_comment[j++] = ip_line[i];
        }
        line_comment[j] = '\0';
        plp_aperta_phymod_dump_rplc_multiplespace_with_singlespace(line_comment);
    }

    return;
}

void plp_aperta_phymod_dump_rplc_multiplespace_with_singlespace (char *ip_line) 
{
    char *dst = ip_line;

    for (; *ip_line; ++ ip_line) {
        *dst++ = *ip_line;
        if (PHYMOD_ISSPACE(*ip_line)) {
            do {
                ++ip_line;
            } while (PHYMOD_ISSPACE(*ip_line));
            --ip_line;
        }
    }
    *dst = 0;
}

void plp_aperta_phymod_dump_rmv_strtng_whitespace (char *ip_line) 
{
    int i, j = 0;
    for (i = 0; ip_line[i] == ' ' || ip_line[i] == '\t'; i++);

    for (j = 0; ip_line[i]; i++) {
        ip_line[j++] = ip_line[i];
    }
    ip_line[j] = '\0';
}

void plp_aperta_phymod_dump_rmv_pattern (char *ip_line, char pattern) 
{
    int i = 0;
    int j = 0;
    while (ip_line[i] != '\0') {
        if (!(ip_line[i] == pattern)) {
            ip_line[j] = ip_line[i];
            j++;
        }
        i++;
    }
    ip_line[j] = '\0';
}
#endif /* PHYMOD_APERTA_SUPPORT */
