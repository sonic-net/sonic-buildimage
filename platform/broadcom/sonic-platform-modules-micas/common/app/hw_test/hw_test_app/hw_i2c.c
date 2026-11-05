#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <hw_i2c/hw_i2c.h>
#include <hw_i2c/i2c-dev.h>
#include <hw_i2c/i2c.h>
#include <hw_i2c/rtc.h>

static void rtc_help(char *name)
{
    fprintf(stderr,
            "Usage: %s  rtc year month day hour minute second                       \r\n"
            "  rtc          rtc number(default 0)                                   \r\n"
            "  year                                                                 \r\n"
            "  month                                                                \r\n"
            "  day                                                                  \r\n"
            "  hour                                                                 \r\n"
            "  minute                                                               \r\n"
            "  second                                                               \r\n",
            name);
    exit(1);
}

static void i2c_help(char *name)
{
    fprintf(stderr,
            "Usage: %s  i2c_bus slave_addr offset data [data_len] [times] [offset_len]  \r\n"
            "  i2c_bus      controller integer or an I2C bus name                    \r\n"
            "  slave_addr   i2c device address                                       \r\n"
            "  offset       The relative offset within the device                    \r\n"
            "  data         Test data(8bit)                                          \r\n"
            "  [data_len]   The length of test data(default 1)                       \r\n"
            "  [times}                                                               \r\n",
            name);
    exit(1);
}

static int lookup_i2c_bus(const char *i2cbus_arg)
{
    unsigned long i2cbus;
    char *end;

    i2cbus = strtoul(i2cbus_arg, &end, 0);
    if (*end || !*i2cbus_arg) {
        /* Not a number, maybe a name? */
        //return lookup_i2c_bus_by_name(i2cbus_arg);
        return -EINVAL;
    }
    if (i2cbus > 0xFFFFF) {
        fprintf(stderr, "Error: I2C bus out of range!\n");
        return -EINVAL;
    }

    return i2cbus;
}

static int parse_i2c_address(const char *address_arg)
{
    long address;
    char *end;

    address = strtol(address_arg, &end, 0);
    if (*end || !*address_arg) {
        fprintf(stderr, "Error: slave address is not a number!\n");
        return -EINVAL;
    }
    if (address < 0x03 || address > 0x255) {
        fprintf(stderr, "Error: save address out of range "
                "(0x03-0x77)!\n");
        return -EINVAL;
    }

    return address;
}

static int i2c_arg_check(int argc, char **argv, struct i2c_dev_priv *i2c_priv, int min_arg)
{
    int i2cbus, address, offset = 0, data = 0, data_len = 1, times = 1;
    int offset_len = 1;
    char *end;
    int flags = 0;

    if (argc < min_arg) {
        return -EINVAL;
    }

    i2cbus = lookup_i2c_bus(argv[flags + 1]);
    if (i2cbus < 0) {
        return -EINVAL;
    }

    address = parse_i2c_address(argv[flags + 2]);
    if (address < 0) {
        return -EINVAL;
    }

    offset = strtol(argv[flags + 3], &end, 0);
    if (*end || offset < 0 || offset > 0xffff) {
        fprintf(stderr, "Error: Data offset invalid!\n");
        return -EINVAL;
    }

    if (argc > 4) {
        data = strtol(argv[flags + 4], &end, 0);
        if (*end || data < 0) {
            fprintf(stderr, "Error: Data invalid!\n");
            return -EINVAL;
        }
    }

    if (argc > 5) {
        data_len = strtol(argv[flags + 5], &end, 0);
        if (*end || data_len < 0) {
            fprintf(stderr, "Error: Data length invalid!\n");
            return -EINVAL;
        }
    }

    if (argc > 6) {
        times = strtol(argv[flags + 6], &end, 0);
        if (*end || times < 0) {
            fprintf(stderr, "Error: times invalid!\n");
            return -EINVAL;
        }
    }

    if (argc > 7) {
        offset_len = strtol(argv[flags + 7], &end, 0);
        if (*end || offset_len < 0) {
            fprintf(stderr, "Error: offset_len invalid!\n");
            return -EINVAL;
        }
    }

    i2c_priv->i2cbus        = i2cbus;
    i2c_priv->save_addr     = address;
    i2c_priv->offset        = offset;
    i2c_priv->offset_len    = offset_len;
    i2c_priv->data          = data;
    i2c_priv->data_len      = data_len;
    i2c_priv->times         = times;
    return 0;
}

static int open_i2c_dev(int i2cbus)
{
    int file, ret;
    char filename[I2C_MAX_NAME_SIZE];

    ret = snprintf(filename, I2C_MAX_NAME_SIZE, "/dev/i2c-%d", i2cbus);
    filename[ret] = '\0';
    if ((file = open(filename, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "`%s': %s\n", filename, strerror(errno));
    }

    return file;
}

#if 0
static int set_slave_addr(int file, int address, int force)
{
    /* With force, let the user read from/write to the registers
       even when a driver is also running */
    if (ioctl(file, force ? I2C_SLAVE_FORCE : I2C_SLAVE, address) < 0) {
        fprintf(stderr,
            "Error: Could not set address to 0x%02x: %s\n",
            address, strerror(errno));
        return -errno;
    }

    return 0;
}
#endif

/**
 *
 * i2c_transfer
 *
 *
 * @param file
 * @param i2c_priv
 * @param read_write   0:write  1:read
 * @param num_msgs
 *
 * @return int
 */
static int i2c_transfer(int file, struct i2c_dev_priv *i2c_priv, int read_write, int num_msgs)
{
    int i;
    unsigned char offset[2];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data ioctl_data;
    int len;

    if (i2c_priv->buffer == NULL) {
        i2c_priv->buffer = malloc(i2c_priv->data_len);
        if (i2c_priv->buffer == NULL) {
            fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
            return -ENOMEM;
        }
    }

    for (i = 0; i < i2c_priv->data_len && !read_write; i++) {
        i2c_priv->buffer[i] = i2c_priv->data;
    }

    if (i2c_priv->offset_len == 1) {
    offset[0] = (i2c_priv->offset & 0xFF);
        len = 1;
    } else {
        offset[0] = ((i2c_priv->offset & 0xFF00) >> 8);
        offset[1] = (i2c_priv->offset & 0xFF);
        len = 2;
    }

    switch (num_msgs) {
    case 1:
        msgs[0].addr     = i2c_priv->save_addr;
        msgs[0].flags    = read_write;
        msgs[0].buf      = i2c_priv->buffer;
        msgs[0].len      = i2c_priv->data_len;

        ioctl_data.msgs  = msgs;
        ioctl_data.nmsgs = 1;
        break;
    case 2:
        msgs[0].addr     = i2c_priv->save_addr;
        msgs[0].flags    = 0;
        msgs[0].buf      = offset;
        msgs[0].len      = len;

        msgs[1].addr     = i2c_priv->save_addr;
        msgs[1].flags    = read_write;
        msgs[1].buf      = i2c_priv->buffer;
        msgs[1].len      = i2c_priv->data_len;

        ioctl_data.msgs  = msgs;
        ioctl_data.nmsgs = 2;
        break;
    default:
        break;
    }

    if (ioctl(file, I2C_RDWR, &ioctl_data) < 0) {
        fprintf(stderr,
            "Error:  i2c_transfer error %s\n",
            strerror(errno));
        return -1;
    }

    return 0;
}

static void print_arg(struct i2c_dev_priv *i2c_priv)
{
    printf("I2C Controller %d, Slave addr: 0x%02X, Offset: 0x%02X",
           i2c_priv->i2cbus, i2c_priv->save_addr, i2c_priv->offset);

    if (i2c_priv->data) {
        printf(", Data = 0x%02X", i2c_priv->data);
    }

    if (i2c_priv->data_len) {
        printf(", Data_len = %d", i2c_priv->data_len);
    }

    if (i2c_priv->times) {
        printf(", Times = %u", i2c_priv->times);
    }

    if (i2c_priv->offset_len) {
        printf(", Offset_len = %u", i2c_priv->offset_len);
    }

    printf("\r\n");
}

static int i2c_check_data(unsigned char *buffer, struct i2c_dev_priv *i2c_priv)
{
    int i;
    for (i = 0; i < i2c_priv->data_len; i++) {
        if (buffer[i] != i2c_priv->buffer[i]) {
            return (-1);
        }
    }

    return (0);
}

int i2c_wr_main(int argc, char **argv)
{
    int ret, fd;
    struct i2c_dev_priv i2c_priv;

    memset(&i2c_priv, 0, sizeof(struct i2c_dev_priv));

    ret = i2c_arg_check(argc, argv, &i2c_priv, 5);
    if (ret < 0) {
        i2c_help("i2c_wr");
        return -1;
    }

    printf("\r\n"
           "********************** I2C write test **********************\r\n");
    print_arg(&i2c_priv);

    if ((fd = open_i2c_dev(i2c_priv.i2cbus)) < 0) {
        return -1;
    }

#if 0
    if ((ret = set_slave_addr(fd, i2c_priv.save_addr, 0)) < 0) {
        close(fd);
        return -1;
    }
#endif

    while (i2c_priv.times--) {
        if ((ret = i2c_transfer(fd, &i2c_priv, 0, 2)) < 0) {
            goto  error_out;
        }
    }

error_out:
    printf("\r\n******************** I2C read test End ********************\r\n\r\n");
    if (i2c_priv.buffer) {
        free(i2c_priv.buffer);
    }
    close(fd);
    return ret;
}

int i2c_rd_main(int argc, char **argv)
{
    int i, ret, fd;
    struct i2c_dev_priv i2c_priv;

    memset(&i2c_priv, 0, sizeof(struct i2c_dev_priv));
    ret = i2c_arg_check(argc, argv, &i2c_priv, 4);
    if (ret < 0) {
        i2c_help("i2c_rd");
        return -1;
    }

    if ((fd = open_i2c_dev(i2c_priv.i2cbus)) < 0) {
        return -1;
    }

    printf("\r\n"
           "********************** I2C read test **********************\r\n");
    print_arg(&i2c_priv);

#if 0
    if ((ret = set_slave_addr(fd, i2c_priv.save_addr, 0)) < 0) {
        close(fd);
        return -1;
    }
#endif

    while (i2c_priv.times--) {
        if ((ret = i2c_transfer(fd, &i2c_priv, I2C_M_RD, 2)) < 0) {
            goto  error_out;
        }
    }
    printf("Last Read: \r\n");
    /* dump data */
    for (i = 0; i < i2c_priv.data_len; i++) {
        printf("%02X ", i2c_priv.buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\r\n");
        }
    }

error_out:
    printf("\r\n******************** I2C read test End ********************\r\n\r\n");
    if (i2c_priv.buffer) {
        free(i2c_priv.buffer);
    }
    close(fd);
    return ret;
}

int i2c_chk_main(int argc, char **argv)
{
    int ret, fd;
    struct i2c_dev_priv i2c_priv;
    unsigned char *buffer;

    memset(&i2c_priv, 0, sizeof(struct i2c_dev_priv));

    ret = i2c_arg_check(argc, argv, &i2c_priv, 5);
    if (ret < 0) {
        i2c_help("i2c_chk");
        return -1;
    }

    printf("\r\n"
           "********************** I2C write test **********************\r\n");
    print_arg(&i2c_priv);

    if ((fd = open_i2c_dev(i2c_priv.i2cbus)) < 0) {
        return -1;
    }

#if 0
    if ((ret = set_slave_addr(fd, i2c_priv.save_addr, 0)) < 0) {
        close(fd);
        return -1;
    }
#endif

    buffer = malloc(i2c_priv.data_len);
    if (buffer == NULL) {
        fprintf(stderr,
                "Error: Could not malloc memory %s\n",
                strerror(errno));
        ret = -ENOMEM;
        goto error_out;
    }
    while (i2c_priv.times--) {
        if ((ret = i2c_transfer(fd, &i2c_priv, 0, 2)) < 0) {
            goto  error_out;
        }
        memcpy(buffer, i2c_priv.buffer, i2c_priv.data_len);
        if ((ret = i2c_transfer(fd, &i2c_priv, I2C_M_RD, 2)) < 0) {
            goto  error_out;
        }
        if ((ret = i2c_check_data(buffer, &i2c_priv)) < 0) {
            i2c_priv.check_test_errors++;
        }
    }
    printf("Check test errors = %u\r\n", i2c_priv.check_test_errors);

error_out:
    printf("\r\n******************** I2C read test End ********************\r\n\r\n");
    if (i2c_priv.buffer) {
        free(i2c_priv.buffer);
    }
    if (buffer) {
        free(buffer);
    }
    close(fd);
    return ret;
}

int i2c_reset_main(int argc, char **argv)
{
    printf("not support %s argc:%d, \r\n", argv[0], argc);
    return 0;
}

int pca9548_rd_main(int argc, char **argv)
{
    int i, ret, fd;
    struct i2c_dev_priv i2c_priv;

    memset(&i2c_priv, 0, sizeof(struct i2c_dev_priv));
    ret = i2c_arg_check(argc, argv, &i2c_priv, 4);
    if (ret < 0) {
        i2c_help("pca9548_rd");
        return -1;
    }

    printf("\r\n"
           "********************** I2C multiplexer read test **********************\r\n");
    print_arg(&i2c_priv);

    if ((fd = open_i2c_dev(i2c_priv.i2cbus)) < 0) {
        return -1;
    }

#if 0
    if ((ret = set_slave_addr(fd, i2c_priv.save_addr, 0)) < 0) {
        close(fd);
        return -1;
    }
#endif

    while (i2c_priv.times--) {
        if ((ret = i2c_transfer(fd, &i2c_priv, I2C_M_RD, 1)) < 0) {
            goto  error_out;
        }
    }
    printf("Last Read: \r\n");
    /* dump data */
    for (i = 0; i < i2c_priv.data_len; i++) {
        printf("%02X ", i2c_priv.buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\r\n");
        }
    }

error_out:
    printf("\r\n******************** I2C multiplexer read test End ********************\r\n\r\n");
    if (i2c_priv.buffer) {
        free(i2c_priv.buffer);
    }
    close(fd);
    return ret;
}

int pca9548_wr_main(int argc, char **argv)
{
    int ret, fd;
    struct i2c_dev_priv i2c_priv;

    memset(&i2c_priv, 0, sizeof(struct i2c_dev_priv));

    ret = i2c_arg_check(argc, argv, &i2c_priv, 5);
    if (ret < 0) {
        i2c_help("pca9548_wr");
        return -1;
    }

    printf("\r\n"
           "********************** I2C multiplexer write test **********************\r\n");
    print_arg(&i2c_priv);

    if ((fd = open_i2c_dev(i2c_priv.i2cbus)) < 0) {
        return -1;
    }

#if 0
    if ((ret = set_slave_addr(fd, i2c_priv.save_addr, 0)) < 0) {
        close(fd);
        return -1;
    }
#endif

    while (i2c_priv.times--) {
        if ((ret = i2c_transfer(fd, &i2c_priv, 0, 1)) < 0) {
            goto  error_out;
        }
    }

error_out:
    printf("\r\n******************** I2C multiplexer read test End ********************\r\n\r\n");
    if (i2c_priv.buffer) {
        free(i2c_priv.buffer);
    }
    close(fd);
    return ret;
}

int rtc_rd_main(int argc, char **argv)
{
    int ret, fd;
    struct rtc_time rtc;
    int rtc_num;
    char *end, filename[I2C_MAX_NAME_SIZE];

    if (argc < 2) {
        rtc_help("rtc_rd");
    }

    rtc_num =  strtol(argv[1], &end, 0);
    if (*end || rtc_num < 0 || rtc_num > 0xff) {
        fprintf(stderr, "Error: Invalid argument!\n");
        rtc_help("rtc_rd");
        return -EINVAL;
    }

    ret = snprintf(filename, I2C_MAX_NAME_SIZE, "/dev/rtc%d", rtc_num);
    filename[ret] = '\0';
    if ((fd = open(filename, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "`%s': %s\n", filename, strerror(errno));
        return -1;
    }

    if (ioctl(fd, RTC_RD_TIME, &rtc) < 0) {
        fprintf(stderr,
            "Error:  get rtc error %s\n",
            strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    printf("\r\n******************** RTC read test ********************\r\n");

    printf("Read RTC: %d-%d-%d, %d:%d:%d\n",
           rtc.tm_year + 1900, rtc.tm_mon + 1, rtc.tm_mday,
           rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
    printf("\r\n******************** RTC read test End ********************\r\n\r\n");

    return 0;
}

int rtc_wr_main(int argc, char **argv)
{
    int ret, fd, year;
    struct rtc_time rtc;
    int rtc_num;
    char *end, filename[I2C_MAX_NAME_SIZE];

    if (argc < 8) {
        rtc_help("rtc_wr");
    }

    rtc_num =  strtol(argv[1], &end, 0);
    if (*end || rtc_num < 0 || rtc_num > 0xff) {
        fprintf(stderr, "Error: Invalid argument!\n");
        rtc_help("rtc_wr");
        return -EINVAL;
    }

    year =  strtol(argv[2], &end, 0);
    if (*end || year < 0) {
        fprintf(stderr, "Error: Invalid argument!\n");
        rtc_help("rtc_wr");
        return -EINVAL;
    }

    rtc.tm_year = year - 1900;

    rtc.tm_mon =  strtol(argv[3], &end, 0);
    if (*end || rtc.tm_mon <= 0 || rtc.tm_mon > 12) {
        fprintf(stderr, "Error: Invalid argument!\n");
        rtc_help("rtc_wr");
        return -EINVAL;
    }

    rtc.tm_mon = rtc.tm_mon - 1;

    rtc.tm_mday =  strtol(argv[4], &end, 0);
    if (*end || rtc.tm_mday < 0 || rtc.tm_mday > 31) {
        fprintf(stderr, "Error: Invalid argument!\n");
        rtc_help("rtc_wr");
        return -EINVAL;
    }

    rtc.tm_hour =  strtol(argv[5], &end, 0);
    if (*end || rtc.tm_hour < 0 || rtc.tm_hour > 23) {
        fprintf(stderr, "Error: Invalid argument!\n");
        rtc_help("rtc_wr");
        return -EINVAL;
    }

    rtc.tm_min =  strtol(argv[6], &end, 0);
    if (*end || rtc.tm_min < 0 || rtc.tm_min > 59) {
        fprintf(stderr, "Error: Invalid argument!\n");
        rtc_help("rtc_wr");
        return -EINVAL;
    }

    rtc.tm_sec =  strtol(argv[7], &end, 0);
    if (*end || rtc.tm_sec < 0 || rtc.tm_sec > 59) {
        fprintf(stderr, "Error: Invalid argument!\n");
        rtc_help("rtc_wr");
        return -EINVAL;
    }

    ret = snprintf(filename, I2C_MAX_NAME_SIZE, "/dev/rtc%d", rtc_num);
    filename[ret] = '\0';
    if ((fd = open(filename, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "`%s': %s\n", filename, strerror(errno));
        return -1;
    }

    if (ioctl(fd, RTC_SET_TIME, &rtc) < 0) {
        fprintf(stderr,
            "Error:  get rtc error %s\n",
            strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    printf("\r\n******************** RTC write test ********************\r\n");

    printf("Set RTC: %d-%d-%d, %d:%d:%d\n",
           year, rtc.tm_mon + 1, rtc.tm_mday,
           rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
    printf("\r\n******************** RTC write test End ********************\r\n\r\n");

    return 0;
}