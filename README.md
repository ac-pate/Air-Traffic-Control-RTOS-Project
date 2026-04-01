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

## How to Run

### Step 1: Start Lab4_ATC (Radar + Aircraft)
```bash
cd /path/to/Lab4_ATC
./Lab4_ATC_ARCH64
```
This starts the Radar and spawns aircraft threads based on `planes.txt`.

### Step 2: Start Lab5_Computer (Collision Detection + Operator Console)
```bash
cd /path/to/Lab5_Computer
./ATC_Computer
```
This monitors shared memory for aircraft data and detects collisions.

### Step 3: Start Lab5_Display (Visualization)
```bash
cd /path/to/Lab5_Display
./ATC_Display
```
This shows aircraft positions and collision warnings.

---

## My Unique Design Choices

### 1. IPC Naming Convention (Msg_structs.h)
Used `#define` macros instead of `static const char*` variables:
```cpp
#define ATC_RADAR_CHANNEL      "Achal_Parsa_320_radar"
#define ATC_DISPLAY_CHANNEL    "Achal_Parsa_320_display"
#define ATC_COMPUTER_CHANNEL   "Achal_Parsa_320_computer"
#define ATC_SHM_NAME           "/shm_Achal_Parsa_320"
```

Macros are resolved at compile-time. Group name avoids conflicts with other groups.

### 2. Shared Memory Synchronization (Radar.cpp)
Used `pthread_mutex` embedded in shared memory instead of named semaphore:
```cpp
struct SharedMemory {
    msg_plane_info plane_data[MAX_PLANES_IN_AIRSPACE];
    int count;
    std::atomic<bool> is_empty;
    bool start;
    uint64_t timestamp;
    pthread_mutex_t accessLock;  // mutex in shm, not separate semaphore
};
```

Mutex initialized with `PTHREAD_PROCESS_SHARED` so all processes can use it.

### 3. Collision Detection Algorithm (ComputerSystem.cpp)
Replaced `checkAxes()` with `willCollide()` - samples future positions:
```cpp
bool ComputerSystem::willCollide(const msg_plane_info& p1, const msg_plane_info& p2) {
    for (int t = 0; t <= collisionLookahead; t += 10) {
        // project positions forward
        double futureX1 = p1.PositionX + p1.VelocityX * t;
        // ... check if within safety constraints
    }
}
```

Samples every 10 sec up to lookahead horizon (default 180 sec).

---

## Key Concepts for Oral Exam

### Q: What is QNX message passing?
**A:** QNX uses synchronous message passing for IPC. A client calls `MsgSend()` which blocks until the server calls `MsgReceive()` and then `MsgReply()`. This provides built-in synchronization.

### Q: How does shared memory work?
**A:** 
1. `shm_open()` creates/opens a shared memory object
2. `ftruncate()` sets its size
3. `mmap()` maps it into the process address space
4. Multiple processes can map the same object and share data

### Q: How do you synchronize shared memory access?
**A:** I use a `pthread_mutex` with `PTHREAD_PROCESS_SHARED` attribute embedded in the shared memory structure. Before reading/writing, processes lock the mutex.

### Q: How does collision detection work?
**A:** For each pair of aircraft:
1. Project their positions forward in time using their velocities
2. At each time step, check if the distance between them is less than the safety constraints (3000m horizontal, 1000m vertical)
3. If they get too close within the lookahead period, flag a collision

### Q: What are the threads in the system?
**A:**
- **Lab4_ATC:** One thread per aircraft (position updates), two Radar threads (arrival/departure listener, position poller)
- **Lab5_Computer:** Monitor thread (reads shared memory), operator listener thread
- **Lab5_Display:** Display thread (periodic output), message listener thread

### Q: How do aircraft receive commands?
**A:** Each aircraft creates a named channel (`ATC_40227663_aircraft_<id>`). The ComputerSystem forwards operator commands via `MsgSend()` to the aircraft's channel. The aircraft handles `REQUEST_CHANGE_OF_HEADING`, `REQUEST_CHANGE_POSITION`, and `REQUEST_CHANGE_ALTITUDE` messages.

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
