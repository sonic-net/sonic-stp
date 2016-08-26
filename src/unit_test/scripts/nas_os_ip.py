#!/usr/bin/python
#
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#

import cps
import cps_object
import cps_utils

_ipv4_key = cps.key_from_name('target', 'base-ip/ipv4')
_ipv6_key = cps.key_from_name('target', 'base-ip/ipv6')


if __name__ == '__main__':
    # monitor ip change event
    handle = cps.event_connect()
    cps.event_register(handle, _ipv4_key)
    cps.event_register(handle, _ipv6_key)
    while True:
        o = cps.event_wait(handle)
        print o
        obj = cps_object.CPSObject(obj=o)
        cps_utils.print_obj(obj)
        if _ipv4_key == obj.get_key():
            print "IPv4 change event received"
        elif _ipv6_key == obj.get_key():
            print "IPv6 change event received"
