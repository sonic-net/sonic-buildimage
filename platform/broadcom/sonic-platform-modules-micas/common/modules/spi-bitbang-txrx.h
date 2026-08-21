static inline u32
bitbang_txrx_be_cpha0(struct spi_device *spi,
		unsigned nsecs, unsigned cpol, unsigned flags,
		u32 word, u8 bits)
{
	/* if (cpol == 0) this is SPI_MODE_0; else this is SPI_MODE_2 */
	u32 oldbit = (!(word & (1<<(bits-1)))) << 31;
	/* clock starts at inactive polarity */
	for (word <<= (32 - bits); likely(bits); bits--) {
		/* setup MSB (to slave) on trailing edge */
		if ((flags & SPI_CONTROLLER_NO_TX) == 0) {
			if ((word & (1 << 31)) != oldbit) {
				setmosi(spi, word & (1 << 31));
				oldbit = word & (1 << 31);
			}
		}
		spidelay(nsecs);	/* T(setup) */
		setsck(spi, !cpol);
		spidelay(nsecs);
		/* d17xx cpu gpio in iio,hardwarn may not respond software command in time,so add 2us delay before sampling to ensure get correct data*/
		udelay(SPI_SCK_DELAY);
		/* sample MSB (from slave) on leading edge */
		word <<= 1;
		if ((flags & SPI_CONTROLLER_NO_RX) == 0)
			word |= getmiso(spi);
		setsck(spi, cpol);
	}
	return word;
}
static inline u32
bitbang_txrx_be_cpha1(struct spi_device *spi,
		unsigned nsecs, unsigned cpol, unsigned flags,
		u32 word, u8 bits)
{
	/* if (cpol == 0) this is SPI_MODE_1; else this is SPI_MODE_3 */
	u32 oldbit = (!(word & (1<<(bits-1)))) << 31;
	/* clock starts at inactive polarity */
	for (word <<= (32 - bits); likely(bits); bits--) {
		/* setup MSB (to slave) on leading edge */
		setsck(spi, !cpol);
		if ((flags & SPI_CONTROLLER_NO_TX) == 0) {
			if ((word & (1 << 31)) != oldbit) {
				setmosi(spi, word & (1 << 31));
				oldbit = word & (1 << 31);
			}
		}
		spidelay(nsecs); /* T(setup) */
		setsck(spi, cpol);
		spidelay(nsecs);
		/* sample MSB (from slave) on trailing edge */
		word <<= 1;
		if ((flags & SPI_CONTROLLER_NO_RX) == 0)
			word |= getmiso(spi);
	}
	return word;
}
