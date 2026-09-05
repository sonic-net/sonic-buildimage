#ifndef __UAPI_GENL_PACKET_H
#define __UAPI_GENL_PACKET_H

enum {
	/* packet metadata */
	GENL_PACKET_ATTR_IIFINDEX,
	GENL_PACKET_ATTR_OIFINDEX,
	GENL_PACKET_ATTR_CONTEXT,
	GENL_PACKET_ATTR_DATA,

	__GENL_PACKET_ATTR_MAX
};

enum genl_packet_command {
	GENL_PACKET_CMD_PACKET,
};

/* Can be overridden at runtime by module option */
#define GENL_PACKET_ATTR_MAX (__GENL_PACKET_ATTR_MAX - 1)

#define GENL_PACKET_MCGRP_NAME "packets"
#define GENL_PACKET_NAME "genl_packet"
#define GENL_PACKET_VERSION 1
#endif /* __UAPI_GENL_PACKET_H */
