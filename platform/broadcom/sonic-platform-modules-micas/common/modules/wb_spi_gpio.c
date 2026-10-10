#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/spi_gpio.h>
#include <linux/delay.h>

struct spi_gpio {
	struct spi_bitbang		bitbang;
	struct spi_gpio_platform_data	pdata;
	struct platform_device		*pdev;
	struct gpio_desc		*sck;
	struct gpio_desc		*miso;
	struct gpio_desc		*mosi;
	struct gpio_desc		**cs_gpios;
	bool				has_cs;
};

#ifndef DRIVER_NAME
#define DRIVER_NAME	"wb_spi_gpio"
#define GENERIC_BITBANG	/* vs tight inlines */
#endif

static inline struct spi_gpio *__pure
spi_to_spi_gpio(const struct spi_device *spi)
{
	const struct spi_bitbang	*bang;
	struct spi_gpio			*spi_gpio;
	bang = spi_controller_get_devdata(spi->controller);
	spi_gpio = container_of(bang, struct spi_gpio, bitbang);
	return spi_gpio;
}

static inline struct spi_gpio_platform_data *__pure
spi_to_pdata(const struct spi_device *spi)
{
	return &spi_to_spi_gpio(spi)->pdata;
}

static inline void setsck(const struct spi_device *spi, int is_on)
{
	struct spi_gpio *spi_gpio = spi_to_spi_gpio(spi);
	gpiod_set_value_cansleep(spi_gpio->sck, is_on);
}

static inline void setmosi(const struct spi_device *spi, int is_on)
{
	struct spi_gpio *spi_gpio = spi_to_spi_gpio(spi);
	gpiod_set_value_cansleep(spi_gpio->mosi, is_on);
}

static inline int getmiso(const struct spi_device *spi)
{
	struct spi_gpio *spi_gpio = spi_to_spi_gpio(spi);
	if (spi->mode & SPI_3WIRE)
		return !!gpiod_get_value_cansleep(spi_gpio->mosi);
	else
		return !!gpiod_get_value_cansleep(spi_gpio->miso);
}


#define spidelay(nsecs)	do {} while (0)
#define SPI_SCK_DELAY 	2 	/*  after setsck delay 2us */

#include "spi-bitbang-txrx.h"

static u32 spi_gpio_txrx_word_mode0(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits, unsigned flags)
{
	return bitbang_txrx_be_cpha0(spi, nsecs, 0, flags, word, bits);
}

static u32 spi_gpio_txrx_word_mode1(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits, unsigned flags)
{
	return bitbang_txrx_be_cpha1(spi, nsecs, 0, flags, word, bits);
}

static u32 spi_gpio_txrx_word_mode2(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits, unsigned flags)
{
	return bitbang_txrx_be_cpha0(spi, nsecs, 1, flags, word, bits);
}

static u32 spi_gpio_txrx_word_mode3(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits, unsigned flags)
{
	return bitbang_txrx_be_cpha1(spi, nsecs, 1, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode0(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits, unsigned flags)
{
	flags = spi->controller->flags;
	return bitbang_txrx_be_cpha0(spi, nsecs, 0, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode1(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits, unsigned flags)
{
	flags = spi->controller->flags;
	return bitbang_txrx_be_cpha1(spi, nsecs, 0, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode2(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits, unsigned flags)
{
	flags = spi->controller->flags;
	return bitbang_txrx_be_cpha0(spi, nsecs, 1, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode3(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits, unsigned flags)
{
	flags = spi->controller->flags;
	return bitbang_txrx_be_cpha1(spi, nsecs, 1, flags, word, bits);
}

static void spi_gpio_chipselect(struct spi_device *spi, int is_active)
{
	struct spi_gpio *spi_gpio = spi_to_spi_gpio(spi);
	if (is_active)
		gpiod_set_value_cansleep(spi_gpio->sck, spi->mode & SPI_CPOL);
	if (spi_gpio->has_cs) {
		struct gpio_desc *cs = spi_gpio->cs_gpios[spi_get_chipselect(spi, 0)];
		gpiod_set_value_cansleep(cs, (spi->mode & SPI_CS_HIGH) ? is_active : !is_active);
	}
}

static int spi_gpio_setup(struct spi_device *spi)
{
	struct gpio_desc	*cs;
	int			status = 0;
	struct spi_gpio		*spi_gpio = spi_to_spi_gpio(spi);
	cs = spi_gpio->cs_gpios[spi_get_chipselect(spi, 0)];
	if (!spi->controller_state && cs)
		status = gpiod_direction_output(cs,
						!(spi->mode & SPI_CS_HIGH));
	if (!status)
		status = spi_bitbang_setup(spi);
	return status;
}

static int spi_gpio_set_direction(struct spi_device *spi, bool output)
{
	struct spi_gpio *spi_gpio = spi_to_spi_gpio(spi);
	if (output)
		return gpiod_direction_output(spi_gpio->mosi, 1);
	else
		return gpiod_direction_input(spi_gpio->mosi);
}

static void spi_gpio_cleanup(struct spi_device *spi)
{
	spi_bitbang_cleanup(spi);
}

static int spi_gpio_request(struct device *dev,
			    struct spi_gpio *spi_gpio,
			    unsigned int num_chipselects,
			    u16 *mflags)
{
	int i;
	spi_gpio->mosi = devm_gpiod_get_optional(dev, "mosi", GPIOD_OUT_LOW);
	if (IS_ERR(spi_gpio->mosi))
		return PTR_ERR(spi_gpio->mosi);
	if (!spi_gpio->mosi)
		/* HW configuration without MOSI pin */
		*mflags |= SPI_CONTROLLER_NO_TX;
	spi_gpio->miso = devm_gpiod_get_optional(dev, "miso", GPIOD_IN);
	if (IS_ERR(spi_gpio->miso))
		return PTR_ERR(spi_gpio->miso);
	spi_gpio->sck = devm_gpiod_get(dev, "sck", GPIOD_OUT_LOW);
	if (IS_ERR(spi_gpio->sck))
		return PTR_ERR(spi_gpio->sck);
	for (i = 0; i < num_chipselects; i++) {
		spi_gpio->cs_gpios[i] = devm_gpiod_get_index(dev, "cs",
							     i, GPIOD_OUT_HIGH);
		if (IS_ERR(spi_gpio->cs_gpios[i]))
			return PTR_ERR(spi_gpio->cs_gpios[i]);
	}
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id spi_gpio_dt_ids[] = {
	{ .compatible = "wb-spi-gpio" },
	{}
};

MODULE_DEVICE_TABLE(of, spi_gpio_dt_ids);

static int spi_gpio_probe_dt(struct platform_device *pdev)
{
	int ret;
	u32 tmp;
	struct spi_gpio_platform_data	*pdata;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *of_id =
			of_match_device(spi_gpio_dt_ids, &pdev->dev);
	if (!of_id)
		return 0;
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;
	ret = of_property_read_u32(np, "num-chipselects", &tmp);
	if (ret < 0) {
		dev_err(&pdev->dev, "num-chipselects property not found\n");
		goto error_free;
	}
	pdata->num_chipselect = tmp;
	pdev->dev.platform_data = pdata;
	return 1;
error_free:
	devm_kfree(&pdev->dev, pdata);
	return ret;
}
#else
static inline int spi_gpio_probe_dt(struct platform_device *pdev)
{
	return 0;
}
#endif

static int spi_gpio_probe(struct platform_device *pdev)
{
	int				status;
	struct spi_controller		*master;
	struct spi_gpio			*spi_gpio;
	struct spi_gpio_platform_data	*pdata;
	u16 master_flags = 0;
	bool use_of = 0;
	status = spi_gpio_probe_dt(pdev);
	if (status < 0)
		return status;
	if (status > 0)
		use_of = 1;
	pdata = dev_get_platdata(&pdev->dev);
#ifdef GENERIC_BITBANG
	if (!pdata || (!use_of && !pdata->num_chipselect))
		return -ENODEV;
#endif
	master = spi_alloc_host(&pdev->dev, sizeof(*spi_gpio));
	if (!master)
		return -ENOMEM;
	spi_gpio = spi_controller_get_devdata(master);
	spi_gpio->cs_gpios = devm_kcalloc(&pdev->dev,
				pdata->num_chipselect,
				sizeof(*spi_gpio->cs_gpios),
				GFP_KERNEL);
	if (!spi_gpio->cs_gpios)
		return -ENOMEM;
	platform_set_drvdata(pdev, master);
	/* Determine if we have chip selects connected */
	spi_gpio->has_cs = !!pdata->num_chipselect;
	spi_gpio->pdev = pdev;
	if (pdata)
		spi_gpio->pdata = *pdata;
	status = spi_gpio_request(&pdev->dev, spi_gpio,
				  pdata->num_chipselect, &master_flags);
	if (status)
		return status;
	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(1, 32);
	master->mode_bits = SPI_3WIRE | SPI_CPHA | SPI_CPOL | SPI_CS_HIGH;
	master->flags = master_flags;
	master->bus_num = pdev->id;
	/* The master needs to think there is a chipselect even if not connected */
	master->num_chipselect = spi_gpio->has_cs ? pdata->num_chipselect : 1;
	master->setup = spi_gpio_setup;
	master->cleanup = spi_gpio_cleanup;
	if (pdev->dev.of_node) {
		master->dev.of_node = pdev->dev.of_node;
	}
	spi_gpio->bitbang.ctlr = master;
	spi_gpio->bitbang.chipselect = spi_gpio_chipselect;
	spi_gpio->bitbang.set_line_direction = spi_gpio_set_direction;
	if ((master_flags & SPI_CONTROLLER_NO_TX) == 0) {
		spi_gpio->bitbang.txrx_word[SPI_MODE_0] = spi_gpio_txrx_word_mode0;
		spi_gpio->bitbang.txrx_word[SPI_MODE_1] = spi_gpio_txrx_word_mode1;
		spi_gpio->bitbang.txrx_word[SPI_MODE_2] = spi_gpio_txrx_word_mode2;
		spi_gpio->bitbang.txrx_word[SPI_MODE_3] = spi_gpio_txrx_word_mode3;
	} else {
		spi_gpio->bitbang.txrx_word[SPI_MODE_0] = spi_gpio_spec_txrx_word_mode0;
		spi_gpio->bitbang.txrx_word[SPI_MODE_1] = spi_gpio_spec_txrx_word_mode1;
		spi_gpio->bitbang.txrx_word[SPI_MODE_2] = spi_gpio_spec_txrx_word_mode2;
		spi_gpio->bitbang.txrx_word[SPI_MODE_3] = spi_gpio_spec_txrx_word_mode3;
	}
	spi_gpio->bitbang.setup_transfer = spi_bitbang_setup_transfer;
	status = spi_bitbang_start(&spi_gpio->bitbang);
	if (status)
		spi_controller_put(master);
	return status;
}

static void spi_gpio_remove(struct platform_device *pdev)
{
	struct spi_controller		*ctlr;
	struct spi_gpio			*spi_gpio;
	ctlr = platform_get_drvdata(pdev);
	spi_gpio = spi_controller_get_devdata(ctlr);
	/* stop() unregisters child devices too */
	spi_bitbang_stop(&spi_gpio->bitbang);
	spi_controller_put(ctlr);
}

MODULE_ALIAS("platform:" DRIVER_NAME);
static struct platform_driver spi_gpio_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(spi_gpio_dt_ids),
	},
	.probe		= spi_gpio_probe,
	.remove		= spi_gpio_remove,
};

module_platform_driver(spi_gpio_driver);

MODULE_DESCRIPTION("SPI master driver using generic bitbanged GPIO ");
MODULE_AUTHOR("support");
MODULE_LICENSE("GPL");
