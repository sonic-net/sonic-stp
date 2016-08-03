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

import cps
import subprocess
import sys
import time
import cps_object
import cps_utils
import socket
import binascii
import ifindex_utils

import dn_base_ip_tool

iplink_cmd = '/sbin/ip'

_keys = {
    'base-ip/ipv4': cps.key_from_name('target', 'base-ip/ipv4'),
    'base-ip/ipv6': cps.key_from_name('target', 'base-ip/ipv6'),
    'base-ip/ipv4/address':
    cps.key_from_name('target', 'base-ip/ipv4/address'),
    'base-ip/ipv6/address':
    cps.key_from_name('target', 'base-ip/ipv6/address'),
    cps.key_from_name('target', 'base-ip/ipv4'): 'base-ip/ipv4',
    cps.key_from_name('target', 'base-ip/ipv6'): 'base-ip/ipv6',
    cps.key_from_name('target', 'base-ip/ipv4/address'): 'base-ip/ipv4/address',
    cps.key_from_name('target', 'base-ip/ipv6/address'): 'base-ip/ipv6/address',
}


def get_next_index(d):
    count = 0
    while True:
        if str(count) not in d:
            return count
        count += 1
    return count


def _get_af_from_name(name):
    type = 'ipv4'
    if name.find(type) == -1:
        type = 'ipv6'
    return type


def _get_obj_name(obj):
    return _keys[obj.get_key()]


def _get_af_from_obj(obj):
    return _get_af_from_name(_get_obj_name(obj))


def _get_proc_fwd_entry(dev, iptype):
    return ['proc', 'sys', 'net', iptype, 'conf', dev, 'forwarding']


def _get_proc_variable(path):
    try:
        path = '/'.join(path)
        with open('/' + path, 'r') as f:
            data = f.read()
        return int(data)
    except:
        print "Error reading ", path
        return -1


def _set_proc_variable(path, value):
    try:
        path = '/'.join(path)
        with open('/' + path, 'w') as f:
            data = f.write(str(value))
        return int(data)
    except:
        print "Error writing ", path


def create_obj_from_line(obj_type, ifix, ifname):

    af = _get_af_from_name(obj_type)

    o = cps_object.CPSObject(obj_type, data={'base-ip/' + af + '/vrf-id': 0,
                                             'base-ip/' + af + '/ifindex': ifix,
                                             'base-ip/' + af + '/name': ifname,
                                             })
    return o


def _get_key_from_obj(obj):
    af = _get_af_from_obj(obj)

    str_index = 'base-ip/' + af + '/ifindex'
    str_name = 'base-ip/' + af + '/name'

    name = None

    try:
        index = obj.get_attr_data(str_index)
        name = ifindex_utils.if_indextoname(index)
    except:
        pass
    if name is None:
        try:
            name = obj.get_attr_data(str_name)
        except:
            pass
    return name


def _ip_line_type_valid(af, ip):

    if af == 'ipv4' and ip[0] == 'inet':
        return True
    if af == 'ipv6' and ip[0] == 'inet6':
        return True
    return False


def process_ip_line(af, d, ip):
    search_str = None

    _srch = {'ipv4': 'inet', 'ipv6': 'inet6'}
    _af = {'ipv4': socket.AF_INET, 'ipv6': socket.AF_INET6}

    if af not in _srch:
        return

    if ip[0] == _srch[af]:
        try:
            addr = ip[1]
            prefix = ip[2]

            addr = binascii.hexlify(socket.inet_pton(_af[af], addr))
            prefix = int(prefix)

            d['base-ip/' + af + '/address/ip'] = cps_object.types.to_data(
                'base-ip/' + af + '/address/ip', addr)
            d['base-ip/' + af + '/address/prefix-length'] = cps_object.types.to_data(
                'base-ip/' + af + '/address/prefix-length', prefix)

        except:
            print "Unable to convert address ", header
            pass


def add_ip_info(af, o, ip):
    if af is None:
        return

    if 'base-ip/' + af + '/address' not in o.get()['data']:
        o.get()['data']['base-ip/' + af + '/address'] = {}

    _v = o.get()['data']['base-ip/' + af + '/address']

    d = {}
    next_index = get_next_index(_v)
    process_ip_line(af, d, ip)
    if (len(d)) > 0:
        _v[str(next_index)] = d


def _get_ip_objs(filt, resp):
    af = _get_af_from_obj(filt)
    name = _get_key_from_obj(filt)

    lst = dn_base_ip_tool.get_if_details(name)

    for _if in lst:
        o = create_obj_from_line('base-ip/' + af, _if.ifix, _if.ifname)

        if not filt.key_compare(
            {'base-ip/' + af + '/name': o.get_attr_data('base-ip/' + af + '/name'),
                               'base-ip/' + af + '/ifindex': o.get_attr_data('base-ip/' + af + '/ifindex')}):
            continue

        fwd = _get_proc_variable(
            _get_proc_fwd_entry(o.get_attr_data('base-ip/' + af + '/name'), af))
        if fwd == -1:
            fwd = 0
        o.add_attr('base-ip/' + af + '/enabled', 1)
        o.add_attr('base-ip/' + af + '/forwarding', fwd)

        for _ip in _if.ip:
            add_ip_info(af, o, _ip)
        resp.append(o.get())

    return True


def _get_ip_addr_obj(filt, resp):
    af = _get_af_from_obj(filt)
    name = _get_key_from_obj(filt)

    lst = dn_base_ip_tool.get_if_details(name)

    for _if in lst:

        for ip in _if.ip:
            if not _ip_line_type_valid(af, ip):
                continue

            o = create_obj_from_line(
                'base-ip/' + af + '/address',
                _if.ifix,
                _if.ifname)
            if not filt.key_compare(
                {'base-ip/' + af + '/name': o.get_attr_data('base-ip/' + af + '/name'),
                                   'base-ip/' + af + '/ifindex': o.get_attr_data('base-ip/' + af + '/ifindex')}):
                continue

            process_ip_line(af, o.get()['data'], ip)
            resp.append(o.get())

    return True


def get_cb(methods, params):
    obj = cps_object.CPSObject(obj=params['filter'])
    resp = params['list']

    if obj.get_key() == _keys['base-ip/ipv4'] or obj.get_key() == _keys['base-ip/ipv6']:
        return _get_ip_objs(obj, resp)

    if obj.get_key() == _keys['base-ip/ipv4/address'] or obj.get_key() == _keys['base-ip/ipv6/address']:
        return _get_ip_addr_obj(obj, resp)

    return False


def _create_ip_and_prefix_from_obj(obj, iptype):
    addr = obj.get_attr_data('base-ip/' + iptype + '/address/ip')
    prefix = obj.get_attr_data('base-ip/' + iptype + '/address/prefix-length')
    addr = binascii.unhexlify(addr)
    af = socket.AF_INET
    if iptype == 'ipv6':
        af = socket.AF_INET6
    addr = socket.inet_ntop(af, addr)
    addr = addr + '/' + str(prefix)
    return addr


def trans_cb(methods, params):
    obj = cps_object.CPSObject(obj=params['change'])
    af = _get_af_from_obj(obj)

    name = _get_key_from_obj(obj)
    if name is None:
        print "Missing keys for request ", obj
        return False

    addr = ""

    try:
        if params['operation'] == 'set' and obj.get_key() == _keys['base-ip/' + af]:
            fwd = obj.get_attr_data('base-ip/' + af + '/forwarding')
            _set_proc_variable(_get_proc_fwd_entry(name, af), str(fwd))

        if params['operation'] == 'create' and obj.get_key() == _keys['base-ip/' + af + '/address']:
            addr = _create_ip_and_prefix_from_obj(obj, af)

            print "Attempting to add address ", addr, name
            if dn_base_ip_tool.add_ip_addr(addr, name):
                return True
            print "Failed to execute request..."

        if params['operation'] == 'delete' and obj.get_key() == _keys['base-ip/' + af + '/address']:
            addr = _create_ip_and_prefix_from_obj(obj, af)
            print "Attempting to delete address ", addr, name
            if dn_base_ip_tool.del_ip_addr(addr, name):
                return True
            print "Failed to execute request..."
    except Exception as e:
        print "Faild to commit operation.", e
        print params

    return False

if __name__ == '__main__':
    if len(sys.argv) > 1:
        l = []
        _get_ip_objs(cps_object.CPSObject('base-ip/ipv4'), l)
        for i in l:
            cps_utils.print_obj(i)
        sys.exit(1)

    handle = cps.obj_init()
    d = {}
    d['get'] = get_cb
    d['transaction'] = trans_cb

    for i in _keys.keys():
        if i.find('base-ip') == -1:
            continue
        cps.obj_register(handle, _keys[i], d)

    while True:
        time.sleep(1)
