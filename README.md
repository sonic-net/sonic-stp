# sonic-nas-linux
This repo contains the Linux integration portion of the network abstraction service (NAS). Linux utilities provide functions to manipulate IP, interfaces, VLAN, route, bridge, and so on via common Linux APIs.

## Build
---------
See [sonic-nas-manifest](https://github.com/Azure/sonic-nas-manifest) for more information on common build tools.

### Build dependencies
* `sonic-common-utils`
* `sonic-nas-common`
* `sonic-object-library`
* `sonic-logging`
* `sonic-base-model`

### Dependent packages
* `libsonic-logging-dev` 
* `libsonic-logging1` 
* `libsonic-model1`
* `libsonic-model-dev`
* `libsonic-common1` 
* `libsonic-common-dev`
* `libsonic-object-library1`
* `libsonic-object-library-dev`
* `libsonic-nas-common1`
* `libsonic-nas-common-dev`
* `sonic-ndi-api-dev`
* `libsonic-nas-ndi1`
* `libsonic-nas-ndi-dev`

BUILD CMD: sonic_build libsonic-logging-dev libsonic-logging1 libsonic-model1 libsonic-model-dev libsonic-common1 libsonic-common-dev libsonic-object-library1 libsonic-object-library-dev libsonic-nas-common1 libsonic-nas-common-dev sonic-ndi-api-dev libsonic-nas-ndi1 libsonic-nas-ndi-dev -- clean binary

(c) Dell 2016
