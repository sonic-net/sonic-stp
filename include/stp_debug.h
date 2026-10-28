/*
 *  Copyright 2025 Broadcom. All rights reserved.
 *  The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *   */

#ifndef __STP_DEBUG_H__
#define __STP_DEBUG_H__

extern FILE *dbgfp;

// dbgfp stays NULL when the dump file cannot be opened; a debug dump must not be
// able to kill the daemon, so every use of it is guarded.
#define STP_DUMP_START      do {dbgfp = fopen("/var/log/stp_dmp.log", "w+"); \
                                if (!dbgfp) \
                                    STP_LOG_ERR("/var/log/stp_dmp.log open failed: %s", strerror(errno)); \
                               }while(0)
#define STP_DUMP(fmt, ...)  do {if (dbgfp) {fprintf(dbgfp, fmt, ##__VA_ARGS__); fflush(dbgfp);}}while(0)
#define STP_DUMP_STOP       do {if (dbgfp) {fclose(dbgfp); dbgfp = NULL;}}while(0)

#define L2_STATE_STRING(s, p)       l2_port_state_to_string(s, p)

extern void stpdbg_dump_nl_db();
extern void stpdbg_dump_stp_stats();
extern void stpdbg_dump_nl_db_intf(char *name);

#endif //__STP_DEBUG_H__
