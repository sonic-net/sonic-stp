
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*
 * filename: net_publish.h
 */


#ifndef NET_PUBLISH_EVENT_H
#define NET_PUBLISH_EVENT_H

#include "cps_api_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
Send an event using the event dispatchers socket API - decoupling the thread of netlink from event service
*/
void net_publish_event(cps_api_object_t obj);

#ifdef __cplusplus
}
#endif

#endif

