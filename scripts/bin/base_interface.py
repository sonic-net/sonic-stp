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
import ifindex_utils

import dn_base_ip_tool

from traceback import print_exc


iplink_cmd = '/sbin/ip'

_name_field = 'base-interface/entry/name'
_ifindex_field = 'base-interface/entry/ifindex'

_keys = {
    'base-interface/entry':
    cps.key_from_name('target', 'base-interface/entry'),
    cps.key_from_name('target', 'base-interface/entry'): 'base-interface/entry',
}


def _get_name_from_obj(obj):
    try:
        name = obj.get_attr_data(_name_field)
        if len(name) > 0:
            return name
    except:
        pass
    index = None
    try:
        index = obj.get_attr_data(_ifindex_field)
        return ifindex_utils.if_indextoname(index)
    except:
        pass
    return None


def handle_delete_op(obj):
    name = _get_name_from_obj(obj)
    dn_base_ip_tool.delete_if(name)


def handle_set_obj(obj):
    name = _get_name_from_obj(obj)

    try:
        mtu = obj.get_attr_data('base-interface/entry/mtu')
        if dn_base_ip_tool.set_if_mtu(name, mtu) == False:
            print "Failed to execute request...", res
            return False
    except:
        pass

    try:
        state = obj.get_attr_data('base-interface/entry/admin-status')
        if dn_base_ip_tool.set_if_state(name, state) == False:
            print('Failed to set the interface state.', name, state)
            return False
    except:
        pass

    try:
        mac = obj.get_attr_data('base-interface/entry/mac-address')
        if dn_base_ip_tool.set_if_mac(name, mac) == False:
            print('Failed to set the interface mac.', name, mac)
            return False
    except:
        pass

    return True


def handle_create_op(obj, params):

    # if no mac specified use the system MAC
    mac = None
    try:
        mac = obj.get_attr_data('base-interface/entry/mac-address')
    except:
        pass

    if mac is None:
        chassis_obj = cps_object.CPSObject('base-pas/chassis')
        l = []
        if cps.get([chassis_obj.get()], l) and len(l) > 0:
            chassis_obj = cps_object.CPSObject(obj=l[0])
            try:
                mac = chassis_obj.get_attr_data(
                    'base-pas/chassis/base_mac_addresses')
            except:
                pass
    else:
        # since we will set it via the set method.. don't bother setting it at
        # init time
        mac = None

    name = None
    try:
        name = obj.get_attr_data('base-interface/entry/name')
    except:
        pass

    if name is None:
        print("Failed to create interface without name")
        return False

    lst = dn_base_ip_tool.get_if_details(name)

    if len(lst) > 0:
        print("Failed to create an existing interface ", name)
        return False

    rc = dn_base_ip_tool.create_loopback_if(name, mac=mac)

    if rc:
        rc = handle_set_obj(obj)
        if_index = ifindex_utils.if_nametoindex(name)
        obj.add_attr('base-interface/entry/ifindex', if_index)
        params['change'] = obj.get()

    return rc


def get_cb(methods, params):
    obj = cps_object.CPSObject(obj=params['filter'])
    resp = params['list']
    try:
        name = _get_name_from_obj(obj)

        lst = dn_base_ip_tool.get_if_details(name)

        for _if in lst:
            o = cps_object.CPSObject(
                'base-interface/entry',
                data={'name': _if.ifname,
                      'ifindex': _if.ifix})

            if not obj.key_compare({_name_field: _if.ifname,
                                    _ifindex_field: _if.ifix}):
                continue

            o.add_attr('mtu', _if.mtu)
            o.add_attr('admin-status', _if.state)
            o.add_attr('oper-status', _if.oper_state)
            if _if.mac is not None:
                o.add_attr('mac-address', _if.mac)
            resp.append(o.get())

        return True
    except Exception as ex:
        print ex
        print ex.print_exc()
    return False


def trans_cb(methods, params):
    try:
        obj = cps_object.CPSObject(obj=params['change'])

        name = _get_name_from_obj(obj)
        if name is None:
            print "Missing keys for request ", obj
            return False

        if params['operation'] == 'create' and obj.get_key() == _keys['base-interface/entry']:
            return handle_create_op(obj, params)
        if params['operation'] == 'delete' and obj.get_key() == _keys['base-interface/entry']:
            return handle_delete_op(obj)
        if params['operation'] == 'set' and obj.get_key() == _keys['base-interface/entry']:
            return handle_set_obj(obj)
    except Exception as e:
        print "Failed to commit operation.", e
        print params

    return False

if __name__ == '__main__':
    handle = cps.obj_init()
    d = {}
    d['transaction'] = trans_cb
    d['get'] = get_cb

    for i in _keys.keys():
        if i.find('base-interface') == -1:
            continue
        cps.obj_register(handle, _keys[i], d)

    while True:
        time.sleep(1)
