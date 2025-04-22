/*
 *  Copyright 2025 Broadcom. All rights reserved.
 *  The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *   */

#ifndef __STP_DEBUG_H__
#define __STP_DEBUG_H__

extern FILE *dbgfp;

#define STP_DUMP(fmt, ...)  do {fprintf(dbgfp, fmt, ##__VA_ARGS__); fflush(dbgfp);}while(0)

#define L2_STATE_STRING(s, p)       l2_port_state_to_string(s, p)

extern void stpdbg_dump_nl_db();
extern void stpdbg_dump_stp_stats();
extern void stpdbg_dump_nl_db_intf(char *name);

#endif //__STP_DEBUG_H__
