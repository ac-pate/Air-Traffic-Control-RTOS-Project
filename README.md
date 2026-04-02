# Air Traffic Control (ATC) System
## COEN 320 - Real-Time Systems Project
### Achal & Parsa (Group: Achal_Parsa_320)

---

## Project Overview

This is a real-time Air Traffic Control simulation running on **QNX Neutrino RTOS**. The system monitors aircraft in a defined airspace, detects potential collisions, and allows operators to issue commands to aircraft to avoid collisions.

### System Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Lab4_ATC      │     │  Lab5_Computer  │     │  Lab5_Display   │
│                 │     │                 │     │                 │
│  ┌───────────┐  │     │ ┌─────────────┐ │     │ ┌─────────────┐ │
│  │ Aircraft  │──┼──┐  │ │ComputerSys  │ │     │ │DisplaySystem│ │
│  │ (threads) │  │  │  │ │             │ │     │ │             │ │
│  └───────────┘  │  │  │ └─────────────┘ │     │ └─────────────┘ │
│                 │  │  │        │        │     │        ▲        │
│  ┌───────────┐  │  │  │        │        │     │        │        │
│  │   Radar   │──┼──┼──┼────────┼────────┼─────┼────────┘        │
│  │           │  │  │  │        │        │     │                 │
│  └───────────┘  │  │  │ ┌─────────────┐ │     │                 │
│                 │  │  │ │OperatorCon  │ │     │                 │
│                 │  │  │ │             │ │     │                 │
│                 │  │  │ └─────────────┘ │     │                 │
└─────────────────┘  │  └─────────────────┘     └─────────────────┘
                     │
                     └──── Shared Memory ("/atc_shm_40227663")
```

---

## Operator Console Commands

| Command | Description | Example |
|---------|-------------|---------|
| `speed <id> <vx> <vy> <vz>` | Change aircraft velocity | `speed 1 200 100 0` |
| `pos <id> <x> <y> <z>` | Change aircraft position | `pos 1 50000 50000 25000` |
| `alt <id> <altitude>` | Change aircraft altitude | `alt 1 30000` |
| `info <id>` | Display aircraft info | `info 1` |
| `lookahead <seconds>` | Set collision lookahead time | `lookahead 120` |
| `help` | Show help | `help` |
| `quit` | Exit system | `quit` |

---

## File Structure

```
achal_atc_project/
├── Lab4_ATC/
│   └── src/
│       ├── main.cpp              # Entry point
│       ├── Aircraft.cpp/h        # Aircraft thread logic
│       ├── Radar.cpp/h           # Radar + shared memory
│       ├── AirTrafficControl.cpp/h # Plane file loading
│       ├── ATCTimer.cpp/h        # Timer utility
│       ├── Msg_structs.h         # Message types, IPC names
│       └── planes.txt            # Aircraft input data
│
├── Lab5_Computer/
│   └── src/
│       ├── main.cpp              # Entry point
│       ├── ComputerSystem.cpp/h  # Collision detection
│       ├── OperatorConsole.cpp/h # User input handling
│       ├── CommunicationsSystem.cpp/h # Aircraft messaging
│       ├── ATCTimer.cpp/h        # Timer utility
│       └── Msg_structs.h         # Message types
│
├── Lab5_Display/
│   └── src/
│       ├── main.cpp              # Entry point
│       ├── DisplaySystem.cpp/h   # Visualization
│       ├── ATCTimer.cpp/h        # Timer utility
│       └── Msg_structs.h         # Message types
│
└── README.md                     # This file
```

---

## planes.txt Format

```
<arrival_time> <plane_id> <x> <y> <z> <vx> <vy> <vz>
```

Example:
```
0 1 95750 59850 29950 250 150 50
5 2 75000 45000 35000 500 100 -75
3 3 95750 59850 29950 250 150 50
```

---

## Safety Constraints

- **Horizontal separation:** 3000 meters (X and Y axes)
- **Vertical separation:** 1000 meters (Z axis / altitude)
- **Collision lookahead:** 180 seconds (configurable)

---

## Author

**Achal & Parsa**  
Group: Achal_Parsa_320  
Course: COEN 320 - Real-Time Systems  
Concordia University
