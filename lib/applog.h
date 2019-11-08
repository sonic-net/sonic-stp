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

#ifndef _APPLOG_H_
#define _APPLOG_H_

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>

int applog_init();
int applog_deinit();
int applog_set_config_level(int level);
int applog_get_config_level();
int applog_get_init_status();
int applog_write(int priority, const char *fmt, ...);


#define APP_LOG_LEVEL_NONE            (-1)
#define APP_LOG_LEVEL_EMERG           (0) /* system is unusable */
#define APP_LOG_LEVEL_ALERT           (1) /* action must be taken immediately */
#define APP_LOG_LEVEL_CRIT            (2) /* critical conditions */
#define APP_LOG_LEVEL_ERR             (3) /* error conditions */
#define APP_LOG_LEVEL_WARNING         (4) /* warning conditions */
#define APP_LOG_LEVEL_NOTICE          (5) /* normal but significant condition */
#define APP_LOG_LEVEL_INFO            (6) /* informational */
#define APP_LOG_LEVEL_DEBUG           (7) /* debug-level messages */

#define APP_LOG_LEVEL_MIN             (APP_LOG_LEVEL_EMERG)
#define APP_LOG_LEVEL_DEFAULT         (APP_LOG_LEVEL_ERR)
#define APP_LOG_LEVEL_MAX             (APP_LOG_LEVEL_DEBUG)

#define APP_LOG_STATUS_OK             (0)
#define APP_LOG_STATUS_FAIL           (-1)
#define APP_LOG_STATUS_INVALID_LEVEL  (-2)
#define APP_LOG_STATUS_LEVEL_DISABLED (-3)

#define APP_LOG_INIT                  applog_init
#define APP_LOG_DEINIT                applog_deinit
#define APP_LOG_SET_LEVEL(level)      applog_set_config_level(level)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"

#ifdef APP_LOG_NO_FUNC_LINE 
#define APP_LOG_DEBUG(...)            applog_write(APP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define APP_LOG_INFO(...)             applog_write(APP_LOG_LEVEL_INFO, __VA_ARGS__)
#define APP_LOG_ERR(...)              applog_write(APP_LOG_LEVEL_ERR, __VA_ARGS__)
#define APP_LOG_WARNING(...)          applog_write(APP_LOG_LEVEL_WARNING, __VA_ARGS__)
#define APP_LOG_EMERG(...)            applog_write(APP_LOG_LEVEL_EMERG, __VA_ARGS__)
#define APP_LOG_CRITICAL(...)         applog_write(APP_LOG_LEVEL_CRIT, __VA_ARGS__)
#define APP_LOG_NOTICE(...)           applog_write(APP_LOG_LEVEL_NOTICE, __VA_ARGS__)
#define APP_LOG_ALERT(...)            applog_write(APP_LOG_LEVEL_ALERT, __VA_ARGS__)
#else
#define APP_LOG_DEBUG(MSG, ...)       applog_write(APP_LOG_LEVEL_DEBUG,   "%s:%u:"MSG" ", __func__, __LINE__,  ##__VA_ARGS__) 
#define APP_LOG_INFO(MSG, ...)        applog_write(APP_LOG_LEVEL_INFO,    "%s:%u:"MSG" ", __func__, __LINE__,  ##__VA_ARGS__)
#define APP_LOG_ERR(MSG, ...)         applog_write(APP_LOG_LEVEL_ERR,     "%s:%u:"MSG" ", __func__, __LINE__,  ##__VA_ARGS__)
#define APP_LOG_WARNING(MSG, ...)     applog_write(APP_LOG_LEVEL_WARNING, "%s:%u:"MSG" ", __func__, __LINE__,  ##__VA_ARGS__)
#define APP_LOG_EMERG(MSG, ...)       applog_write(APP_LOG_LEVEL_EMERG,   "%s:%u:"MSG" ", __func__, __LINE__,  ##__VA_ARGS__)
#define APP_LOG_CRITICAL(MSG, ...)    applog_write(APP_LOG_LEVEL_CRIT,    "%s:%u:"MSG" ", __func__, __LINE__,  ##__VA_ARGS__)
#define APP_LOG_NOTICE(MSG, ...)      applog_write(APP_LOG_LEVEL_NOTICE,  "%s:%u:"MSG" ", __func__, __LINE__,  ##__VA_ARGS__)
#define APP_LOG_ALERT(MSG, ...)       applog_write(APP_LOG_LEVEL_ALERT,   "%s:%u:"MSG" ", __func__, __LINE__,  ##__VA_ARGS__)
#endif

/* Use below one for Syslog */
#define APP_SYS_LOG_INFO(...)         applog_write(APP_LOG_LEVEL_INFO, __VA_ARGS__)

#pragma GCC diagnostic pop

#endif /*_APPLOG_H_*/

