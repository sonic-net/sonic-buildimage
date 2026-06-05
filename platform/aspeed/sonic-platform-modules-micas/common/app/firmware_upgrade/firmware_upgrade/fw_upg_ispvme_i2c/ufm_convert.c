#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "debug.h"

#include "xo2_dev.h"
#include "systypes.h"
#include "ufm_0123.h"

#define TRUE 1
#define FALSE 0

#define MAX_LINE 1023
#define STX 0x02
#define ETX 0x03

enum
{
    COMMENT,
    FUSE_CHECKSUM,
    FUSE_DATA,
    END_DATA,
    FUSE_LIST,
    SECURITY_FUSE,
    FUSE_DEFAULT,
    FUSE_SIZE,
    USER_CODE,
    FEATURE_ROW,
    DONE,
    OTHER
};

/*===============================================================================
 These are copied from the driver code so sizes/enums match
===============================================================================*/

void    convertFeatureRowToHexArray(char *p, unsigned char *pout, int cnt);
void    convertFuseToHexArray(char *p, unsigned char *pout, int cnt);
void    findDeviceType(char *pS, XO2Devices_t *pDevID);

int  convertBinaryField(char *p, unsigned int *pVal)
{
    unsigned int val;
    unsigned int i;

    val = 0;
    i = 0;
    while ((*p != '\0') && (i < 32)) {
        val = (val<<1) | (*p - '0');
        ++i;
    }
    *pVal = val;
    return(i/8);
}

void removeLastStar(char *p)
{
    int i;
    i = 0;
    while (*p != '\0') {
        ++i;
        ++p;
    }

    if (i == 0) {
        return;  /* empty string */
    }

    while ((*p != '*') && i) {
        --p;
        --i;
    }
    if (i) {
        *p = '\0';  /* replace last '*' with NULL, end line at that spot */
    }
}

int ufm_convert_jed(char *jed_file, XO2Devices_t cpld_type, XO2_JEDEC_t *pjedec_t)
{
    FILE *fpIn;
    char line[MAX_LINE + 1];
    int pageCnt, UFMPageCnt, CfgPageCnt;
    int done;
    int state;
    int fuse_addr;
    int fuse_size;
    int erase_val;
    int security_fuse;
    unsigned int userCode;
    XO2Devices_t devID;

    devID = cpld_type;

    fpIn = fopen(jed_file, "r");
    if (fpIn == NULL) {
        perror("Opening input file");
        exit(-1);
    }

    fuse_addr = 0;
    pageCnt = 0;
    UFMPageCnt = 0;
    CfgPageCnt = 0;
    state = OTHER;
    security_fuse = 0;
    userCode = 0;
    if (fgets(line, MAX_LINE, fpIn) == NULL) {
        printf("ERROR!  Failed to read line from file!\nAborting.\n");
        fclose(fpIn);
        /*fclose(fpOut);*/
        exit(-3);
    }

    if (line[0] != STX) {
        printf("ERROR!  Expected STX as first char!\nAborting.\n");
        fclose(fpIn);
        /*fclose(fpOut);*/
        exit(-2);
    }

    done = FALSE;
    while (!feof(fpIn)  && !done) {
        if (fgets(line, MAX_LINE, fpIn) != NULL) {

            if ((line[0] == '0') || (line[0] == '1')) {
                state = FUSE_DATA;
            } else if (strncmp("NOTE", line, 4) == 0) {
                state = COMMENT;
            } else if (line[0] == 'G') {
                state = SECURITY_FUSE;
            } else if (line[0] == 'L') {
                state = FUSE_LIST;
            } else if (line[0] == 'C') {
                state = FUSE_CHECKSUM;
            } else if (line[0] == '*') {
                state = END_DATA;
            } else if (line[0] == 'D') {
                state = FUSE_DEFAULT;
            } else if (line[0] == 'U') {
                state = USER_CODE;
            } else if (line[0] == 'E') {
                state = FEATURE_ROW;
            } else if (strncmp("QF", line, 2) == 0) {
                state = FUSE_SIZE;
            } else if (line[0] == ETX) {
                state = DONE;
            }

            switch (state) {
                case FUSE_DATA:
                    ++pageCnt;
                    if ((pageCnt == 1) && (fuse_addr == 0)) {
                        /*fprintf(fpOut, "unsigned char %s_CfgData[] = { // Cfg Data Array\n", pName);*/
                    } else if (pageCnt == XO2DevList[devID].Cfgpages + 1) {
                        /*fprintf(fpOut, "};\n\nunsigned char %s_UFMData[] = { // UFM Data Array\n", pName);*/
                    }

                    if (pageCnt <= XO2DevList[devID].Cfgpages) {
                        ++CfgPageCnt;
                        convertFuseToHexArray(line, &pjedec_t->pCfgData[((CfgPageCnt - 1) * 16)], CfgPageCnt);
                    } else {
                        ++UFMPageCnt;
                        convertFuseToHexArray(line, &pjedec_t->pUFMData[((UFMPageCnt - 1) * 16)], UFMPageCnt);
                    }
                    break;

                case COMMENT:
                    removeLastStar(line);
                    /*fprintf(fpOut, "// %s\n", &line[5]);*/
                    break;

                case FUSE_LIST:
                    removeLastStar(line);
                    sscanf(&line[1], "%d", &fuse_addr);
                    if (fuse_addr == 0) {
                    /*
                    fprintf(fpOut, "#include \"XO2_dev.h\"   // for XO2_JEDEC_t def\n\n");
                    fprintf(fpOut, "// Flash Data Array Declarations\n");
                    */
                    }
                    break;

                case SECURITY_FUSE:
                    removeLastStar(line);
                        sscanf(&line[1], "%d", &security_fuse);
                        printf("Security Fuse: %x\n", security_fuse);
                    break;

                case FUSE_DEFAULT:
                    removeLastStar(line);
                    sscanf(&line[1], "%d", &erase_val);
                    if (erase_val != 0) {
                        printf("WARNING!  DEFAULT ERASE STATE NOT 0!\n");
                    }
                    break;

                case FUSE_SIZE:
                    removeLastStar(line);
                    sscanf(&line[2], "%d", &fuse_size);
                    break;

                case USER_CODE:   /* This is informational only.  USERCODE is part of Config sector */
                    removeLastStar(line);
                    if (line[1] == 'H') {
                        sscanf(&line[2], "%x", &userCode);
                    } else {
                        convertBinaryField(&line[1], &userCode);
                    }
                    break;

                case FEATURE_ROW:
                    /* 2 consectutive rows.  1st starts with E and is 64 bits.  2nd line is 16 bits, ends in */
                    /*
                    fprintf(fpOut, "\n// Feature Row Data Structure\n");
                    fprintf(fpOut, "XO2FeatureRow_t %s_FeatureRow = \n{\n", pName);
                    fprintf(fpOut, "\t{");
                    */
                    convertFeatureRowToHexArray(&line[1], pjedec_t->pFeatureRow->feature, 8);
                    /*fprintf(fpOut, "},  // Feature bits\n");*/

                    (void)fgets(line, MAX_LINE, fpIn);
                    removeLastStar(line);
                    /* fprintf(fpOut, "\t{"); */
                    convertFeatureRowToHexArray(&line[0], pjedec_t->pFeatureRow->feabits, 2);
                    /* fprintf(fpOut, "}  // FEABITS\n};\n\n"); */
                    break;

                case DONE:
                    done = TRUE;
                    break;

                case END_DATA:
                    /* fprintf(fpOut, "// *\n"); */
                    break;

                case FUSE_CHECKSUM:
                    /* fprintf(fpOut, "};  // End UFM Data\n"); */
                    break;

                default:
                    /* do nothing */
                    break;

            }

            /* Look for specific XO2 Device type and extract */
            if ((state == COMMENT)  && (strncmp("DEVICE NAME:", &line[5], 12) == 0)) {
                findDeviceType(&line[17], &devID);
            }
        }
    }

    pjedec_t->devID = devID;
    pjedec_t->pageCnt = pageCnt;
    pjedec_t->CfgDataSize = CfgPageCnt * 16;
    pjedec_t->UFMDataSize = UFMPageCnt * 16;
    pjedec_t->UserCode = userCode;
    pjedec_t->SecurityFuses = security_fuse;

    /* Close the C file that holds the byte data in the structure */
    fclose(fpIn);
    return 0;
}

/**
 * Convert a line of fuse data into a list of C byte values in an array.
 * This function is specific to the MachXO2 format of 128 bits per page (fuse row).
 * So it just processes knowing that there are 16 bytes per row.
 *
 * JEDEC file format is lsb first so shift into byte from top down.
 */
void    convertFuseToHexArray(char *p, unsigned char *pout, int cnt)
{
    unsigned char val;
    int i, j;

    for (i = 0; i < 16; i++) {
        val = 0;
        for (j = 0; j < 8; j++) {
            val = (val<<1) | (*p - '0');
            ++p;
        }
        pout[i] = val;
        /* fprintf(fpOut, " 0x%02x,", val);*/
    }
    /*fprintf(fpOut, "  // %d\n", cnt);*/
}

/**
 * Convert the E Field (Feature Row) into a list of C byte values in an array.
 * This function is specific to the MachXO2 Feature Row format.
 * The number of bytes in a row is passed in.
 * the bits are actually in reverse order so we need to work backwards through
 * the string.
 *
 * JEDEC file format is lsb first so shift into byte from top down.
 */
void    convertFeatureRowToHexArray(char *p, unsigned char *pout, int cnt)
{
    unsigned char val;
    int i, j;

    /* start at last char in string and work backwards */
    p = p + ((8 * cnt) - 1);

    for (i = 0; i < cnt; i++) {
        val = 0;
        for (j = 0; j < 8; j++) {
            val = (val<<1) | (*p - '0');
            --p;
        }
        pout[i] = val;
        /*fprintf(fpOut, " 0x%02x,", val);*/
    }

}

/* Still need to handle the U devices and their increased sizes. */
void    findDeviceType(char *pS, XO2Devices_t *pDevID)
{
    CPLD_I2C_VERBOS("XO2 Dev: %s\n", pS);

    if (strstr(pS, "LCMXO2-256") != NULL) {
        *pDevID = MachXO2_256;
    } else if (strstr(pS, "LCMXO2-640") != NULL) {
        *pDevID = MachXO2_640;
    } else if (strstr(pS, "LCMXO2-1200") != NULL) {
        *pDevID = MachXO2_1200;
    } else if (strstr(pS, "LCMXO2-2000") != NULL) {
        *pDevID = MachXO2_2000;
    } else if (strstr(pS, "LCMXO2-4000") != NULL) {
        *pDevID = MachXO2_4000;
    } else if (strstr(pS, "LCMXO2-7000") != NULL) {
        *pDevID = MachXO2_7000;
    } else if (strstr(pS, "LCMXO3LF-2100C") != NULL) {
        *pDevID = MachXO3_2100;
    } else if (strstr(pS, "LCMXO3LF-4300C") != NULL) {
        *pDevID = MachXO3_4300c;
    } else {
        *pDevID = MachXO2_1200;
    }
}
