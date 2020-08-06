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
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

#include "applog.h"

int applog_level_map[APP_LOG_LEVEL_MAX + 2];
int applog_config_level = APP_LOG_LEVEL_DEFAULT;
int applog_inited = 0;

int applog_init()
{
  memset(applog_level_map, 0, sizeof(applog_level_map));

  /* match user level to system level */
  applog_level_map[APP_LOG_LEVEL_EMERG] = LOG_EMERG;
  applog_level_map[APP_LOG_LEVEL_ALERT] = LOG_ALERT;
  applog_level_map[APP_LOG_LEVEL_CRIT] = LOG_CRIT;
  applog_level_map[APP_LOG_LEVEL_ERR] = LOG_ERR;
  applog_level_map[APP_LOG_LEVEL_WARNING] = LOG_WARNING;
  applog_level_map[APP_LOG_LEVEL_NOTICE] = LOG_NOTICE;
  applog_level_map[APP_LOG_LEVEL_INFO] = LOG_INFO;
  applog_level_map[APP_LOG_LEVEL_DEBUG] = LOG_DEBUG;

  openlog("stpd", LOG_NDELAY | LOG_CONS, LOG_DAEMON);

  applog_inited = 1;
  applog_config_level = APP_LOG_LEVEL_DEFAULT;

  return APP_LOG_STATUS_OK;
}

int applog_get_init_status()
{
  return applog_inited;
}

int applog_set_config_level(int level)
{
  if (level < APP_LOG_LEVEL_NONE || level > APP_LOG_LEVEL_MAX)
  {
    return APP_LOG_STATUS_INVALID_LEVEL;
  }
  else
  {
    applog_config_level = level;
  }

  return APP_LOG_STATUS_OK;
}

int applog_get_config_level()
{
  return applog_config_level;
}

int applog_write(int level, const char *fmt, ...)
{
  int priority;
  va_list ap;

  if (level < APP_LOG_LEVEL_MIN || level > APP_LOG_LEVEL_MAX)
  {
    return APP_LOG_STATUS_INVALID_LEVEL;
  }

  if (level > applog_config_level)
  {
    return APP_LOG_STATUS_LEVEL_DISABLED;
  }

  priority = applog_level_map[level];

  va_start(ap, fmt);
  vsyslog(priority, fmt, ap);
  va_end(ap);

  return APP_LOG_STATUS_OK;
}

int applog_deinit()
{
  int status = 0;

  closelog();

  applog_inited = 0;
  applog_config_level = APP_LOG_LEVEL_DEFAULT;

  return status;
}

