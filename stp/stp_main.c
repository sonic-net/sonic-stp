/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stp_main.h"

STPD_CONTEXT stpd_context;

int stpd_ipc_init()
{
    struct sockaddr_un sa;
    int ret;
    struct event *ipc_event = 0;

    unlink(STPD_SOCK_NAME);
    g_stpd_ipc_handle = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (!g_stpd_ipc_handle)
    {
        STP_LOG_ERR("ipc socket error %s", strerror(errno));
        return -1;
    }

    // setup socket address structure
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, STPD_SOCK_NAME, sizeof(sa.sun_path) - 1);

    ret = bind(g_stpd_ipc_handle, (struct sockaddr *)&sa, sizeof(struct sockaddr_un));
    if (ret == -1)
    {
        STP_LOG_ERR("ipc bind error %s", strerror(errno));
        close(g_stpd_ipc_handle);
        return -1;
    }

    ipc_event = stpmgr_libevent_create(g_stpd_evbase, g_stpd_ipc_handle,
            EV_READ|EV_PERSIST, stpmgr_recv_client_msg, (char *)"IPC", NULL);
    if (!ipc_event)
    {
        STP_LOG_ERR("ipc_event Create failed");
        return -1;
    }

    STP_LOG_DEBUG("ipc init done");

    return 0;
}

void stpd_log_init()
{
    STP_LOG_INIT();
    if (fopen("/stpd_dbg_reload", "r"))
    {
        STP_LOG_SET_LEVEL(STP_LOG_LEVEL_DEBUG);
    }
    else
        STP_LOG_SET_LEVEL(STP_LOG_LEVEL_INFO);
}

int stpd_main()
{
    int rc = 0;
    struct timeval stp_100ms_tv = {0, STPD_100MS_TIMEOUT};
    struct timeval msec_50 = { 0, 50*1000 };
    struct event   *evtimer_100ms = 0;
    struct event   *evpkt = 0;
    struct event_config *cfg = 0;
    int8_t ret = 0;

    signal(SIGPIPE, SIG_IGN);

    stpd_log_init();
    stpsync_clear_appdb_stp_tables();

    memset(&stpd_context, 0, sizeof(STPD_CONTEXT));

    stpmgr_set_extend_mode(true);

    cfg = event_config_new();
    if (!cfg)
    {
        STP_LOG_ERR("event_config_new Failed");
        return -1;
    }

    STP_LOG_INFO("LIBEVENT VER : 0x%x", event_get_version_number());
    event_config_set_max_dispatch_interval(cfg, &msec_50/*max_interval*/, 5/*max_callbacks*/, 1/*min-prio*/);
    g_stpd_evbase = event_base_new_with_config(cfg);
    if (g_stpd_evbase == NULL)
    {
        STP_LOG_ERR("eventbase create Failed ");
        return -1;
    }

    event_base_priority_init(g_stpd_evbase, STP_LIBEV_PRIO_QUEUES);

    //Create the high priority Timer libevent
    evtimer_100ms = stpmgr_libevent_create(g_stpd_evbase, -1, EV_PERSIST, 
            stptimer_100ms_tick, (char *)"100MS_TIMER", &stp_100ms_tv);
    if (!evtimer_100ms)
    {
        STP_LOG_ERR("evtimer_100ms Create failed");
        return -1;
    }

    /* Open Socket communication to STP Mgr */
    rc = stpd_ipc_init();
    if (rc < 0)
    {
        STP_LOG_ERR("ipc init failed");
        return -1;
    }

    /* Create STP interface DB */
    g_stpd_intf_db = avl_create(&stp_intf_avl_compare, NULL, NULL);
    if(!g_stpd_intf_db)
    {
        STP_LOG_ERR("intf db create failed");
        return -1;
    }

    /* Open Netlink comminucation to populate Interface DB */
    g_stpd_netlink_handle = stp_netlink_init(&stp_intf_netlink_cb);
    if(-1 == g_stpd_netlink_handle)
    {
        STP_LOG_ERR("netlink init failed");
        return -1;
    }

    /* Open Socket for Packet Tx
     * We need this for sending packet over Port-channels.
     * To simplify the design all STP tx will use this socket.
     * For RX we have per phy-port socket.
     *
     */
    if (-1 == (g_stpd_pkt_handle = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))) 
    {
        STP_LOG_ERR("Create g_stpd_pkt_tx_handle, errno : %s", strerror(errno));
        return -1;
    }

    STP_LOG_INFO("STP Daemon Running");

    event_base_dispatch(g_stpd_evbase);

    return 0;
}


