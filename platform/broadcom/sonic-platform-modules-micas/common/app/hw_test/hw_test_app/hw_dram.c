#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/mount.h>
#include <stddef.h>
#include <hw_dram/fac_common.h>
#include <hw_dram/ft_ddr_test.h>

int platform_fac_dbg = GRTD_LOG_NONE;
static int do_mlock = 1;
static ulong mem_test_data1 = sizeof(ulong) == 4 ? 0x55555555UL : 0x5555555555555555ULL;
static ulong mem_test_data2 = sizeof(ulong) == 4 ? 0xAAAAAAAAUL : 0xAAAAAAAAAAAAAAAAULL;

int dram_wr_main(int argc, char **argv)
{
    printf("%s, %d, %d, %s\r\n", __FUNCTION__, __LINE__, argc, argv[0]);
    return 0;
}

int dram_rd_main(int argc, char **argv)
{
    int fd, i, ret;
    unsigned char buffer[1024];
    memset(buffer, 0, 1024);
    argc = argc;
    argv = argv;
    if ((fd = open("/dev/mem",O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO)) < 0 ) {
        printf("xxxx:%s\r\n", strerror(errno));
        return fd;
    }

    if ((ret = lseek(fd, 0x1f400000, SEEK_SET)) < 0) {
        printf("llseek:%s\r\n", strerror(errno));
        close(fd);
        return ret;
    }
    if ((ret = read(fd, buffer, 1024)) < 0) {
        printf("read:%s\r\n", strerror(errno));
        close(fd);
        return ret;
    }

    for (i = 0; i < 1024; i++) {
        printf("%02x ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\r\n");
        }
    }
    printf("\r\n");

    close(fd);
    return 0;
}

int dram_chk_main(int argc, char **argv)
{
    printf("%s, %d, %d, %s\r\n", __FUNCTION__, __LINE__, argc, argv[0]);
    return 0;
}

static int dram_test_arg_pars(int argc, char **argv, int *simple, int *all, int *debug)
{
    switch(argc) {
    case 1:
        *simple = 1;
        *all = 0;
        *debug = 0;
        return 0;
    case 2:
        if (strcmp(argv[1], "simple") == 0) {
            *simple = 1;
            *all = 0;
            *debug = 0;
        } else if (strcmp(argv[1], "complex") == 0) {
            *simple = 0;
            *all = 0;
            *debug = 0;
        } else if (strcmp(argv[1], "debug") == 0) {
            *simple = 1;
            *all = 0;
            *debug = 1;
        } else {
            break;
        }
        return 0;
    case 3:
        if (strcmp(argv[1], "simple") == 0 && strcmp(argv[2], "debug") == 0) {
            *simple = 1;
            *all = 0;
            *debug = 1;
        } else if (strcmp(argv[1], "complex") == 0) {
            if (strcmp(argv[2], "some") == 0) {
                *simple = 0;
                *all = 0;
                *debug = 0;
            } else if (strcmp(argv[2], "all") == 0) {
                *simple = 0;
                *all = 1;
                *debug = 0;
            } else if (strcmp(argv[2], "debug") == 0) {
                *simple = 0;
                *all = 0;
                *debug = 1;
            }
        } else {
            break;
        }
        return 0;
    case 4:
       if (strcmp(argv[1], "complex") == 0) {
            if (strcmp(argv[2], "some") == 0 && strcmp(argv[3], "debug") == 0) {
                *simple = 0;
                *all = 0;
                *debug = 1;
            } else if (strcmp(argv[2], "all") == 0 && strcmp(argv[3], "debug") == 0) {
                *simple = 0;
                *all = 1;
                *debug = 1;
            } else {
                break;
            }
        } else {
            break;
        }
        return 0;
    default:
        break;
    }

    return -1;
}

int platform_devfile_read(const char *filename, char *buf, int size, int offset)
{
    int fd;
    int nread;
    int ret;

    if (filename == NULL || buf == NULL) {
       FAC_LOG_DBG(GRTD_LOG_ERR, "param is error!\n");
       return FAC_TEST_FAIL;
    }
    ret = FAC_TEST_OK;
    if ((fd = open(filename, O_RDONLY | O_SYNC)) < 0) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "ERROR: Open file[%s] failed(%s)!\n", filename, strerror(errno));
        return FAC_TEST_FAIL;
    }

    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "ERROR: lseek file[%s] error(%s)!\n", filename, strerror(errno));
        ret = FAC_TEST_FAIL;
        goto error;
    }

    nread = read(fd, buf, size);
    if (nread < 0) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "ERROR: Read file[%s] error(%s)!\n", filename, strerror(errno));
        ret = FAC_TEST_FAIL;
        goto error;
    }

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "%s read data(off:0x%x):0x%x\n", filename, offset, buf[0]);
error:
    close(fd);
    return ret;
}

int platform_devfile_write(const char *filename, char *buf, int size, int offset)
{
    int fd;
    int nwrite;
    int ret;

    if (filename == NULL || buf == NULL) {
       FAC_LOG_DBG(GRTD_LOG_ERR, "param is error!\n");
       return FAC_TEST_FAIL;
    }
    ret = FAC_TEST_OK;
    if ((fd = open(filename, O_CREAT | O_RDWR | O_TRUNC | O_SYNC, 0)) < 0) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "ERROR: Open file[%s] failed(%s)!\n", filename, strerror(errno));
        return FAC_TEST_FAIL;
    }

    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "ERROR: lseek file[%s] error(%s)!\n", filename, strerror(errno));
        ret = FAC_TEST_FAIL;
        goto error;
    }

    nwrite = write(fd, buf, size);
    if (nwrite != size) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "ERROR: Write file[%s] error(%s)!\n", filename, strerror(errno));
        ret = FAC_TEST_FAIL;
        goto error;
    }

error:
    close(fd);
    sync();
    return ret;
}

int platform_get_sys_memory_size_form_cmdline(unsigned int *sys_memory_size)
{
    int ret;
    char *start;
    char *mem_start_string;
    char mem_size_buf[FAC_MEM_SIZE_BUF_LEN];
    
    /* MEMORY information area start string */
    mem_start_string = "RAMSZ=";
    /* Get memory size from the /proc/octeon_info file */
    memset(mem_size_buf, 0, FAC_MEM_SIZE_BUF_LEN);
    ret = platform_devfile_read("/proc/cmdline", mem_size_buf, FAC_MEM_SIZE_BUF_LEN - 1, 0);
    if (ret) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Read mem info fail!\n");
        return FAC_TEST_FAIL;
    }
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:mem info[%s]\n", mem_size_buf);

    /* Extract the memory size information from the read information */
    start = strstr(mem_size_buf, mem_start_string);
    if (start != NULL) {
        start = start + strlen(mem_start_string);
        *sys_memory_size = strtoul(start, NULL, 10); 
        FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:Mem size[%d MB]\n", *sys_memory_size);
        return FAC_TEST_OK;
    } else {
        FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Get mem size fail!\n");
        return FAC_TEST_FAIL;
    }
}

/*
 * platform_simple_get_mem_space - Gets the space for the memory test
 * @buf: pointer to the start address
 * @size: indicates the space size
 *
 * return: FAC_TEST_FAIL | FAC_TEST_OK
 */
static int platform_simple_get_mem_space(void **buf, size_t *size)
{
    size_t pagesize, wantsize, alignedsize;
    void *start, *aligned;
    int done_mem;
    ptrdiff_t pagesizemask;

    start = NULL;
    done_mem = 0;
    wantsize = FAC_MEM_TEST_SIZE;
    pagesize = 4096;
    pagesizemask = (ptrdiff_t) ~(pagesize - 1);

    while (!done_mem) {
        while (!start && wantsize) {
            start = (void *) malloc(wantsize);
            if (!start) wantsize -= pagesize;
        }

        if (!wantsize) {
            FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Malloc simple space failed\n");
            return FAC_TEST_FAIL;
        }

        memset(start, 0, wantsize);
        *size = wantsize;
        alignedsize = wantsize;
        
        fflush(stdout);
        if (do_mlock && wantsize) {
            fflush(stdout);
            
            if ((size_t) start % pagesize) {
                aligned = (void *) ((size_t) start & pagesizemask) + pagesize;
                alignedsize -= ((size_t) aligned - (size_t) start);
            } else {
                aligned = start;
            }
            
            FAC_LOG_DBG(GRTD_LOG_DEBUG, "trying mlock ...start[0x%lx], size[0x%x]", (ulong)start,
                (int)alignedsize);
            /* Try mlock */
            if (mlock(aligned, alignedsize) < 0) {
                switch(errno) {
                case EAGAIN: 
                    FAC_LOG_DBG(GRTD_LOG_ERR, "over system/pre-process limit, reducing...\n");
                    free(start);
                    start = NULL;
                    wantsize -= pagesize;
                    break;
                case ENOMEM: 
                    FAC_LOG_DBG(GRTD_LOG_ERR, "too many pages, reducing...\n"); 
                    free(start); 
                    start = NULL;
                    wantsize -= pagesize;
                    break;
                case EPERM:
                    FAC_LOG_DBG(GRTD_LOG_ERR, "insufficient permission.\n");
                    FAC_LOG_DBG(GRTD_LOG_ERR, "Trying again, unlocked:\n");
                    do_mlock = 0;
                    free(start);
                    start = NULL;
                    wantsize = FAC_MEM_TEST_SIZE; 
                    break;
                default:
                    FAC_LOG_DBG(GRTD_LOG_ERR, "failed for unknown reason.\n");
                    do_mlock = 0;
                    done_mem = 1;
                }
            } else {
                FAC_LOG_DBG(GRTD_LOG_DEBUG, "locked.\n");
                done_mem = 1;
            }
        } else {
            done_mem = 1;
        }
    }
    
    if (!wantsize) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Malloc space failed\n");
        return FAC_TEST_FAIL;
    }
    if (!do_mlock) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Mlock mmap'ed space failed\n");
        FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM WARNING:Continuing with unlocked memory;"
            " testing will be slower and less reliable.\n");
    }
    *buf = start;
    return FAC_TEST_OK;
}

static int platform_simple_check_addrline(ulong *start, size_t size, char *desc)
{    
    ulong *p1;
    ulong j;
    size_t i;
    size_t physaddr;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:addr line check START[0x%lx] SIZE[0x%lx]!\n",
        (ulong)start, (ulong)size);
    fflush(stdout);
    for (j = 0; j < 2; j++) {
        p1 = start;
        
        fflush(stdout);
        for (i = 0; i < size; i += sizeof(ulong), p1++) {
            *((volatile ulong *)p1) = ((((j + i) % 2) == 0) ? (ulong)p1 : ~((ulong)p1));
        }
        
        fflush(stdout);
        
        p1 = start;
        for (i = 0; i < size; i += sizeof(ulong), p1++) {
            if (*((volatile ulong *)p1) != (((j + i) % 2) == 0 ? (ulong)p1 : ~((ulong)p1))) {
                physaddr = (size_t)start + (i * sizeof(ulong));
                FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Possible bad address line at physical "
                        "address 0x%08lx.\n", physaddr);
                sprintf(desc, "MEM ERROR:Possible bad address line at physical "
                        "address 0x%08lx.\n", physaddr);
                fflush(stdout);
                return FAC_TEST_FAIL;
            }
        }
    }
    fflush(stdout);
    
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:addr line check OK!\n");
    return FAC_TEST_OK;
}
static int platform_simple_check_data(ulong *start, size_t size, char *desc)
{
    int i;
    size_t j;
    ulong data = 0;
    ulong *p1;
    ulong check_error = 0;

    /* Walking 1's Test */
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:Walking 1's Test START[0x%lx] SIZE[0x%lx]!\n",
        (ulong)start, (ulong)size);
    for (i = 0; i < 8; i++) {
        data = (data << 1) + 1;
        p1 = start;
        fflush(stdout);
        for (j = 0; j < size; j += sizeof(ulong), p1++) {
            *((volatile ulong *)p1) = data;
        }        
        fflush(stdout);
        p1 = start;
        for (j = 0; j < size; j += sizeof(ulong), p1++) {
            if (*((volatile ulong *)p1) != data) {
                FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:[0x%lx] should be 0x%lx,but is 0x%lx.\n",
                    (ulong)p1, data, *((volatile ulong *)p1));
                check_error++;
                sprintf(desc, "MEM ERROR:[0x%lx] should be 0x%lx,but is 0x%lx.\n",
                    (ulong)p1, data, *((volatile ulong *)p1));
                return FAC_TEST_FAIL;
            }
        }
    }
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:Walking 1's Test END!\n");
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:Bus Noise Test START[0x%lx] SIZE[0x%lx]!\n",
        (ulong)start, (ulong)size);

    /* Bus Noise Test */
    fflush(stdout);
    p1 = start;
    for (j = 0; j < size; j += sizeof(ulong), p1++) {
        *((volatile ulong *)p1) = (j % 2) ? mem_test_data1 : mem_test_data2;
    }
    fflush(stdout);
    p1 = start;
    for (j = 0; j < size; j += sizeof(ulong), p1++) {
        if (*((volatile ulong *)p1) != ((j % 2) ? mem_test_data1 : mem_test_data2)) {
            FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:[0x%lx] should be 0x%lx,but is 0x%lx.\n",
                (ulong)p1, ((j % 2) ? mem_test_data1 : mem_test_data2), *((volatile ulong *)p1));
            check_error++;
            sprintf(desc, "MEM ERROR:[0x%lx] should be 0x%lx,but is 0x%lx.\n",
                (ulong)p1, ((j % 2) ? mem_test_data1 : mem_test_data2), *((volatile ulong *)p1));
            return FAC_TEST_FAIL;
        }
    }
    fflush(stdout);    
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:Bus Noise Test END!\n");
    return check_error;
}

static void platform_simple_free_mem_space(void *buf, size_t size)
{
    if (do_mlock) {
        munlock(buf, size);
    }
    free(buf);
}

static int platform_simple_sdram_ecc_detect()
{
    char c;
    char *loc, *next;
    FILE *fp;
    char file_line[FAC_FILE_LINE_LEN];
    int i, core_num, temp, ecc_error;

    i = 0;
    core_num = 0;
    ecc_error = 0;
    
    /* Open the MTD device file */
    if ((fp = fopen("/proc/interrupts", "r")) == NULL) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "Fopen  /proc/interrupts fail!\t err_no[%d]\n", errno);
        return FAC_TEST_FAIL;
    }
    
    memset(file_line, 0, FAC_FILE_LINE_LEN);
    /* Find the MTD device name available for production testing on each line in the MTD file */
    while (fgets(file_line, FAC_FILE_LINE_LEN, fp) != NULL) {
        if (i == 0) {
            loc = strrchr(file_line, 'U');
            core_num = strtoul(loc + 1, NULL, 10);
        }
        i++;
        if (strstr(file_line, "LMC") != NULL) {
           goto find;
        }
    }
    FAC_LOG_DBG(GRTD_LOG_ERR, "ECC: Not find lmc info in file\n");
    fclose(fp);
    return ecc_error;
find:
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ECC: Get ecc info[%s]\n", file_line);
    /* Gets the number of the MTD device name that can be production-tested, with the device name and device partition size delimiters as colons */
    c = ':';
    if ((loc = strchr(file_line, c))!= NULL) {
        i = 0;
        loc++;
        while (i <= core_num) {
            temp = strtoul(loc, &next, 10);
            loc = next;
            ecc_error += temp;
            FAC_LOG_DBG(GRTD_LOG_DEBUG, "ECC: cpu %d ecc error num: %d\n", i, temp);
            i++;
        }
        
    }
    
    fclose(fp);
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ECC: ecc error num: %d\n", ecc_error);
    return ecc_error;

}

int platform_simple_sys_memory_test(char *desc)
{
    void *buf, *aligned;
    size_t size, alignedsize, pagesize;
    ptrdiff_t pagesizemask;
    int ret = FAC_TEST_OK;

    /* Gets the memory space available for testing */
    if (platform_simple_get_mem_space(&buf, &size) != FAC_TEST_OK) {
        ret = GRTD_SDRAM_WR_ERR;
        goto ecctest;
    }
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:Get test space success, start at [0x%lx] size[0x%lx]\n",
        (ulong)buf, (ulong)size);

    /* Align 4K */
    pagesize = 4096;
    pagesizemask = (ptrdiff_t) ~(pagesize - 1);
    alignedsize = size;
    if ((size_t) buf % pagesize) {
        aligned = (void *) ((size_t) buf & pagesizemask) + pagesize;
        alignedsize -= ((size_t) aligned - (size_t) buf);
    } else {
        aligned = buf;
    }
    
    /* Check address line */
    if (platform_simple_check_addrline((ulong *)aligned, alignedsize, desc) != FAC_TEST_OK) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Check address line failed!\n");
        ret = GRTD_SDRAM_WR_ERR;
        goto _error;
    }

    /* Memory read/write check */
    if (platform_simple_check_data((ulong *)aligned, alignedsize, desc)) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Check data failed!\n");
        ret = GRTD_SDRAM_WR_ERR;
        goto _error;
    }

_error:
    platform_simple_free_mem_space(buf, size);
ecctest:
    /* Memory ECC error detection */
    if (platform_simple_sdram_ecc_detect()) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "ERROR: ECC error occured!\n");
        ret = GRTD_SDRAM_ECC_ERR;
    }
    return ret;
}

static int platform_get_system_free_mem(size_t *free_size)
{
    int len;
    FILE *fp;
    char *loc;
    char mem_file_line[FAC_FILE_LINE_LEN];
    char mem_size[FAC_FILE_LINE_LEN];
    size_t all_free_size;

    if ((fp = fopen("/proc/meminfo", "r")) == NULL) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "Open meminfo fail(%s)!\n", strerror(errno));
        return FAC_TEST_FAIL;
    }

    /* Gets the path to the mount */
    memset(mem_file_line, 0, sizeof(mem_file_line));
    while (fgets(mem_file_line, FAC_FILE_LINE_LEN, fp) != NULL) {
        FAC_LOG_DBG(GRTD_LOG_DEBUG, "meminfo file content:%s \n", mem_file_line);
        loc = strstr(mem_file_line, "MemFree:");
        if (loc == NULL) {
            continue;
        }

        loc = loc + strlen("MemFree:");
        while (*loc == ' ') {
            loc++;
        }
        len = 0;
        while (*(loc + len) != ' ') {
            len++;
        }
        FAC_LOG_DBG(GRTD_LOG_DEBUG, "free mem info:%s, len:%d \n", loc, len);
        strncpy(mem_size, loc, len);
        mem_size[len] = 0;
        all_free_size = strtoul(mem_size, NULL, 10);
        /* Use 90% to test */
        *free_size = all_free_size / 4 / 10 * 9 * 4096;
        goto getok;
    }

    FAC_LOG_DBG(GRTD_LOG_ERR, "MEM free info get failed!\n");
    fclose(fp);
    return FAC_TEST_FAIL;

getok:
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "free mem: total--0x%lx use--0x%lx\n", all_free_size * 1024, *free_size);
    fclose(fp);
    return FAC_TEST_OK;
}

/*
 * platform_get_mem_space - Gets the space for the memory test
 * @buf: pointer to the obtained start address
 * @size: indicates the space size
 *
 * Return int: FAC_TEST_FAIL | FAC_TEST_OK
 */
static int platform_get_mem_space(void **buf, size_t *size)
{
    void *start;
    size_t pagesize, wantsize;

    start = NULL;
    if (platform_get_system_free_mem(&wantsize)) {
        FAC_LOG_DBG(GRTD_LOG_DEBUG, "Get free mem info fail,use defaul size(0x%x)\n", FAC_MEM_TEST_SIZE);
        wantsize = FAC_MEM_TEST_SIZE;
    }
    pagesize = 4096;      /* The page size is 4K*/

retry:
    while (!start && wantsize) {
        start = (void *) malloc(wantsize);
        if (!start) wantsize -= pagesize;
    }
    if (!wantsize) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "MEM ERROR:Malloc space failed\n");
        return FAC_TEST_FAIL;
    }
    memset(start, 0, wantsize);
    *size = wantsize;
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "trying mlock ...start[%p], size[0x%lx]\n", start, wantsize);
    /* mlock Memory space */
    if (mlock(start, wantsize) < 0) {
        FAC_LOG_DBG(GRTD_LOG_ERR, "failed for mlock(%s), retry.\n", strerror(errno));
        free(start);
        start = NULL;
        wantsize -= pagesize;
        goto retry;
    }

    *buf = start;
    return FAC_TEST_OK;
}

/*
 * platform_free_mem_space - Release the memory required for the memory test
 * 
 * Return void:
 */
static void platform_free_mem_space(void *buf, size_t size)
{
    munlock(buf, size);
    free(buf);
}

/**
 * platform_complex_sys_memory_test - Memory test
 * @sdram_test: Output details of memory test results
 * @kaoji: Enter, is the machine currently copied *
 * Return int: FAC_TEST_FAIL | FAC_TEST_OK
 */
int platform_complex_sys_memory_test(char *desc, int kaoji)
{
    int ret;
    void *buf;
    size_t size, autotest_size;
    int i;
    char desc_tmp[128];

    if (desc == NULL) {
       FAC_LOG_DBG(GRTD_LOG_ERR, "param is error!\n");
       return FAC_TEST_FAIL;
    }

    ret = FAC_TEST_OK;
    /* Gets the memory space available for testing */
    if (platform_get_mem_space(&buf, &size) != FAC_TEST_OK) {
        return FAC_TEST_FAIL;
    }

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "MEM:Get test space success, start at [0x%lx] size[0x%lx]\n",
        (ulong)buf, (ulong)size);

    autotest_size = size;
#if 0
    /* Init ddr test. */
    if (kaoji && autotest_size > FAC_MEM_AUTOTEST_SIZE) {
        autotest_size = FAC_MEM_AUTOTEST_SIZE;
    } else if (autotest_size > FAC_MEM_MAX_SIZE) {
        autotest_size = FAC_MEM_MAX_SIZE;
    }
#endif
    ft_ddr_test_init(buf, autotest_size);
    /* init memory */
    memset((char *)buf, 0, autotest_size);

    /* Test memory */
    memset((char *)desc_tmp, 0, 128);
    for (i = 0 ; i < TETS_TOTAL_NUMBER; i++) {
        if (ft_ddr_test_fun[i].always == 0 && kaoji) {
            /* In the copy state, some test items are not tested */
            continue;
        }

        if (ft_ddr_test_fun[i].ddr_test != NULL) {
            if (ft_ddr_test_fun[i].ddr_test(desc_tmp) != FT_DDR_SUCCESS) {
                sprintf(desc, "%s:%s", ft_ddr_test_fun[i].name, desc_tmp);
                ret = GRTD_SDRAM_WR_ERR;
                break;
            }
        }
    }
    FT_DDR_SHOW_END();

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "TEST ok(ret:%d).\n", ret);
    /* Free memory */
    platform_free_mem_space(buf, size);
    return ret;
}

static int memory_test(int simple, int all)
{
    unsigned int memory_size;
    char desc[128];
    int ret;

    if (platform_get_sys_memory_size_form_cmdline(&memory_size)) {
        printf("Get memory size fail\n");
        return FAC_TEST_FAIL;
    }

    if (simple) {
        ret = platform_simple_sys_memory_test(desc);
    } else {
        if (all) {
            ret = platform_complex_sys_memory_test(desc, 0);
        } else {
            ret = platform_complex_sys_memory_test(desc, 1);
        }
    }

    if (simple) {
        printf("\n[Simple Algorithm]\n");
    } else {
        if (all) {
            printf("\n[Complex All Algorithm]\n");
        } else {
            printf("\n[Complex Some Algorithm]\n");
        }
    }
    printf("    SDRAM Space: %d(MB)\n", memory_size);
    if (ret != FAC_TEST_OK) {
        switch (ret) {
        case GRTD_SDRAM_ECC_ERR:
            printf("    ECC check error\r\n");
            break;
        case GRTD_SDRAM_WR_ERR:
            printf("    Error type: Memory read/write error\r\n");
            break;
        case GRTD_SDRAM_GET_MEM_SIZE_ERR:
            printf("    Error type: Failed to obtain memory capacity\r\n");
            break;
        default:
            printf("    Unknown cause of failure(%d)\r\n", ret);
            break;
        }
        printf("    %s", desc);
        printf("    Test Result: Fail\n\n");
    } else {
        printf("    Test Result: Pass\n\n");
    }
    return ret;
}

int dram_test_main(int argc, char **argv)
{
    int simple,all,debug;
    int ret;

    printf("\r\n"
           "********************** dram test **********************\r\n");

    debug = 0;
    simple = 0;
    all = 0;
    ret = dram_test_arg_pars(argc, argv, &simple, &all, &debug);
    if (ret < 0) {
        fprintf(stderr,
            "Usage: dram_test [simple|complex some|complex all]  \r\n"
            "  simple       Use the simple Algorithm             \r\n"
            "  complex some Use some the complex Algorithm       \r\n"
            "  complex all  Use all the complex Algorithm        \r\n");
        exit(1);
        return -1;
    }
    if (debug) {
        platform_fac_dbg = GRTD_LOG_DEBUG;
    }

    return memory_test(simple, all);
}
