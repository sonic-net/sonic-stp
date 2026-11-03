/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * -Wl,--wrap stubs for the two things the daemon does to the kernel.
 * Neither is on the config path under test.
 */

#include "stp_inc.h"

/* Opens a PF_PACKET tap per port: needs CAP_NET_RAW and a real ifindex,
 * and sys_assert()s on failure. */
int __wrap_stp_pkt_sock_create(INTERFACE_NODE *intf_node)
{
    intf_node->sock = -1;
    intf_node->ev = NULL;
    return 0;
}

/* The daemon drives linux bridge VLAN membership by shelling out to
 * /sbin/bridge. There is no bridge here, and the CIST teardown would fork
 * a shell per (vlan, port). The commands are kept, newest last, so a test
 * can check what the kernel would have been told. */
#define STPTEST_KERNEL_CMD_SLOTS 256
#define STPTEST_KERNEL_CMD_LEN 128

static char g_kernel_cmds[STPTEST_KERNEL_CMD_SLOTS][STPTEST_KERNEL_CMD_LEN];
static unsigned int g_kernel_cmd_count;

int __wrap_system(const char *command)
{
    unsigned int slot = g_kernel_cmd_count % STPTEST_KERNEL_CMD_SLOTS;

    snprintf(g_kernel_cmds[slot], STPTEST_KERNEL_CMD_LEN, "%s", command ? command : "");
    g_kernel_cmd_count++;
    return 0;
}

void stptest_reset_kernel_commands(void)
{
    g_kernel_cmd_count = 0;
}

const char *stptest_last_kernel_command_containing(const char *needle)
{
    unsigned int n = g_kernel_cmd_count < STPTEST_KERNEL_CMD_SLOTS ? g_kernel_cmd_count
                                                                   : STPTEST_KERNEL_CMD_SLOTS;
    unsigned int i;

    for (i = 0; i < n; i++)
    {
        unsigned int slot = (g_kernel_cmd_count - 1 - i) % STPTEST_KERNEL_CMD_SLOTS;
        if (strstr(g_kernel_cmds[slot], needle))
            return g_kernel_cmds[slot];
    }
    return "";
}
