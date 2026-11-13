struct spi_ciena_fpga_platform_data {
	int      num_chipselect;
	int      bus_num;
	int      width;       /* bus width (16 or 32) for register accesses */
	int      big_endian;  /* bus endianness for register accesses */
	int      ctrlreg_err; /* error bits are in the SPI_CTRL register */
	int      fifo_depth;  /* FPGA write/read FIFO depth */
	int      ppos_size;
	uint32_t xfer_delay;   /* default inter-message transfer delay */
	bool     gpio_chipsel; /* enable chip select gpios */
};
