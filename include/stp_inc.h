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

#ifndef __STP_INC_H__
#define __STP_INC_H__


#include "applog.h"
#include "avl.h"
#include "bitmap.h"
#include "stp_netlink.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <signal.h>
#include <search.h>
#include <pthread.h>
#include <sys/socket.h>
#include <event2/event.h>
#include <sys/un.h>
#include <assert.h>
#include <linux/filter.h>
#include <linux/if_packet.h>
#include "l2.h"
#include "stp_timer.h"
#include "stp_intf.h"
#include "stp_common.h"
#include "stp_ipc.h"
#include "stp.h"
#include "stp_main.h"
#include "stp_externs.h"
#include "stp_dbsync.h"

#include "mstp_common.h"
#include "mstp.h"

#endif //__STP_INC_H__
