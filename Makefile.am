bin_SCRIPTS = scripts/bin/*

pyutilsdir=$(libdir)/sonic
pyutils_SCRIPTS = scripts/lib/python/*.py

nobase_include_HEADERS=sonic/db_linux_event_register.h sonic/db_api_linux_init.h sonic/cps_api_interface_types.h sonic/cps_api_route.h sonic/nas_linux_l2.h sonic/nas_os_vlan.h sonic/nas_os_interface.h sonic/nas_os_lag.h sonic/nas_os_l3.h

lib_LTLIBRARIES=libsonic_nas_linux.la

libsonic_nas_linux_la_SOURCES=src/nas_os_int_utils.c src/nas_os_vlan_utils.c src/db_linux_interface.c src/net_main.cpp src/netlink_tools.c src/db_linux_route.c src/ds_linux_init.c src/ds_interface_name_tools.c src/ds_api_linux_neigh.c src/nas_os_vlan.c src/nas_os_lag.c src/nas_os_interface.cpp src/nas_os_stg.c src/nas_os_l3.c src/nas_os_ip.cpp src/nas_os_mac.cpp

libsonic_nas_linux_la_SOURCES+=src/if/os_interface_cache.cpp src/if/os_interface_lag.cpp src/if/os_interface_vlan.cpp src/if/os_interface.cpp src/if/os_interface_stg.cpp

libsonic_nas_linux_la_CPPFLAGS= -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/sonic -I$(top_srcdir)/sonic/private -I$(includedir)/sonic

libsonic_nas_linux_la_CXXFLAGS=-std=c++11

libsonic_nas_linux_la_LDFLAGS=-shared -version-info 1:1:0

libsonic_nas_linux_la_LIBADD=-lsonic_common -lsonic_object_library -lsonic_cps_class_map -lsonic_logging

systemdconfdir=/lib/systemd/system
systemdconf_DATA = scripts/init/*.service
