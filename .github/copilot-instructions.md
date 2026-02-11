# Copilot Instructions for sonic-stp

## Project Overview

sonic-stp implements the Spanning Tree Protocol (STP) for SONiC switches. It supports Multiple Spanning Tree Protocol (MSTP/802.1s), Rapid STP (RSTP/802.1w), and classic STP (802.1D). STP prevents L2 network loops by computing a loop-free topology and blocking redundant links. The stpd daemon processes BPDUs and manages port states.

## Architecture

```
sonic-stp/
├── stp/                 # Classic STP implementation
├── mstp/                # Multiple STP (MSTP) implementation
├── stpsync/             # STP-to-database sync (stpsyncd)
├── stpctl/              # STP control utility
├── include/             # Header files
├── lib/                 # Shared libraries
├── stpd_main.cpp        # STP daemon entry point
├── debian/              # Debian packaging
└── .azure-pipelines/    # CI pipeline definitions
```

### Key Concepts
- **BPDU processing**: Receives and transmits Bridge Protocol Data Units
- **Port states**: Blocking, Listening, Learning, Forwarding, Disabled
- **Root bridge election**: Lowest bridge ID becomes root
- **stpsyncd**: Syncs STP state to SONiC Redis databases
- **MSTP instances**: Multiple spanning tree instances for VLAN groups

## Language & Style

- **Primary language**: C++ (daemon), C (core STP logic)
- **Indentation**: 4 spaces
- **Naming conventions**:
  - Functions: `snake_case`
  - Structs: `snake_case`
  - Constants/macros: `UPPER_CASE`
  - Classes (C++): `PascalCase`
- **Code style**: Follow existing patterns in the codebase

## Build Instructions

```bash
# Build from sonic-buildimage context
# STP is built as part of the SONiC image build

# Standalone build
make

# Debian package
dpkg-buildpackage -us -uc -b
```

## Testing

- Integration tests via sonic-mgmt test suite
- Protocol conformance testing with standard STP test tools
- VS (virtual switch) platform for basic validation

## PR Guidelines

- **Commit format**: `[stp|mstp|stpsync]: Description`
- **Signed-off-by**: REQUIRED (`git commit -s`)
- **CLA**: Sign Linux Foundation EasyCLA
- **IEEE compliance**: Must comply with IEEE 802.1D/802.1w/802.1s standards
- **Interoperability**: Test with other STP implementations (Cisco, Arista, etc.)

## Common Patterns

### BPDU Processing Flow
```
Receive BPDU → Parse → Update port/bridge info →
Run STP algorithm → Compute new topology →
Update port states → Sync to DB via stpsyncd
```

### Database Integration
```
# STP state is synced to STATE_DB
STP_PORT_TABLE|Ethernet0 → { "state": "forwarding", "role": "designated" }
STP_VLAN_TABLE|Vlan100 → { "root_bridge": "...", "root_cost": "..." }
```

## Dependencies

- **sonic-swss-common**: Database connectivity
- **libnl**: Netlink for port state management
- **Linux kernel**: Bridge module for port state control

## Gotchas

- **IEEE standards compliance**: STP timers and state transitions must strictly follow IEEE specifications
- **Convergence time**: Changes to the algorithm affect network convergence — test extensively
- **VLAN awareness**: MSTP maps VLANs to spanning tree instances — mapping must be correct
- **Port flapping**: Rapid link state changes can cause STP instability
- **Interaction with LAG**: LAG/port-channel members need special STP handling
- **Root guard / BPDU guard**: Security features that prevent unauthorized root bridge changes
- **Timer precision**: STP timers (hello, max age, forward delay) are critical — don't approximate
- **L2 loops**: Bugs in STP cause broadcast storms — test failover scenarios thoroughly
