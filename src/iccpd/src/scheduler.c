/*
 * scheduler.c
 *
 * Copyright(c) 2016-2019 Nephos/Estinet.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 *  Maintainer: jianjun, grace Li from nephos
 */

#include <unistd.h>
#include <pthread.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "../include/logger.h"
#include "../include/system.h"
#include "../include/scheduler.h"
#include "../include/iccp_csm.h"
#include "../include/iccp_ifm.h"
#include "../include/iccp_cmd.h"
#include "../include/mlacp_link_handler.h"
#include "../include/iccp_netlink.h"

/******************************************************
*
*    Global Variable
*
******************************************************/

//this needs to be fine tuned
#define PEER_SOCK_SND_BUF_LEN  (6 * 1024 * 1024)
#define PEER_SOCK_RCV_BUF_LEN  (6 * 1024 * 1024)
#define RECV_RETRY_INTERVAL_USEC    100000
#define RECV_RETRY_MAX              10

extern int mlacp_prepare_for_warm_reboot(struct CSM* csm, char* buf, size_t max_buf_size);

static int session_conn_thread_lock(pthread_mutex_t *conn_mutex)
{
    return 1; /*pthread_mutex_lock(conn_mutex);*/
}

static int session_conn_thread_trylock(pthread_mutex_t *conn_mutex)
{
    return 0; /*pthread_mutex_trylock(conn_mutex);*/
}

static int session_conn_thread_unlock(pthread_mutex_t *conn_mutex)
{
    return 1;/* pthread_mutex_unlock(conn_mutex);*/
}

static void heartbeat_check(struct CSM *csm)
{
    if (csm->heartbeat_update_time == 0)
    {
        time(&csm->heartbeat_update_time);
        return;
    }

    if ( (time(NULL) - csm->heartbeat_update_time) > csm->session_timeout)
    {
        /* hearbeat timeout*/
        ICCPD_LOG_WARN("ICCP_FSM", "iccpd connection timeout (heartbeat)");
        scheduler_session_disconnect_handler(csm);
    }

    return;
}

static void heartbeat_update(struct CSM *csm)
{
    if (csm->sock_fd > 0)
    {
        heartbeat_check(csm);
    }

    return;
}

/* Transit FSM of all connections */
static int scheduler_transit_fsm()
{
    struct CSM* csm = NULL;
    struct System* sys = NULL;

    if ((sys = system_get_instance()) == NULL)
        return MCLAG_ERROR;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        heartbeat_update(csm);
        iccp_csm_transit(csm);
        app_csm_transit(csm);
        mlacp_fsm_transit(csm);
    }

    //lif->changed flag is marked for state change for lif, for active node when
    //it is in STAGE2 where it receiving cfg sync from peer if there is any po
    //state change that change is not sent and this clear clears the marking and
    //thus remote state is not updated; so commenting this out
    //local_if_change_flag_clear();
    local_if_purge_clear();

    return 1;
}

/* Receive packets call back function */
int scheduler_csm_read_callback(struct CSM* csm)
{
    struct Msg* msg = NULL;
    /*peer message*/
    char *peer_msg = g_csm_buf;
    LDPHdr* ldp_hdr = (LDPHdr*)peer_msg;
    char* data = &peer_msg[sizeof(LDPHdr)];
    size_t data_len = 0;
    size_t pos = 0;
    int recv_len = 0, len = 0, retval;
    int num_retry = 0;
    int total_retry_time = 0;
    int total_data_len = 0;

    if (csm->sock_fd <= 0)
        return MCLAG_ERROR;

    memset(peer_msg, 0, CSM_BUFFER_SIZE);

    recv_len = 0;
    errno = 0;

    while (recv_len != sizeof(LDPHdr))
    {
        len = recv(csm->sock_fd, peer_msg + recv_len, sizeof(LDPHdr) - recv_len, 0);
        if (len == -1)
        {
            perror("recv(). Error");
            ICCPD_LOG_WARN("ICCP_FSM", "Peer disconnect for header read error[%s] len = -1 till now received len  = %d ", strerror(errno), recv_len);
            SYSTEM_INCR_HDR_READ_SOCK_ERR_COUNTER(system_get_instance());
            goto recv_err;
        }
        else if (len == 0)
        {
            ICCPD_LOG_WARN("ICCP_FSM", "Peer disconnect for header read error[%s] len = 0, till now received len = %d ",strerror(errno), recv_len);
            SYSTEM_INCR_HDR_READ_SOCK_ZERO_LEN_COUNTER(system_get_instance());
            goto recv_err;
        }
        recv_len += len;
        /*usleep(100);*/
    }

    if (ntohs(ldp_hdr->msg_len) >= MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS)
    {
        data_len = ntohs(ldp_hdr->msg_len) - MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS;
    }
    else
    {
        ICCPD_LOG_ERR("ICCP_FSM", "Peer disconnect for invalid data error; length[%d] msg_type[0x%x] ", ntohs(ldp_hdr->msg_len),  ntohs(ldp_hdr->msg_type));
        SYSTEM_INCR_INVALID_PEER_MSG_COUNTER(system_get_instance());
        goto recv_err;
    }
    total_data_len = data_len;
    pos = 0;

    while (data_len > 0)
    {
        /* When consecutive CCP session flaps happen, recv() call got stuck.
         * Change recv() to non-blocking with retry mechanism. If max
         * retry reaches, attempt one more retry after waiting for one KA
         * interval before bringing down the session.
         */
        recv_len = recv(csm->sock_fd, &data[pos], data_len, MSG_DONTWAIT);
        if (recv_len == -1)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                ICCPD_LOG_NOTICE(
                    "ICCP_FSM", "Non-blocking recv total/pending len %d/%d, errno %d, num_retry %d",
                    total_data_len, data_len, errno, num_retry);
                ++num_retry;
                if (num_retry > RECV_RETRY_MAX)
                {
                    ICCPD_LOG_ERR(
                        "ICCP_FSM", "Non-blocking recv() retry failed, total/pending len %d/%d",
                        total_data_len, data_len);
                    SYSTEM_INCR_RX_RETRY_FAIL_COUNTER(system_get_instance());
                    goto recv_err;
                }
                else
                {
                    if (num_retry == RECV_RETRY_MAX)
                        usleep((csm->session_timeout * 1000000) - total_retry_time);
                    else
                    {
                        total_retry_time += (num_retry * RECV_RETRY_INTERVAL_USEC);
                        usleep(num_retry * RECV_RETRY_INTERVAL_USEC);
                    }
                    recv_len = 0;
                }
            }
            else
            {
                perror("continue recv() Error");
                ICCPD_LOG_WARN("ICCP_FSM", "Peer data read error; recv_len:%d data_len:%d, errno %d",
                               recv_len, data_len, errno);
                SYSTEM_INCR_TLV_READ_SOCK_ERR_COUNTER(system_get_instance());
                goto recv_err;
            }
        }
        else if (recv_len == 0)
        {
            ICCPD_LOG_WARN("ICCP_FSM", "Peer disconnect for data read error; recv_len:%d data_len:%d ", recv_len, data_len);
            SYSTEM_INCR_TLV_READ_SOCK_ZERO_LEN_COUNTER(system_get_instance());
            goto recv_err;
        }
        data_len -= recv_len;
        pos += recv_len;
        /*usleep(100);*/
    }
    if (num_retry > 0)
    {
        SYSTEM_SET_RETRY_COUNTER(system_get_instance(), num_retry);
    }
    retval = iccp_csm_init_msg(&msg, peer_msg, ntohs(ldp_hdr->msg_len) + MSG_L_INCLUD_U_BIT_MSG_T_L_FIELDS);
    if (retval == 0)
    {
        iccp_csm_enqueue_msg(csm, msg);
        ++csm->icc_msg_in_count;
    }
    else
        ++csm->i_msg_in_count;

    return 1;

 recv_err:
    scheduler_session_disconnect_handler(csm);
    return MCLAG_ERROR;
}

/* Handle server accept client */
int scheduler_server_accept()
{
    int new_fd;
    int ret = MCLAG_ERROR;
    struct CSM* csm = NULL;
    struct System* sys = NULL;
    struct sockaddr_in client_addr;
    socklen_t addr_len;

    if ((sys = system_get_instance()) == NULL )
    {
        return MCLAG_ERROR;
    }
    if (sys->server_fd <= 0)
    {
        return MCLAG_ERROR;
    }

    addr_len = sizeof(struct sockaddr_in);
    new_fd = accept(sys->server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (new_fd == -1)
    {
        goto reject_client;
    }
    else
    {
        csm = system_get_csm_by_peer_ip(inet_ntoa(client_addr.sin_addr));
        if (!csm)
        {
            /* can't find csm with peer ip*/
            ICCPD_LOG_NOTICE("ICCP_FSM", "csm null with peer ip [%s]", inet_ntoa(client_addr.sin_addr));
            goto reject_client;
        }

        if (csm->sock_fd > 0)
        {
            /* peer already connected*/
            ICCPD_LOG_NOTICE("ICCP_FSM", "csm sock is connected with peer ip [%s]", inet_ntoa(client_addr.sin_addr));
            goto reject_client;
        }

        if ((ret = scheduler_check_csm_config(csm)) < 0)
        {
            /* csm config error*/
            ICCPD_LOG_NOTICE("ICCP_FSM", "csm config error with peer ip [%s]", inet_ntoa(client_addr.sin_addr));
            goto reject_client;
        }
    }

    /* Accept*/
    goto accept_client;

 reject_client:
    if (new_fd >= 0)
        close(new_fd);
    return MCLAG_ERROR;

 accept_client:
    session_conn_thread_lock(&csm->conn_mutex);
    ICCPD_LOG_INFO(__FUNCTION__, "Server Accept, SocketFD [%d], %p", new_fd, csm);

    struct epoll_event event;
    int err;
    int send_buf_len = PEER_SOCK_SND_BUF_LEN;
    int recv_buf_len = PEER_SOCK_RCV_BUF_LEN;
    event.data.fd = new_fd;
    event.events = EPOLLIN;
    err = epoll_ctl(sys->epoll_fd, EPOLL_CTL_ADD, new_fd, &event);
    if (err)
    {
        session_conn_thread_unlock(&csm->conn_mutex);
        goto reject_client;
    }

    csm->sock_fd = new_fd;
    if (setsockopt(csm->sock_fd, SOL_SOCKET, SO_SNDBUF, &send_buf_len, sizeof(send_buf_len)) == -1)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Set socket send buf option failed. Error");
    }
    if (setsockopt(csm->sock_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_len, sizeof(recv_buf_len)) == -1)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Set socket recv buf option failed. Error");
    }
    csm->current_state = ICCP_NONEXISTENT;
    FD_SET(new_fd, &(sys->readfd));
    sys->readfd_count++;
    session_conn_thread_unlock(&csm->conn_mutex);
    return 0;
}

void iccp_get_start_type(struct System* sys)
{
    FILE* fp;

    memset(g_csm_buf, 0, CSM_BUFFER_SIZE);

    fp = fopen("/proc/cmdline", "r");
    if (!fp)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Error: Can't open file /proc/cmdline!");
        return;
    }

    fread(g_csm_buf, CSM_BUFFER_SIZE, 1, fp);
    (void)fclose(fp);

    if (strstr(g_csm_buf, "SONIC_BOOT_TYPE=warm"))
        sys->warmboot_start = WARM_REBOOT;

    ICCPD_LOG_DEBUG("ICCP_FSM", "Start ICCP: warm reboot %s",
        (sys->warmboot_start == WARM_REBOOT)? "yes" : "no");
    return;
}

/* scheduler initialization */
void scheduler_init()
{
    struct System* sys = NULL;

    if (!(sys = system_get_instance()))
        return;

    iccp_get_start_type(sys);
    /*Get kernel interface and port */
    iccp_sys_local_if_list_get_init();
    iccp_sys_local_if_list_get_addr();
    /*Interfaces must be created before this func called*/
    //no need to create iccpd config from startup file, it will be done through
    //cli
    iccp_config_from_file(sys->config_file_path);

    /*Get kernel ARP info */
    iccp_neigh_get_init();

    if (iccp_connect_syncd() < 0)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Syncd info socket connect fail");
    }
    else
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Syncd info socket connect success");
    }

    if (mclagd_ctl_sock_create() < 0)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Mclagd ctl info socket connect fail");
    }

    return;
}

void mlacp_sync_send_warmboot_flag()
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;
    int msg_len = 0;

    if ((sys = system_get_instance()) == NULL)
        return;

    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        if (MLACP(csm).current_state == MLACP_STATE_EXCHANGE)
        {
            memset(g_csm_buf, 0, CSM_BUFFER_SIZE);
            msg_len = mlacp_prepare_for_warm_reboot(csm, g_csm_buf, CSM_BUFFER_SIZE);
            iccp_csm_send(csm, g_csm_buf, msg_len);
        }
    }
    ICCPD_LOG_DEBUG("ICCP_FSM", "Send warmboot flag to peer. Start warmboot");
    return;
}

int iccp_receive_signal_handler(struct System* sys)
{
    char ctrl_byte;
    int err = 0;

    err = read(sys->sig_pipe_r, &ctrl_byte, 1);
    if (err == -1)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Read sig_pipe_r fail !");
        return err;
    }

    switch (ctrl_byte)
    {
        case 'w':
            /*send packet to peer*/
            mlacp_sync_send_warmboot_flag();
            sys->warmboot_exit = WARM_REBOOT;
            break;

        default:
            break;
    }

    return 0;
}

/* Thread fetch to call */
void scheduler_loop()
{
    struct System* sys = NULL;

    if ((sys = system_get_instance()) == NULL)
        return;

    while (1)
    {
        if (sys->sync_fd <= 0)
        {
            iccp_connect_syncd();
        }

        /*handle socket slelect event ,If no message received, it will block 0.1s*/
        iccp_handle_events(sys);
        /*csm, app state machine transit */
        scheduler_transit_fsm();

        if (sys->warmboot_exit == WARM_REBOOT)
        {
            ICCPD_LOG_DEBUG(__FUNCTION__, "Warm reboot exit ......");
            return;
        }
    }

    return;
}

/*****************************************
* Sync portchannel  MAC with kernel
*
* ***************************************/
int mlacp_sync_with_kernel_callback()
{
    struct System* sys = NULL;
    struct CSM* csm = NULL;
    struct LocalInterface* local_if = NULL;

    if ((sys = system_get_instance()) == NULL)
    {
        goto out;
    }

    /* traverse all CSM */
    LIST_FOREACH(csm, &(sys->csm_list), next)
    {
        /* Sync MLAG po state with kernel*/
        LIST_FOREACH(local_if, &(MLACP(csm).lif_list), mlacp_next)
        {
            if (local_if->type == IF_T_PORT_CHANNEL)
            {
                /* sync system info from one port-channel device*/
                if (memcmp(MLACP(csm).system_id, local_if->mac_addr, ETHER_ADDR_LEN) != 0)
                {
                    memcpy(MLACP(csm).system_id, local_if->mac_addr, ETHER_ADDR_LEN);
                    MLACP(csm).system_config_changed = 1;
                    break;
                }
            }
        }
    }

 out:
    return 0;
}

/* Scheduler start while loop */
void scheduler_start()
{
    /*mlacp_sync_with_kernel_callback();*/

    scheduler_loop();

    return;
}

/* Scheduler tear down */
void scheduler_finalize()
{
    struct System* sys = NULL;

    if ((sys = system_get_instance()) == NULL)
        return;

    syncd_info_close();

    log_finalize();

    ICCPD_LOG_INFO(__FUNCTION__, "Scheduler is terminated.");

    return;
}

void session_client_conn_handler(struct CSM *csm)
{
    struct System* sys = NULL;
    struct sockaddr_in peer_addr;
    int connFd = -1, connStat = -1;
    struct timeval con_tv;
    socklen_t len = sizeof(con_tv);
    int err = 0;

    struct sockaddr_in src_addr;
    bzero(&(src_addr), sizeof(src_addr));
    src_addr.sin_family = PF_INET;
    src_addr.sin_port = 0;
    src_addr.sin_addr.s_addr = inet_addr(csm->sender_ip);

    /* Lock the thread*/
    session_conn_thread_lock(&csm->conn_mutex);

    sys = system_get_instance();
    if (!sys)
        goto conn_fail;

    /* Create sock*/
    connFd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&peer_addr, sizeof(peer_addr));
    peer_addr.sin_family = PF_INET;
    peer_addr.sin_port = htons(ICCP_TCP_PORT);
    peer_addr.sin_addr.s_addr = inet_addr(csm->peer_ip);
    if (connFd == -1)
    {
        ICCPD_LOG_DEBUG(__FUNCTION__, "Peer IP:%s Socket FD creation failed.",
                        csm->peer_ip);
        goto conn_fail;
    }

    /* Set connect timeout secs*/
    con_tv.tv_sec = 0;
    con_tv.tv_usec = CONNECT_TIMEOUT_MSEC * 1000;
    if (setsockopt(connFd, SOL_SOCKET, SO_SNDTIMEO, &con_tv, len) == -1)
    {
        ICCPD_LOG_INFO(__FUNCTION__, "Set socket timeout fail");
    }

    err = bind(connFd, (struct sockaddr*)&(src_addr), sizeof(src_addr));
    if (err < 0)
    {
        ICCPD_LOG_INFO(__FUNCTION__, "Bind socket failed. Error = %d errno = %d ",err,errno);
        goto conn_fail;
    }

    /* Try conn*/
    ICCPD_LOG_INFO(__FUNCTION__, "Connecting. peer ip = [%s], %p", csm->peer_ip, csm);
    connStat = connect(connFd, (struct sockaddr*)&(peer_addr), sizeof(peer_addr));
    ICCPD_LOG_INFO(__FUNCTION__, "Connection. fd = [%d], status = [%d], %p",
                   connFd, connStat, csm);

    if (connStat != 0)
    {
        /* Conn Fail*/
        goto conn_fail;
    }
    else
    {
        /* Conn OK*/
        struct epoll_event event;
        int err;
        int send_buf_len = PEER_SOCK_SND_BUF_LEN;
        int recv_buf_len = PEER_SOCK_RCV_BUF_LEN;
        event.data.fd = connFd;
        event.events = EPOLLIN;
        err = epoll_ctl(sys->epoll_fd, EPOLL_CTL_ADD, connFd, &event);
        if (err)
            goto conn_fail;
        csm->sock_fd = connFd;
        if (setsockopt(csm->sock_fd, SOL_SOCKET, SO_SNDBUF, &send_buf_len, sizeof(send_buf_len)) == -1)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "Set socket send buf option failed. Error");
        }
        if (setsockopt(csm->sock_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_len, sizeof(recv_buf_len)) == -1)
        {
            ICCPD_LOG_ERR(__FUNCTION__, "Set socket recv buf option failed. Error");
        }

        FD_SET(connFd, &(sys->readfd));
        sys->readfd_count++;
        ICCPD_LOG_INFO(__FUNCTION__, "Connect to server %s sucess .", csm->peer_ip);
        goto conn_ok;
    }

 conn_fail:
    if (connFd >= 0)
    {
        csm->sock_fd = -1;
        close(connFd);
    }
 conn_ok:
    time(&csm->connTimePrev);
    session_conn_thread_unlock(&csm->conn_mutex);
    return;
}

/* Create socket connect to peer */
int scheduler_prepare_session(struct CSM* csm)
{
    int ret = MCLAG_ERROR;
    uint32_t local_ip = 0;
    uint32_t peer_ip = 0;

    /* Init time_t*/
    if (csm->connTimePrev == 0)
    {
        time(&csm->connTimePrev);
    }

    /* Don't conn to svr continously*/
    if ((time(NULL) - csm->connTimePrev) < CONNECT_INTERVAL_SEC)
    {
        goto no_time_update;
    }

    /* Already conn?*/
    if (csm->sock_fd > 0)
    {
        goto time_update;
    }

    if ((ret = scheduler_check_csm_config(csm)) < 0)
        goto time_update;

    /* Who is client*/
    local_ip = inet_addr(csm->sender_ip);
    peer_ip = inet_addr(csm->peer_ip);
    if (local_ip > peer_ip)
    {
        goto time_update;
    }
    else if (local_ip == peer_ip)
    {
        ICCPD_LOG_WARN(__FUNCTION__, "Local IP must not be the same as the peer IP.");
        goto time_update;
    }

    if (session_conn_thread_trylock(&csm->conn_mutex) == 0)
    {
        session_client_conn_handler(csm);
        session_conn_thread_unlock(&csm->conn_mutex);
    }

 time_update:
    time(&csm->connTimePrev);
    return 0;

 no_time_update:
    return 0;
}

/* Server socket initialization */
void scheduler_server_sock_init()
{
    int optval = 1;
    struct System* sys = NULL;
    struct sockaddr_in src_addr;

    if ((sys = system_get_instance()) == NULL)
        return;

    sys->server_fd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&(src_addr), sizeof(src_addr));
    src_addr.sin_family = PF_INET;
    src_addr.sin_port = htons(ICCP_TCP_PORT);
    src_addr.sin_addr.s_addr = INADDR_ANY;

    if (sys->server_fd == -1)
    {
        ICCPD_LOG_ERR(__FUNCTION__, "Server Socket FD creation failed.");
        return;
    }

    if (setsockopt(sys->server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        ICCPD_LOG_INFO(__FUNCTION__, "Set socket option failed. Error");
        /*return;*/
    }

    if (bind(sys->server_fd, (struct sockaddr*)&(src_addr), sizeof(src_addr)) < 0)
    {
        ICCPD_LOG_INFO(__FUNCTION__, "Bind socket failed. Error");
        return;
    }

    if (listen(sys->server_fd, MAX_ACCEPT_CONNETIONS) == -1)
    {
        ICCPD_LOG_INFO(__FUNCTION__, "Listen failed. Error");
        return;
    }

    ICCPD_LOG_INFO(__FUNCTION__, "Server socket init done.");

    return;
}

int iccp_get_server_sock_fd()
{
    struct System* sys = NULL;

    if ((sys = system_get_instance()) == NULL)
        return 0;

    return sys->server_fd;
}

/* Server socket initialization */
int scheduler_check_csm_config(struct CSM* csm)
{
    int ret = 1;
    struct LocalInterface* lif = NULL;
    struct System* sys = NULL;

    if ((sys = system_get_instance()) == NULL)
        return MCLAG_ERROR;

    if (csm == NULL )
        return MCLAG_ERROR;

    if (csm->mlag_id <= 0)
        ret = MCLAG_ERROR;
    else if (strlen(csm->peer_ip) <= 0)
        ret = MCLAG_ERROR;
    else if (strlen(csm->sender_ip) <= 0)
        ret = MCLAG_ERROR;
    else if (strlen(csm->peer_itf_name) != 0)
    {
        lif = local_if_find_by_name(csm->peer_itf_name);
        if (lif == NULL)
        {
            /*if peer-link is configured but the interface is not created, peer connection can not establish*/
            return MCLAG_ERROR;
        }
        else
        {
            lif->is_peer_link = 1;
            csm->peer_link_if = lif;
            set_peerlink_learn_kernel(csm, 0, 2);
        }
    }

    if (ret == MCLAG_ERROR)
        ICCPD_LOG_INFO(__FUNCTION__, "mclag config is not complete or conflicting, please check!");

    /* Decide STP role*/
    iccp_csm_stp_role_count(csm);

    return ret;
}

int scheduler_unregister_sock_read_event_callback(struct CSM* csm)
{
    struct System* sys = NULL;

    if ((sys = system_get_instance()) == NULL )
        return MCLAG_ERROR;

    if (csm == NULL )
    {
        return MCLAG_ERROR;
    }

    if (csm->sock_fd > 0)
    {
        FD_CLR(csm->sock_fd, &(sys->readfd));
    }

    return 0;
}

void scheduler_session_disconnect_handler(struct CSM* csm)
{
    struct System* sys = NULL;
    struct If_info * cif = NULL;

    if ((sys = system_get_instance()) == NULL )
        return;

    if (csm == NULL)
        return;

    ICCPD_LOG_NOTICE("ICCP_FSM", "scheduler session disconnect handler");

    session_conn_thread_lock(&csm->conn_mutex);
    scheduler_unregister_sock_read_event_callback(csm);
    if (csm->sock_fd > 0)
    {
        scheduler_csm_socket_cleanup(csm, 1);
    }

    mlacp_peer_disconn_handler(csm);
    MLACP(csm).current_state = MLACP_STATE_INIT;
    iccp_csm_status_reset(csm, 0);

    time(&csm->connTimePrev);
    session_conn_thread_unlock(&csm->conn_mutex);

    return;
}

void scheduler_csm_socket_cleanup(struct CSM* csm, int location)
{
    struct System* sys;
    struct epoll_event event;

    sys = system_get_instance();
    if (sys == NULL)
        return;

    if (csm == NULL)
        return;

    if (csm->sock_fd <= 0)
        return;

    event.data.fd = csm->sock_fd;
    event.events = EPOLLIN;
    if (epoll_ctl(sys->epoll_fd, EPOLL_CTL_DEL, csm->sock_fd, &event) != 0)
    {
        ICCPD_LOG_ERR("ICCP_FSM", "CSM socket %d epoll del error %d, location %d",
                      csm->sock_fd, errno, location);
    }
    if (close(csm->sock_fd) != 0)
    {
        ICCPD_LOG_ERR("ICCP_FSM", "CSM socket %d close error %d, location %d",
                      csm->sock_fd, errno, location);
        SYSTEM_INCR_SOCKET_CLOSE_ERR_COUNTER(system_get_instance());
    }
    else
    {
        ICCPD_LOG_NOTICE("ICCP_FSM", "CSM socket %d close, location %d",
                         csm->sock_fd, location);
    }
    csm->sock_fd = -1;
}

