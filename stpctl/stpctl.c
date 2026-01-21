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
#include "stpctl.h"

CMD_LIST g_cmd_list[] = {
    "help",     STP_CTL_HELP,
    "all",      STP_CTL_DUMP_ALL,
    "global",   STP_CTL_DUMP_GLOBAL,
    "vlan",     STP_CTL_DUMP_VLAN,
    "port",     STP_CTL_DUMP_INTF,
    "dbglvl",   STP_CTL_SET_LOG_LVL,
    "nldb",     STP_CTL_DUMP_NL_DB,
    "nlintf",   STP_CTL_DUMP_NL_DB_INTF,
    "lstats",   STP_CTL_DUMP_LIBEV_STATS,
    "dbg",      STP_CTL_SET_DBG,
    "clrstsall",      STP_CTL_CLEAR_ALL,
    "clrstsvlan",     STP_CTL_CLEAR_VLAN,
    "clrstsintf",     STP_CTL_CLEAR_INTF,
    "clrstsvlanintf", STP_CTL_CLEAR_VLAN_INTF,
    "mst",      STP_CTL_DUMP_MST,
    "mstport",  STP_CTL_DUMP_MST_PORT,
};

#define CMD_LIST_COUNT (sizeof(g_cmd_list) / sizeof(g_cmd_list[0]))

void print_cmds()
{
    size_t i;

    for (i = 0; i < CMD_LIST_COUNT; i++)
        stpout("stpctl %s\n",g_cmd_list[i].cmd_name);
}


int get_cmd_type(char *name)
{
    size_t i;

    for (i = 0; i < CMD_LIST_COUNT; i++)
    {
        if (strcmp(g_cmd_list[i].cmd_name, name) == 0)
            return g_cmd_list[i].cmd_type;
    }

    return -1;
}

int connect_server()
{
    int ret;
    struct sockaddr_un addr;

    unlink(STP_CLIENT_SOCK);
    // create socket
    stpd_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (!stpd_fd) {
        stpout("stpmgr socket error %s\n", strerror(errno));
        return -1;
    }

    // setup socket address structure
    bzero(&addr, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, STP_CLIENT_SOCK, sizeof(addr.sun_path)-1);

    ret = bind(stpd_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (ret == -1)
    {
        stpout("ipc bind error %s", strerror(errno));
        close(stpd_fd);
        return -1;
    }
}

int send_msg_stpd(STP_MSG_TYPE msgType, uint32_t msgLen, void *data)
{
    STP_IPC_MSG *tx_msg;
    size_t len = 0;
    struct sockaddr_un addr;
    int rc;

    len = msgLen + (offsetof(struct STP_IPC_MSG, data));

    tx_msg = (STP_IPC_MSG *)calloc(1, len);
    if (tx_msg == NULL)
    {
        stpout("tx_msg mem alloc error\n");
        return -1;
    }

    tx_msg->msg_type = msgType;
    tx_msg->msg_len  = msgLen;
    memcpy(tx_msg->data, data, msgLen);

    bzero(&addr, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, STPD_SOCK_NAME, sizeof(addr.sun_path)-1);

    rc = sendto(stpd_fd, tx_msg, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (rc == -1)
    {
        stpout("stpmgr tx_msg send error\n");
    }
    free(tx_msg);
    return rc;
}

int send_command(int argc, char **argv)
{
    char *end_ptr = 0;
    char buffer[64];
    int ret;
    int len;
    struct sockaddr client_sock;
    STP_CTL_MSG msg;
    int cmd_type;

    cmd_type = get_cmd_type(argv[1]);

    msg.cmd_type = cmd_type;
    switch (cmd_type)
    {
        case STP_CTL_DUMP_ALL: 
        case STP_CTL_DUMP_GLOBAL: 
        {
            if (!(argc == 2))
            {
                stpout("invalid number of args\n");
                return -1;
            }
            break;
        }

        case STP_CTL_DUMP_VLAN: 
        {
            if (!(argc == 3))
            {
                stpout("invalid number of args\n");
                return -1;
            }
            msg.vlan_id = atoi(argv[2]);
            break;
        }

        case STP_CTL_DUMP_INTF: 
        {
            if (!(argc == 4))
            {
                stpout("invalid number of args\n");
                return -1;
            }
            msg.vlan_id = atoi(argv[2]);
            strncpy(msg.intf_name, argv[3], IFNAMSIZ);
            break;
        }

        case STP_CTL_SET_LOG_LVL:
        {
            if (!(argc == 3))
            {
                stpout("invalid number of args\n");
                return -1;
            }

            msg.level = atoi(argv[2]);
            break;
        }

        case STP_CTL_DUMP_NL_DB:
        {
            /* No arg */
            if (!(argc == 2))
            {
                stpout("invalid number of args\n");
                return -1;
            }
            break;
        }

        case STP_CTL_DUMP_NL_DB_INTF: 
        {
            if (!(argc == 3))
            {
                stpout("invalid number of args\n");
                return -1;
            }

            strncpy(msg.intf_name, argv[2], IFNAMSIZ);
            break;
        }

        case STP_CTL_SET_DBG:
        {
            /*
             * stpctl dbg enable/disable
             * stpctl dbg port <id> on/off   //id = max_stp_port for all
             * stpctl dbg vlan <id> on/off   //id = 4095 for all
             * stpctl dbg verbose on/off
             * stpctl dbg bpdu on/off {rx/tx}
             * stpctl dbg event on/off
             */
            if ((argc < 3) || (argc > 5))
            {
                stpout("invalid number of args\n");
                return -1;
            }

            if (0 == strncmp("help", argv[2], strlen("help")))
            {
                stpout("stpctl dbg enable/disable\n");
                stpout("stpctl dbg verbose on/off\n");
                stpout("stpctl dbg bpdu on/off {rx/tx}\n");
                stpout("stpctl dbg event on/off\n");
                stpout("stpctl dbg port <id> on/off\n");
                stpout("stpctl dbg vlan <id> on/off\n");
                close(stpd_fd);
                exit(0);
            }
            msg.dbg.flags = 0;
            if (0 == strncmp("enable", argv[2], strlen("enable")))
            {
                msg.dbg.flags   |= STPCTL_DBG_SET_ENABLED;
                msg.dbg.enabled = 1;
            }
            else if (0 == strncmp("disable", argv[2], strlen("disable")))
            {
                msg.dbg.flags   |= STPCTL_DBG_SET_ENABLED;
                msg.dbg.enabled = 0;
            }
            else if (0 == strncmp("verbose", argv[2], strlen("verbose")))
            {
                msg.dbg.flags   |= STPCTL_DBG_SET_VERBOSE;
                if (0 == strncmp("on", argv[3], 2))
                    msg.dbg.verbose = 1;
                else if (0 == strncmp("off", argv[3], 3))
                    msg.dbg.verbose = 0;
            }
            else if (0 == strncmp("bpdu", argv[2], strlen("bpdu")))
            {
                if (0 == strncmp("on", argv[3], 2))
                {
                    msg.dbg.flags   |= STPCTL_DBG_SET_BPDU_RX | STPCTL_DBG_SET_BPDU_TX;
                    msg.dbg.bpdu_rx = 1;
                    msg.dbg.bpdu_tx = 1;
                }
                else if (0 == strncmp("off", argv[3], 3))
                {
                    msg.dbg.flags   |= STPCTL_DBG_SET_BPDU_RX | STPCTL_DBG_SET_BPDU_TX;
                    msg.dbg.bpdu_rx = 0;
                    msg.dbg.bpdu_tx = 0;
                }
                else if (0 == strncmp("rx-on", argv[3], 5))
                {
                    msg.dbg.flags   |= STPCTL_DBG_SET_BPDU_RX;
                    msg.dbg.bpdu_rx = 1;
                }
                else if (0 == strncmp("rx-off", argv[3], 6))
                {
                    msg.dbg.flags   |= STPCTL_DBG_SET_BPDU_RX;
                    msg.dbg.bpdu_rx = 0;
                }
                else if (0 == strncmp("tx-on", argv[3], 5))
                {
                    msg.dbg.flags   |= STPCTL_DBG_SET_BPDU_TX;
                    msg.dbg.bpdu_tx = 1;
                }
                else if (0 == strncmp("tx-off", argv[3], 6))
                {
                    msg.dbg.flags   |= STPCTL_DBG_SET_BPDU_TX;
                    msg.dbg.bpdu_tx = 0;
                }
            }
            else if (0 == strncmp("event", argv[2], strlen("event")))
            {
                msg.dbg.flags   |= STPCTL_DBG_SET_EVENT;
                if (0 == strncmp("on", argv[3], 2))
                    msg.dbg.event = 1;
                else if (0 == strncmp("off", argv[3], 3))
                    msg.dbg.event = 0;
            }
            else if (0 == strncmp("port", argv[2], strlen("port")))
            {
                msg.dbg.flags |= STPCTL_DBG_SET_PORT;
                if (0 == strncmp("all", argv[3], 3))
                    msg.intf_name[0] = '\0';
                else
                    strncpy(msg.intf_name, argv[3], IFNAMSIZ);
                if (0 == strncmp("on", argv[4], 2))
                    msg.dbg.port = 1;
                else if (0 == strncmp("off", argv[4], 3))
                    msg.dbg.port = 0;
            }
            else if (0 == strncmp("vlan", argv[2], strlen("vlan")))
            {
                msg.dbg.flags |= STPCTL_DBG_SET_VLAN;
                if (0 == strncmp("all", argv[3], 3))
                    msg.vlan_id = 0;
                else
                    msg.vlan_id = strtol(argv[3], &end_ptr, 10);
                if (0 == strncmp("on", argv[4], 2))
                    msg.dbg.vlan = 1;
                else if (0 == strncmp("off", argv[4], 3))
                    msg.dbg.vlan = 0;
            }
            else if (0 == strncmp("show", argv[2], strlen("show")))
            {
                msg.dbg.flags |= STPCTL_DBG_SHOW;
            }
            else
            {
                stpout("invalid argv[2] : %s\n", argv[2]);
                return -1;
            }

            break;
        }

        case STP_CTL_DUMP_LIBEV_STATS:
        {
            /* No arg */
            if (!(argc == 2))
            {
                stpout("invalid number of args\n");
                return -1;
            }
            break;
        }

        case STP_CTL_CLEAR_ALL:
        {
            /* No arg */
            if (!(argc == 2))
            {
                stpout("invalid number of args\n");
                return -1;
            }
            break;
        }

        case STP_CTL_CLEAR_VLAN:
        {
            if (!(argc == 3))
            {
                stpout("invalid number of args\n");
                return -1;
            }

            msg.vlan_id = atoi(argv[2]);
            break;
        }

        case STP_CTL_CLEAR_INTF:
        {
            if (!(argc == 3))
            {
                stpout("invalid number of args\n");
                return -1;
            }

            strncpy(msg.intf_name, argv[2], IFNAMSIZ);
            break;
        }

        case STP_CTL_CLEAR_VLAN_INTF:
        {
            if (!(argc == 4))
            {
                stpout("invalid number of args\n");
                return -1;
            }

            msg.vlan_id = atoi(argv[2]);
            strncpy(msg.intf_name, argv[3], IFNAMSIZ);
            break;
        }

        case STP_CTL_DUMP_MST: 
        {
            if (!(argc == 3))
            {
                stpout("invalid number of args\n");
                return -1;
            }
            msg.mst_id = atoi(argv[2]);
            break;
        }

        case STP_CTL_DUMP_MST_PORT: 
        {
            if (!(argc == 4))
            {
                stpout("invalid number of args\n");
                return -1;
            }
            msg.mst_id = atoi(argv[2]);
            strncpy(msg.intf_name, argv[3], IFNAMSIZ);
            break;
        }

        default:
        {
            stpout("invalid command %d\n", cmd_type);
            return -1;
        }
    }

    ret = send_msg_stpd(STP_STPCTL_MSG, sizeof(msg), (void*)&msg);
    if (ret == -1)
    {
        stpout("send error\n");
        return -1;
    }

    ret = recvfrom(stpd_fd, buffer, 64, 0, NULL, NULL);
    if (ret == -1)
    {
        stpout("recv  message error %s", strerror(errno));
        return -1;
    }

    return 0;
}

void display_resp()
{
    FILE *fp = fopen("/var/log/stp_dmp.log", "r");
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    if (!fp)
    {
        stpout("invalid file\n");
        return;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        stpout("%s", line);
    }

    fclose(fp);
    if (line)
        free(line);
}

int main(int argc, char **argv)
{
    int ch;

    if (argc < 2)
    {
        stpout("invalid number of args\n");
        return -1;
    }
    if (0 == strncmp("help", argv[1], strlen("help")))
    {
        print_cmds();
        close(stpd_fd);
        return 0;
    }

    if (connect_server() != -1) 
    {
        if (send_command(argc, argv) != -1)
            display_resp();
    }

    close(stpd_fd);
    return 0;
}
