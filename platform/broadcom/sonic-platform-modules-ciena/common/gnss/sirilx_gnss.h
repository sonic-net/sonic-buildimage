#ifndef _SIRILX_GNSS_H
#define SIRILX_GNSS_H

#include <linux/serial_core.h>

#define GNSS_NAME "sirilx_ttyGNSS"

struct sirilx_gnss_platform_data {
	struct uart_port     port;
	const unsigned char *oflow_pattern;
	int                  oflow_size;
};

/* Five Trimble end packet sequences in a row. */
const unsigned char sirilx_gnss_dle_etx[] =
{ 0x10, 0x03, 0x10, 0x03, 0x10, 0x03, 0x10, 0x03, 0x10, 0x03 };

#endif
