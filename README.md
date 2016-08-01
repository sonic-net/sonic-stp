SONiC NAS linux
===============

Linux utilities for the SONiC project

Description
-----------

This repo contains the Linux integration portion of the Network abstraction service. The linux utilities provide functions to manipulate ip, interfaces, vlan, route, bridge, etc. via common linux APIs.

Building
---------
Please see the instructions in the sonic-nas-manifest repo for more details on the common build tools.  [Sonic-nas-manifest](https://github.com/Azure/sonic-nas-manifest)

Build Dependencies
 - sonic-common-utils
 - sonic-nas-common
 - sonic-object-library
 - sonic-logging
 - sonic-base-model

Dependent packages:
  libsonic-logging-dev libsonic-logging1 libsonic-model1 libsonic-model-dev libsonic-common1 libsonic-common-dev libsonic-object-library1 libsonic-object-library-dev libsonic-nas-common1 libsonic-nas-common-dev sonic-ndi-api-dev libsonic-nas-ndi1 libsonic-nas-ndi-dev

BUILD CMD: sonic_build libsonic-logging-dev libsonic-logging1 libsonic-model1 libsonic-model-dev libsonic-common1 libsonic-common-dev libsonic-object-library1 libsonic-object-library-dev libsonic-nas-common1 libsonic-nas-common-dev sonic-ndi-api-dev libsonic-nas-ndi1 libsonic-nas-ndi-dev -- clean binary

(c) Dell 2016
