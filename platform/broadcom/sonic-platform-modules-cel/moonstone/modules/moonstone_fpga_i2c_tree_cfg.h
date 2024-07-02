// moonstone FPGA i2c tree configuration

#define I2C_BUS_OFS		7
#define SWITCH_CPLD_ADAP_NUM	2

/* i2c_bus_config - an i2c-core resource and platform data
 *  @id - I2C bus device ID, for identification.
 *  @res - resources for an i2c-core device.
 *  @num_res - size of the resources.
 */
struct i2c_bus_config {
	int id;
	struct resource *res;
	ssize_t num_res;
};

/* RESOURCE SEPERATES BY FUNCTION */
/* Resource IOMEM for i2c bus 1 */
static struct resource cls_i2c_res_0[] = {
	{
		.start = 0x00010000, .end = 0x00010FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 2 */
static struct resource  cls_i2c_res_1[] = {
	{
		.start = 0x00011000, .end = 0x00011FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 3 */
static struct resource  cls_i2c_res_2[] = {
	{
		.start = 0x00012000, .end = 0x00012FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 4 */
static struct  resource cls_i2c_res_3[] = {
	{
		.start = 0x00013000, .end = 0x00013FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 5 */
static struct resource  cls_i2c_res_4[] = {
	{
		.start = 0x00014000, .end = 0x00014FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 6 */
static struct resource  cls_i2c_res_5[] = {
	{
		.start = 0x00015000, .end = 0x00015FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 7 */
static struct resource  cls_i2c_res_6[] = {
	{
		.start = 0x00016000, .end = 0x00016FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 8 */
static struct resource  cls_i2c_res_7[] = {
	{
		.start = 0x00017000, .end = 0x00017FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 9 */
static struct resource  cls_i2c_res_8[] = {
	{
		.start = 0x00018000, .end = 0x00018FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 10 */
static struct resource  cls_i2c_res_9[] = {
	{
		.start = 0x00019000, .end = 0x00019FFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 11 */
static struct resource  cls_i2c_res_10[] = {
	{
		.start = 0x0001A000, .end = 0x0001AFFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 12 */
static struct resource  cls_i2c_res_11[] = {
	{
		.start = 0x0001B000, .end = 0x0001BFFF,
		.flags = IORESOURCE_MEM,},
};

/* Resource IOMEM for i2c bus 13 */
static struct resource  cls_i2c_res_12[] = {
	{
		.start = 0x0001C000, .end = 0x0001CFFF,
		.flags = IORESOURCE_MEM,},
};

static struct i2c_bus_config i2c_bus_configs[] = {
	{
		.id = 0,
		.res = cls_i2c_res_0,
		.num_res = ARRAY_SIZE(cls_i2c_res_0),
	},
	{
		.id = 1,
		.res = cls_i2c_res_1,
		.num_res = ARRAY_SIZE(cls_i2c_res_1),
	},
	{
		.id = 2,
		.res = cls_i2c_res_2,
		.num_res = ARRAY_SIZE(cls_i2c_res_2),
	},
	{
		.id = 3,
		.res = cls_i2c_res_3,
		.num_res = ARRAY_SIZE(cls_i2c_res_3),
	},
	{
		.id = 4,
		.res = cls_i2c_res_4,
		.num_res = ARRAY_SIZE(cls_i2c_res_4),
	},
	{
		.id = 5,
		.res = cls_i2c_res_5,
		.num_res = ARRAY_SIZE(cls_i2c_res_5),
	},
	/* only available on nonBMC */
	{
		.id = 100,
		.res = cls_i2c_res_6,
		.num_res = ARRAY_SIZE(cls_i2c_res_6),
	},
	{
		.id = 101,
		.res = cls_i2c_res_7,
		.num_res = ARRAY_SIZE(cls_i2c_res_7),
	},
	{
		.id = 102,
		.res = cls_i2c_res_8,
		.num_res = ARRAY_SIZE(cls_i2c_res_8),
	},
	{
		.id = 103,
		.res = cls_i2c_res_9,
		.num_res = ARRAY_SIZE(cls_i2c_res_9),
	},
	{
		.id = 104,
		.res = cls_i2c_res_10,
		.num_res = ARRAY_SIZE(cls_i2c_res_10),
	},
	{
		.id = 105,
		.res = cls_i2c_res_11,
		.num_res = ARRAY_SIZE(cls_i2c_res_11),
	},
	{
		.id = 106,
		.res = cls_i2c_res_12,
		.num_res = ARRAY_SIZE(cls_i2c_res_12),
	},
};