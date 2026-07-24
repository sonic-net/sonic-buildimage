/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <esmi_oob/esmi_cpuid_msr.h>
#define ARGS_MAX 64

static uint32_t eax;
static uint32_t ebx;
static uint32_t ecx;
static uint32_t edx;

static void print_uint32(uint32_t val)
{
	char ch;
	int i;

	for (i = 0; i < 4; i++) {
		ch = val;
		printf("%c", ch);
		val = val >> 8;
	}
}

static oob_status_t read_cupid_fn00000000(uint8_t soc_num, int core_id)
{
	oob_status_t ret;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	ret = esmi_oob_cpuid(soc_num, core_id,
			     &eax, &ebx, &ecx, &edx);
	if (ret != 0)
		return ret;

	printf("CPU 0:\n");
	printf("   vendor_id = \"");
	print_uint32(ebx);
	print_uint32(edx);
	print_uint32(ecx);
	printf("\"\n");

	return ret;
}

static oob_status_t read_cupid_fn00000001(uint8_t soc_num,
					  int core_id)
{
	uint32_t res;
	oob_status_t ret;

	eax = 1;
	ecx = 0;
	ret = esmi_oob_cpuid(soc_num, core_id,
			     &eax, &ebx, &ecx, &edx);
	if (ret != 0)
		return ret;

	printf("   version information(1/eax):\n");
	res = ((eax >> 8) & 0xf) + ((eax >> 20) & 0xff);
	printf("      family\t\t= 0x%x (%d)\n", res, res);
	res = ((eax >> 16) & 0xf) * 0x10 + ((eax >> 4) & 0xf);
	printf("      model\t\t= 0x%x (%d)\n", res, res);
	res = eax & 0xf;
	printf("      stepping id\t= 0x%x (%d)\n", res, res);
	res = ((eax >> 20) & 0xff);
	printf("      extended family\t= 0x%x (%d)\n", res, res);
	res = ((eax >> 8) & 0xf);
	printf("      base family\t= 0x%x (%d)\n", res, res);
	res = ((eax  >> 16) & 0xf);
	printf("      extended model\t= 0x%x (%d)\n", res, res);
	res = ((eax >> 4) & 0xf);
	printf("      base model\t= 0x%x (%d)\n", res, res);

	printf("   miscellaneous(1/ebx):\n");
	res = ((ebx >> 24) & 0xff);
	printf("      process local APIC physical ID\t= 0x%x (%d)\n", res, res);
	res = ((ebx >> 16) & 0xff);
	printf("      cpu count\t\t\t\t= 0x%x (%d)\n", res, res);
	res = ((ebx >> 8) & 0xff);
	printf("      CLFLUSH line size\t\t\t= 0x%x (%d)\n", res, res);

	printf("   feature information(1/edx):\n");
	printf("      FPU\t\t= %s\n", edx & 1 ? "true" : "false");
	printf("      VME\t\t= %s\n", (edx >> 1) & 1 ? "true" : "false");
	printf("      DE\t\t= %s\n", (edx >> 2) & 1 ? "true" : "false");
	printf("      PSE\t\t= %s\n", (edx >> 3) & 1 ? "true" : "false");
	printf("      TSC\t\t= %s\n", (edx >> 4) & 1 ? "true" : "false");
	printf("      MSR\t\t= %s\n", (edx >> 5) & 1 ? "true" : "false");
	printf("      PAE\t\t= %s\n", (edx >> 6) & 1 ? "true" : "false");
	printf("      MCE\t\t= %s\n", (edx >> 7) & 1 ? "true" : "false");
	printf("      CMPXCHG8B\t\t= %s\n", (edx >> 8) & 1 ? "true" : "false");
	printf("      APIC\t\t= %s\n", (edx >> 9) & 1 ? "true" : "false");
	printf("      SysEnterSysExit\t= %s\n", edx >> 11 & 1 ? "true" : "false");
	printf("      MTRR\t\t= %s\n", (edx >> 12) & 1 ? "true" : "false");
	printf("      PGE\t\t= %s\n", (edx >> 13) & 1 ? "true" : "false");
	printf("      MCA\t\t= %s\n", (edx >> 14) & 1 ? "true" : "false");
	printf("      CMOV\t\t= %s\n", (edx >> 15) & 1 ? "true" : "false");
	printf("      PAT\t\t= %s\n", (edx >> 16) & 1 ? "true" : "false");
	printf("      PSE36\t\t= %s\n", (edx >> 17) & 1 ? "true" : "false");
	printf("      CLFSH\t\t= %s\n", (edx >> 19) & 1 ? "true" : "false");
	printf("      MMX\t\t= %s\n", (edx >> 23) & 1 ? "true" : "false");
	printf("      FXSR\t\t= %s\n", (edx >> 24) & 1 ? "true" : "false");
	printf("      SSE\t\t= %s\n", (edx >> 25) & 1 ? "true" : "false");
	printf("      SSE2\t\t= %s\n", (edx >> 26) & 1 ? "true" : "false");
	printf("      HTT\t\t= %s\n", (edx >> 28) & 1 ? "true" : "false");

	printf("   feature information(1/ecx):\n");
	printf("      SSE3\t\t\t= %s\n", ecx & 1 ? "true" : "false");
	printf("      PCLMULQDQ\t\t\t= %s\n", (ecx >> 1) & 1 ? "true" : "false");
	printf("      Monitor\t\t\t= %s\n", (ecx >> 3) & 1 ? "true" : "false");
	printf("      SSSE3\t\t\t= %s\n", (ecx >> 9) & 1 ? "true" : "false");
	printf("      FMA\t\t\t= %s\n", (ecx >> 12) & 1 ? "true" : "false");
	printf("      CMPXCHG16B\t\t= %s\n", (ecx >> 13) & 1 ? "true" : "false");
	printf("      SSE41\t\t\t= %s\n", (ecx >> 19) & 1 ? "true" : "false");
	printf("      SSE42\t\t\t= %s\n", (ecx >> 20) & 1 ? "true" : "false");
	printf("      X2APIC\t\t\t= %s\n", (ecx >> 21) & 1 ? "true" : "false");
	printf("      MOVBE\t\t\t= %s\n", (ecx >> 22) & 1 ? "true" : "false");
	printf("      POPCNT\t\t\t= %s\n", (ecx >> 23) & 1 ? "true" : "false");
	printf("      AES instruction support \t= %s\n", (ecx >> 25) & 1 ? "true" : "false");
	printf("      XSAVE\t\t\t= %s\n", (ecx >> 26) & 1 ? "true" : "false");
	printf("      OSXSAVE\t\t\t= %s\n", (ecx >> 27) & 1 ? "true" : "false");
	printf("      AVX\t\t\t= %s\n", (ecx >> 28) & 1 ? "true" : "false");
	printf("      F16C\t\t\t= %s\n", (ecx >> 29) & 1 ? "true" : "false");
	printf("      RDRAND\t\t\t= %s\n", (ecx >> 30) & 1 ? "true" : "false");

	return ret;
}

static void show_usage(char *exe_name)
{
	printf("Usage: %s  [-b] soc_num \n"
		"Where:  soc_num : socket Index starting from 0\n", exe_name);
}

static void rerun_sudo(int argc, char **argv)
{
	static char *args[ARGS_MAX];
	char sudostr[] = "sudo";
	int i;

	args[0] = sudostr;
	for (i = 0; i < argc; i++)
		args[i + 1] = argv[i];

	args[i + 1] = NULL;
	execvp("sudo", args);
}

/**
Main program.
@param argc number of command line parameters
@param argv list of command line parameters
*/
int main(int argc, char **argv)
{
	uint32_t core_id = 0;
	uint8_t force_flag, bus_type;
	uint8_t soc_num;
	char *end;
	int ret, opt, long_index = 0;
	char *helperstring = "+hb:";

	static struct option long_options[] = {
		{"help",	no_argument,		0,	'h'},
		{"bus",		required_argument,	0,      'b'},
		{0,		0,			0,	  0},
	};

	if (argc <= 1) {
		show_usage(argv[0]);
		return 0;
	}

	if (getuid() != 0)
		rerun_sudo(argc, argv);

	while ((opt = getopt_long(argc, argv,  helperstring, long_options, &long_index)) != EOF) {
		switch (opt) {
		case 'h':
			show_usage(argv[0]);
			return OOB_SUCCESS;
		case 'b':
			soc_num = strtoul(optarg, &end, 0);
			break;
		default:
			show_usage(argv[0]);
			return OOB_INVALID_INPUT;
		}
	}

	ret = read_cupid_fn00000000(soc_num, core_id);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to get addr[0x0] cpuid info, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	ret = read_cupid_fn00000001(soc_num, core_id);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to get addr[0x1] cpuid info, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}

	printf("\n");
	/* Normal program termination */
	return 0;
}
