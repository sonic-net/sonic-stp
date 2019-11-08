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


