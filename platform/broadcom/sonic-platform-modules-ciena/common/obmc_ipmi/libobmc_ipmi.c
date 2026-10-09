#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <linux/ipmi.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "libobmc_ipmi.h"

#define OBMC_ERR_MSG(fmt, args...)              \
   do {                                         \
       int saved_errno = errno;                 \
       syslog(LOG_ERR, fmt, ## args);           \
       errno = saved_errno;                     \
   } while (0);

#define OBMC_MSG_LEN 3

static const char *obmc_ipmi_dev = "/dev/ipmi0";

/*
 * Definition lifted from:
 * https://github.com/openbmc/phosphor-host-ipmid/blob/master/include/ipmid/api-types.hpp
 */
enum obmc_ipmi_netfn {
   netFnApp     = 0x06,
   netFnStorage = 0x0a,
};

/*
 * Definitions lifted from:
 * https://bitbucket.ciena.com/projects/CEDIAG/repos/openbmc/browse/meta-ciena/meta-common/recipes-phosphor/ipmi/phosphor-ipmi-host/0001-Add-ipmid-raw-application-6-commands-for-Ciena.patch
 *
 * and:
 * https://github.com/ipmitool/ipmitool/blob/be11d948f89b10be094e28d8a0a5e8fb532c7b60/include/ipmitool/ipmi_mc.h#L47
 */
enum obmc_ipmi_cmdtype {
   cmdGetDeviceId = 0x01,
   cmdSetSelTime  = 0x49,
   cmdCienaType   = 0x90,
};

enum obmc_ciena_cmds {
   CIENA_CMD_BIOS_SEL = 0,
   CIENA_CMD_BMC_UPGD = 1,
};

struct ipm_devid_rsp {
	unsigned char device_id;
	unsigned char device_revision;
	unsigned char fw_rev1;
	unsigned char fw_rev2;
	unsigned char ipmi_version;
	unsigned char adtl_device_support;
	unsigned char manufacturer_id[3];
	unsigned char product_id[2];
	unsigned char aux_fw_rev[4];
};

/*
 * Definitions and hard-coded constants lifted from:
 * https://bitbucket.ciena.com/projects/CEDIAG/repos/openbmc/browse/meta-ciena/meta-common/recipes-ciena/obmc-installer/ciena-obmc-installer/obmc_hostipmi.sh#83
 */
enum obmc_ciena_opcodes {
   ciena_op_usb_download                 = 0,
   ciena_op_obmc_upgrade                 = 1,
   ciena_op_version_get                  = 2,
   ciena_op_upgrade_status               = 3,
   ciena_op_idp_write                    = 4,
   ciena_op_idp_status                   = 5,
   ciena_op_reset_request                = 6,
   ciena_op_obmc_spi_bank_get            = 8,
   ciena_op_installer_dbg                = 10,
   ciena_op_cpld_upgrade_skip            = 11,
   ciena_op_cpld_upgrade_force           = 12,
   ciena_op_obmc_kgi_update_enable       = 13,
   ciena_op_obmc_kgi_update_status_get   = 14,
   ciena_op_fan_idp_write                = 20,
   ciena_op_fan_idp_status               = 21,
   ciena_op_fru_idp_write                = 22,
   ciena_op_fru_idp_status               = 23,
   ciena_op_console_switch               = 30,
   ciena_op_major_revision               = 31,
   ciena_op_minor_revision               = 32,
   ciena_op_build_revision               = 33,
   ciena_op_console                      = 34,
   ciena_op_console_status               = 35,

};

enum obmc_ciena_op_cmds {
   /* bmc dev commands */
   IDP_CMD_DONE  = 0,
   IDP_CMD_READ  = 1,
   IDP_CMD_WRITE = 2,

   /* fan idp commands */
   FANIDP_CMD_VAL_IDLE       = 0,
   FANIDP_CMD_VAL_INPROGRESS = 1,
   FANIDP_CMD_VAL_WRITE      = 2,

   /* FPGA reset requests */
   CPLD_FPGA_RECONFIG    = 1,
   CPLD_DP_FPGA_RECONFIG = 2,
   CPLD_BOARD_RESET      = 3,
   CPLD_DP_BOARD_RESET   = 4,
   DP_FPGA_RECONFIG      = 5,

   /* magic return code */
   OBMC_CMD_DONE = 0x12,
};

struct obmc_ipmi_priv {
   struct ipmi_addr addr;
   int              fd;
   long             msg_id;
};

static long obmc_ipmi_msg_id;

static void obmc_ipmi_close(struct obmc_ipmi_priv *priv)
{
   TEMP_FAILURE_RETRY(close(priv->fd));
}

static int obmc_ipmi_open(struct obmc_ipmi_priv *priv)
{
   struct ipmi_system_interface_addr   bmc_addr = {
      .addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
      .channel   = IPMI_BMC_CHANNEL,
   };
   struct ipmi_channel_lun_address_set iclas    = {
      .channel = IPMI_BMC_CHANNEL,
      .value   = IPMI_BMC_SLAVE_ADDR,
   };
   int                                 addrlen  = sizeof(bmc_addr);
   int                                 rcv_ev   = 1;
   int                                 fd;
   int                                 rc;

   fd = open(obmc_ipmi_dev, O_RDWR);
   if (0 > fd) {
      OBMC_ERR_MSG("failed to open %s [%s]\n",
                   obmc_ipmi_dev, strerror(errno));
      return -errno;
   }

   priv->fd     = fd;
   priv->msg_id = obmc_ipmi_msg_id++;

   rc = ioctl(priv->fd, IPMICTL_SET_GETS_EVENTS_CMD, &rcv_ev);
   if (rc) {
      OBMC_ERR_MSG("failed IPMICTL_SET_GETS_EVENTS_CMD ioctl [%s]\n",
                   strerror(errno));
      goto out_close;
   }

   rc = ioctl(priv->fd, IPMICTL_SET_MY_CHANNEL_ADDRESS_CMD, &iclas);
   if (rc) {
      OBMC_ERR_MSG("failed IPMICTL_SET_MY_CHANNEL_ADDRESS_CMD ioctl [%s]\n",
                   strerror(errno));
      goto out_close;
   }

   if (sizeof(priv->addr) < addrlen) addrlen = sizeof(priv->addr);

   memcpy(&priv->addr, &bmc_addr, addrlen);

   return 0;

  out_close:
   obmc_ipmi_close(priv);
   return -errno;
}

static void obmc_ipmi_init_recv(struct obmc_ipmi_priv *priv,
                                struct ipmi_recv      *ir,
                                struct ipmi_msg       *im)
{
   ir->addr     = (unsigned char *) &priv->addr;
   ir->addr_len = sizeof(priv->addr);
   ir->msg      = *im;
}

static int obmc_wait_response(struct obmc_ipmi_priv *priv,
                              struct ipmi_msg       *im)
{
   struct ipmi_recv ir = {};
   int              rc;

   obmc_ipmi_init_recv(priv, &ir, im);

   while (1) {
      struct timeval wait_tv = { 1, 0 };
      fd_set         obfds;

      FD_ZERO(&obfds);
      FD_SET(priv->fd, &obfds);

      rc = TEMP_FAILURE_RETRY(select(priv->fd + 1, &obfds, NULL, NULL,
                                     &wait_tv));
      if (0 > rc) {
         OBMC_ERR_MSG("failed select [%s]\n", strerror(errno));
         return -errno;
      }
      if (0 == rc) {
         OBMC_ERR_MSG("select timeout\n");
         return -ETIMEDOUT;
      }

      rc = ioctl(priv->fd, IPMICTL_RECEIVE_MSG_TRUNC, &ir);
      if (rc) {
         OBMC_ERR_MSG("failed IPMICTL_RECEIVE_MSG_TRUNC ioctl [%s]\n",
                      strerror(errno));
         return -errno;
      }

      if (priv->msg_id != ir.msgid) continue;

      break;
   }

   return 0;
}

static void obmc_ipmi_init_req(struct obmc_ipmi_priv *priv,
                               struct ipmi_req       *ir,
                               struct ipmi_msg       *im)
{
   ir->addr     = (unsigned char *) &priv->addr;
   ir->addr_len = sizeof(priv->addr);
   ir->msgid    = priv->msg_id;
   ir->msg      = *im;
}

static int obmc_ipmi_send_cmd_rsp(struct ipmi_msg *im_cmd,
                                  struct ipmi_msg *im_rsp)
{
   struct obmc_ipmi_priv priv = {};
   struct ipmi_req       ir   = {};
   int                   rc;

   rc = obmc_ipmi_open(&priv);
   if (rc) return rc;

   obmc_ipmi_init_req(&priv, &ir, im_cmd);

   rc = ioctl(priv.fd, IPMICTL_SEND_COMMAND, &ir);

   if (!rc) rc = obmc_wait_response(&priv, im_rsp);
   else {
      OBMC_ERR_MSG("failed IPMICTL_SEND_COMMAND ioctl [%s]\n",
                   strerror(errno));
      rc = -errno;
   }

   obmc_ipmi_close(&priv);

   return rc;
}

static int obmc_ipmi_send_ciena_command(enum obmc_ciena_opcodes cmd,
                                        unsigned char           param)
{
   unsigned char   raw_req[OBMC_MSG_LEN] = { CIENA_CMD_BMC_UPGD, cmd, param };
   unsigned char   raw_rsp[OBMC_MSG_LEN] = {};
   struct ipmi_msg im_req                = {};
   struct ipmi_msg im_rsp                = {};
   int             rc;

   im_req.netfn    = netFnApp;
   im_req.cmd      = cmdCienaType;
   im_req.data     = raw_req;
   im_req.data_len = sizeof(raw_req);

   im_rsp.netfn    = netFnApp;
   im_rsp.cmd      = cmdCienaType;
   im_rsp.data     = raw_rsp;
   im_rsp.data_len = sizeof(raw_rsp);

   rc = obmc_ipmi_send_cmd_rsp(&im_req, &im_rsp);
   if (!rc) {
      if (raw_rsp[2] != OBMC_CMD_DONE) {
         OBMC_ERR_MSG("unexpected return code: 0x%x\n", raw_rsp[2]);
         rc = -EINVAL;
      }
      else rc = (int) raw_rsp[1];
   }

   return rc;
}

int obmc_ipmi_main_idp_flush()
{
   return obmc_ipmi_send_ciena_command(ciena_op_idp_write, 0);
}

int obmc_ipmi_main_idp_status()
{
   return obmc_ipmi_send_ciena_command(ciena_op_idp_status, 0);
}

int obmc_ipmi_fpga_reconfig(enum obmc_ipmi_reconfig_type reconfig_type)
{
   enum obmc_ciena_op_cmds reconf_type;

   if (OBMC_IPMI_RECONFIG_CP == reconfig_type)
      reconf_type = CPLD_FPGA_RECONFIG;
   else if (OBMC_IPMI_RECONFIG_DP == reconfig_type)
      reconf_type = DP_FPGA_RECONFIG;
   else reconf_type = CPLD_DP_FPGA_RECONFIG;

   return obmc_ipmi_send_ciena_command(ciena_op_reset_request,
                                       reconf_type);
}

int obmc_ipmi_board_reset(bool all)
{
   return obmc_ipmi_send_ciena_command(ciena_op_reset_request,
                                       (all ?
                                        CPLD_DP_BOARD_RESET :
                                        CPLD_BOARD_RESET));
}

int obmc_ipmi_fan_idp_flush(unsigned fan_num)
{
   return obmc_ipmi_send_ciena_command(ciena_op_fan_idp_write, fan_num);
}

int obmc_ipmi_fan_idp_status(unsigned fan_num)
{
   return obmc_ipmi_send_ciena_command(ciena_op_fan_idp_status, fan_num);
}

int obmc_ipmi_fru_idp_flush(unsigned fru_num)
{
   return obmc_ipmi_send_ciena_command(ciena_op_fru_idp_write, fru_num);
}

int obmc_ipmi_fru_idp_status()
{
   return obmc_ipmi_send_ciena_command(ciena_op_fru_idp_status, 0);
}

int obmc_ipmi_sel_time_set_now()
{
   unsigned char   rsp[OBMC_MSG_LEN] = {};
   struct ipmi_msg im_req            = {};
   struct ipmi_msg im_rsp            = {};
   unsigned        tsec;
   int             rc;

   /* FIXME: the openIPMI standard better support a wider time
    * register before 2038. */
   tsec = (unsigned) time(NULL);

   im_req.netfn    = netFnStorage;
   im_req.cmd      = cmdSetSelTime;
   im_req.data     = (unsigned char *) &tsec;
   im_req.data_len = sizeof(tsec);

   im_rsp.data     = rsp;
   im_rsp.data_len = sizeof(rsp);

   rc = obmc_ipmi_send_cmd_rsp(&im_req, &im_rsp);
   if (!rc) rc = rsp[0];

   return rc;
}

int obmc_ipmi_upgrade_inform()
{
   return obmc_ipmi_send_ciena_command(ciena_op_obmc_upgrade, 0);
}

int obmc_ipmi_brd_send_and_cpld_version_get(unsigned brd_rev)
{
   return obmc_ipmi_send_ciena_command(ciena_op_version_get, brd_rev);
}

int obmc_ipmi_spi_bank_get()
{
   return obmc_ipmi_send_ciena_command(ciena_op_obmc_spi_bank_get, 0);
}

int obmc_ipmi_kgi_update_enable(int kgi_switch)
{
   return obmc_ipmi_send_ciena_command(ciena_op_obmc_kgi_update_enable,
                                       kgi_switch);
}

int obmc_ipmi_kgi_update_status_get()
{
   return obmc_ipmi_send_ciena_command(ciena_op_obmc_kgi_update_status_get, 0);
}

int obmc_ipmi_host_console_switch(unsigned console_type)
{
   return obmc_ipmi_send_ciena_command(ciena_op_console_switch, console_type);
}

int obmc_ipmi_major_revision_number_send(unsigned char version_number)
{
   return obmc_ipmi_send_ciena_command(ciena_op_major_revision,
                                       version_number);
}

int obmc_ipmi_minor_revision_number_send(unsigned char version_number)
{
   return obmc_ipmi_send_ciena_command(ciena_op_minor_revision,
                                       version_number);
}

int obmc_ipmi_build_revision_number_send(unsigned char version_number)
{
   return obmc_ipmi_send_ciena_command(ciena_op_build_revision,
                                       version_number);
}

int obmc_ipmi_upgrade_status_get()
{
   return obmc_ipmi_send_ciena_command(ciena_op_upgrade_status, 0);
}

int obmc_ipmi_usb_image_download(unsigned int image_size)
{
   return obmc_ipmi_send_ciena_command(ciena_op_usb_download, image_size);
}

int obmc_ipmi_serial_console(unsigned state){
   return obmc_ipmi_send_ciena_command(ciena_op_console, state);
}

int obmc_ipmi_serial_console_status_get(){
   return obmc_ipmi_send_ciena_command(ciena_op_console_status, 0);
}

int obmc_ipmi_fw_version_get()
{
   struct ipm_devid_rsp *devid;
   unsigned char         raw_rsp[1 + sizeof(*devid)] = {};
   struct ipmi_msg       im_req                      = {};
   struct ipmi_msg       im_rsp                      = {};
   int                   rc;

   im_req.netfn    = netFnApp;
   im_req.cmd      = cmdGetDeviceId;

   im_rsp.netfn    = netFnApp;
   im_rsp.cmd      = cmdGetDeviceId;
   im_rsp.data     = raw_rsp;
   im_rsp.data_len = sizeof(raw_rsp);

   rc = obmc_ipmi_send_cmd_rsp(&im_req, &im_rsp);
   if (rc)
      return rc;

   if (raw_rsp[0]) {
      OBMC_ERR_MSG("unexpected copletion code: 0x%x\n", raw_rsp[0]);
      return -1;
   }

   devid = (struct ipm_devid_rsp *) &raw_rsp[1];

   return (devid->aux_fw_rev[1] << 8) | devid->aux_fw_rev[0];
}
