#!/usr/bin/python
#
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN #AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
#  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#

"""
This File Provides ifindex realted utilities for converting interface names
(Physical and Logical) into os ifindex and os ifindex into inteface names
"""

import ctypes
import ctypes.util

libc = ctypes.CDLL(ctypes.util.find_library('c'))


def if_nametoindex(name):
    """
    Converts interface name to ifindex
    @name - inteface name
    @return - ifindex in case of success, else raises exception
    """
    ret = libc.if_nametoindex(name)
    if not ret:
        raise RuntimeError("Invalid Name")
    return ret


def if_indextoname(index):
    """
    Converts Physical interface index to name
    @index - inteface index
    @return - interface name if successful, else raises exception
    """
    libc.if_indextoname.argtypes = [ctypes.c_uint32, ctypes.c_char_p]
    libc.if_indextoname.restype = ctypes.c_char_p

    ifname = ctypes.create_string_buffer(32)
    ifname = libc.if_indextoname(index, ifname)
    if not ifname:
        raise RuntimeError("Inavlid Index")
    return ifname
