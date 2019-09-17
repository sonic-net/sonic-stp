/*
 * Copyright 2019 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>
#include "stp_ipc.h"
#include <stdint.h>
#include <sys/un.h>
#include <stddef.h>
#include <errno.h>

#define STP_CLIENT_SOCK "/var/run/client.sock"
#define stpout(fmt, ...)  fprintf(stdout, fmt, ##__VA_ARGS__)

int stpd_fd;

typedef struct CMD_LIST {
    char    cmd_name[32];
    int     cmd_type;
}CMD_LIST;


