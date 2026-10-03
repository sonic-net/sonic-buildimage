/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <esmi_oob/esmi_cpuid_msr.h>
#include <esmi_oob/apml.h>
#include <esmi_oob/esmi_rmi.h>

/* Default message lengths as per APML command protocol */
/* CPUID extended function for max threads per l3 */
#define THREADS_L3_EXTD         0x3
#define MSR_RD_LEN	0xa
#define MSR_WR_LEN	0x9
#define CPUID_RD_LEN	0xa
#define CPUID_WR_LEN	0xa
#define STATUS_RD_LEN	0xa
#define STATUS_WR_LEN	0x3
#define PRI_WR_LEN	0x1

/* Size of CPU ID reg in bytes */
#define REG_SIZE 4
/* Mask to check H/W Alert status bit */
#define HW_ALERT_MASK	0x80
/* Thread Mask */
#define THREAD_MASK	0xFFFF
/* Legacy platforms(Milan & Rome) threads per socket */
#define LEGACY_PLAT_THREADS_PER_SOC 128
/* CPUID function for max threads per l3 */
#define THREADS_L3_FUNC         0x8000001D

static oob_status_t esmi_convert_reg_val(uint32_t reg, char *id)
{
	int i;
	char id_l[REG_SIZE + 1];
	oob_status_t ret;

	for (i = 0; i < (sizeof(id_l) - 1) ; i++) {
		id_l[i] = reg;
		reg = reg >> 8;
	}

	id_l[i] = '\0';

	ret = snprintf(id, sizeof(id_l), "%s", id_l);
	if (ret < 0)
		return ret;

	return 0;

}

oob_status_t esmi_get_vendor_id(uint8_t soc_num,
				char *vendor_id)
{
	uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
	uint32_t core_id = 0;
	char ebx_id[REG_SIZE + 1], ecx_id[REG_SIZE + 1], edx_id[REG_SIZE + 1];
	oob_status_t ret;

	if (!vendor_id)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_cpuid(soc_num, core_id,
			     &eax, &ebx, &ecx, &edx);
	if (ret)
		return ret;
	/*
	 * Processor Vendor EBX [(ASCII Bytes [3:0])]
	 * Processor Vendor ECX [(ASCII Bytes [8:11])]
	 * Processor Vendor EDX [(ASCII Bytes [4:7])]
	 */
	ret = esmi_convert_reg_val(ebx, ebx_id);
	if (ret)
		return ret;
	ret = esmi_convert_reg_val(edx, edx_id);
	if (ret)
		return ret;
	ret = esmi_convert_reg_val(ecx, ecx_id);
	if (ret)
		return ret;

	ret = snprintf(vendor_id, 4 * REG_SIZE + 1, "%s%s%s",
			     ebx_id, edx_id, ecx_id);
	if (ret < 0)
		return ret;

	return 0;
}

static oob_status_t esmi_reg_offset_conv(uint32_t reg, uint32_t offset,
					 uint32_t flag)
{
	return (reg >> offset) & flag;
}

oob_status_t esmi_get_processor_info(uint8_t soc_num,
				     struct processor_info *proc_info)
{

	oob_status_t ret;
	uint32_t eax = 1, ebx, ecx = 0, edx;
	uint32_t core_id = 0;

	if (!proc_info)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_cpuid(soc_num, core_id,
			     &eax, &ebx, &ecx, &edx);
	if (ret != 0)
		return ret;
	/*
	 * Family[7:0]=({0000b,BaseFamily[3:0]}+ExtendedFamily[7:0])
	 */
	proc_info->family = esmi_reg_offset_conv(eax, 8, 0xf) +
			esmi_reg_offset_conv(eax, 20, 0xff);
	/*
	 * Model[7:0]={ExtendedModel[3:0],BaseModel[3:0]}
	 */
	proc_info->model = esmi_reg_offset_conv(eax, 16, 0xf) * 0x10 +
			esmi_reg_offset_conv(eax, 4, 0xf);
	/*
	 * Stepping = Processor stepping (revision) for a specific model
	 */
	proc_info->step_id = esmi_reg_offset_conv(eax, 0, 0xf);
	return ret;
}

oob_status_t esmi_get_threads_per_core(uint8_t soc_num,
				       uint32_t *threads_per_core)
{
	oob_status_t ret;
	uint32_t value;
	uint32_t thread_ind = 0;
	uint32_t cpuid_fn = 1;
	uint32_t cpuid_extd_fn = 0;

	if (!threads_per_core)
		return OOB_ARG_PTR_NULL;

	cpuid_fn = 0x8000001e; // CPUID_Fn8000001E_EBX [Core Identifiers]
	ret = esmi_oob_cpuid_ebx(soc_num, thread_ind, cpuid_fn,
				 cpuid_extd_fn, &value);
	if (ret != OOB_SUCCESS)
		return ret;
	/*
	 * bits 15:8 Threads per core. Read-only.
	 * Reset: XXh. The number of threads per core is ThreadsPerCore+1.
	 */
	*threads_per_core = ((value >> 8) & 0xFF) + 1;

	return ret;
}

oob_status_t
esmi_get_logical_cores_per_socket(uint8_t soc_num,
				  uint32_t *logical_cores_per_socket)
{
	oob_status_t ret;
	uint32_t value;
	uint32_t thread_ind = 0;
	uint32_t cpuid_fn = 1;
	uint32_t cpuid_extd_fn = 0;

	if (!logical_cores_per_socket)
		return OOB_ARG_PTR_NULL;

	/*
	 * CPUID_Fn0000000B_EBX_x01 [Extended Topology Enumeration]
	 */
	cpuid_fn = 0xB;
	cpuid_extd_fn = 1;
	ret = esmi_oob_cpuid_ebx(soc_num, thread_ind, cpuid_fn,
				 cpuid_extd_fn, &value);
	if (ret != OOB_SUCCESS)
		return ret;
	*logical_cores_per_socket = value & 0xFFFF;

	return ret;
}

/* Thread > 127, Thread128 CS register, 1'b1 needs to be set to 1 */
static oob_status_t esmi_oob_extend_thread(uint8_t soc_num, uint32_t *thread)
{
	uint8_t val = 0;

	if (*thread > 127) {
		*thread -= 128;
		val = 1;
	}
	return esmi_oob_rmi_write_byte(soc_num, SBRMI_THREAD128CS, val);
}

static oob_status_t validate_thread(uint8_t soc_num, uint32_t thread_num)
{
	uint32_t max_threads_per_soc = 0;
	uint8_t rev = 0;
	oob_status_t ret;

	ret = read_sbrmi_revision(soc_num, &rev);
	if (ret)
		return ret;
	if (rev != 0x10) {
		ret = esmi_get_threads_per_socket(soc_num, &max_threads_per_soc);
		if (ret)
			return ret;
	} else {
		max_threads_per_soc = LEGACY_PLAT_THREADS_PER_SOC;
	}

	if (thread_num > (max_threads_per_soc - 1))
		return OOB_CPUID_MSR_CMD_INVAL_THREAD;

	return OOB_SUCCESS;
}

oob_status_t esmi_oob_read_msr(uint8_t soc_num,
			       uint32_t thread, uint32_t msraddr,
			       uint64_t *buffer)
{
	struct apml_message msg = {0};
	uint8_t index = 0;
	oob_status_t ret;

	/* NULL pointer check */
	if (!buffer)
		return OOB_ARG_PTR_NULL;
	/* cmd for MCAMSR is 0x1001 */
	msg.cmd = 0x1001;
	msg.data_in.cpu_msr_in = msraddr;

	/* validate thread */
	ret = validate_thread(soc_num, thread);
	if (ret)
		return ret;

	/* Assign thread number to data_in[4:5] */
	msg.data_in.cpu_msr_in = msg.data_in.cpu_msr_in
				 | ((uint64_t)thread << 32);

	/* Assign 7 byte to READ Mode */
	msg.data_in.reg_in[7] = 1;
	ret = sbrmi_xfer_msg(soc_num, &msg);
	if (ret)
		return ret;

	*buffer = msg.data_out.cpu_msr_out;

	return OOB_SUCCESS;
}

oob_status_t esmi_oob_cpuid(uint8_t soc_num, uint32_t thread,
			    uint32_t *eax, uint32_t *ebx,
			    uint32_t *ecx, uint32_t *edx)
{
	uint32_t fn_eax, fn_ecx;
	oob_status_t ret;

	fn_eax = *eax;
	fn_ecx = *ecx;

	/* validate thread */
	ret = validate_thread(soc_num, thread);
	if (ret)
		return ret;

	ret = esmi_oob_cpuid_eax(soc_num, thread, fn_eax, fn_ecx, eax);
	if (ret)
		return ret;

	ret = esmi_oob_cpuid_ebx(soc_num, thread, fn_eax, fn_ecx, ebx);
	if (ret)
		return ret;

	ret = esmi_oob_cpuid_ecx(soc_num, thread, fn_eax, fn_ecx, ecx);
	if (ret)
		return ret;

	return esmi_oob_cpuid_edx(soc_num, thread, fn_eax, fn_ecx, edx);
}

static oob_status_t esmi_oob_cpuid_fn(uint8_t soc_num, uint32_t thread,
				      uint32_t fn_eax, uint32_t fn_ecx,
				      uint8_t mode, uint32_t *value)
{
	struct apml_message msg = {0};
	uint8_t ext = 0, read_reg = 0;
	oob_status_t ret;

	if (!value)
		return OOB_ARG_PTR_NULL;

	/* cmd for CPUID is 0x1000 */
	msg.cmd = 0x1000;
	msg.data_in.cpu_msr_in = fn_eax;

	/* Assign thread number to data_in[4:5] */
	msg.data_in.cpu_msr_in = msg.data_in.cpu_msr_in
				 | ((uint64_t)thread << 32);

	/* Assign extended function to data_in[6][4:7] */
	if (mode == EAX || mode == EBX)
		/* read eax/ebx */
		read_reg = 0;
	else
		/* read ecx/edx */
		read_reg = 1;

	ext = (uint8_t)fn_ecx;
        ext = ext << 4 | read_reg;
        msg.data_in.cpu_msr_in = msg.data_in.cpu_msr_in | ((uint64_t) ext << 48);
	/* Assign 7 byte to READ Mode */
	msg.data_in.reg_in[7] = 1;
        ret = sbrmi_xfer_msg(soc_num, &msg);
        if (ret)
                return ret;

	if (mode == EAX || mode == ECX)
		/* Read low word/mbout[0] */
		*value = msg.data_out.mb_out[0];
	else
		/* Read high word/mbout[1] */
		*value = msg.data_out.mb_out[1];

	return OOB_SUCCESS;
}


/*
 * CPUID functions returning a single datum
 */
oob_status_t esmi_oob_cpuid_eax(uint8_t soc_num,
				uint32_t thread, uint32_t fn_eax,
				uint32_t fn_ecx, uint32_t *eax)
{
	return esmi_oob_cpuid_fn(soc_num, thread, fn_eax, fn_ecx,
				 EAX, eax);
}

oob_status_t esmi_oob_cpuid_ebx(uint8_t soc_num,
				uint32_t thread, uint32_t fn_eax,
				uint32_t fn_ecx, uint32_t *ebx)
{
	return esmi_oob_cpuid_fn(soc_num, thread, fn_eax, fn_ecx,
				 EBX, ebx);
}

oob_status_t esmi_oob_cpuid_ecx(uint8_t soc_num,
				uint32_t thread, uint32_t fn_eax,
				uint32_t fn_ecx, uint32_t *ecx)
{
        return esmi_oob_cpuid_fn(soc_num, thread, fn_eax, fn_ecx,
				 ECX, ecx);
}

oob_status_t esmi_oob_cpuid_edx(uint8_t soc_num,
				uint32_t thread, uint32_t fn_eax,
				uint32_t fn_ecx, uint32_t *edx)
{
        return esmi_oob_cpuid_fn(soc_num, thread, fn_eax, fn_ecx,
				 EDX, edx);
}

oob_status_t read_max_threads_per_l3(uint8_t soc_num, uint32_t *threads_l3)
{
	uint32_t thread;
	oob_status_t ret;

	/* Get maximum threads per l3 */
	thread = 0;
	ret = esmi_oob_cpuid_eax(soc_num, thread, THREADS_L3_FUNC,
				 THREADS_L3_EXTD, threads_l3);
	if (!ret)
		*threads_l3 = ((*threads_l3 >> 14) & 0xFFF) + 1;

	return ret;
}
