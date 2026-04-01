# QNX Neutrino Setup & Testing Guide
## COEN 320 - ATC Project
### Achal & Parsa

---

## 1. What is QNX Neutrino?

QNX is a real-time operating system (RTOS). Unlike Linux/Windows:
- **Microkernel architecture** - only essential stuff runs in kernel
- **Message passing** - processes communicate via `MsgSend/MsgReceive/MsgReply`
- **Shared memory** - processes can share memory regions via `shm_open/mmap`
- **Real-time scheduling** - predictable timing for threads

---

## 2. Project Structure - What Each Part Does

```
achal_atc_project/
├── Lab4_ATC/           <-- RUN THIS FIRST
│   └── src/
│       ├── main.cpp              # Entry point - creates Radar, loads planes
│       ├── AirTrafficControl.cpp # Reads planes.txt, creates Aircraft objects
│       ├── Aircraft.cpp          # Each plane runs as a pthread
│       ├── Radar.cpp             # Creates shared memory, polls aircraft
│       ├── Msg_structs.h         # Message types, channel names
│       └── planes.txt            # INPUT FILE - aircraft data
│
├── Lab5_Computer/      <-- RUN THIS SECOND
│   └── src/
│       ├── main.cpp              # Entry point
│       ├── ComputerSystem.cpp    # Reads shared memory, detects collisions
│       ├── OperatorConsole.cpp   # Reads your keyboard commands
│       └── Msg_structs.h         # Same as Lab4_ATC
│
└── Lab5_Display/       <-- RUN THIS THIRD
    └── src/
        ├── main.cpp              # Entry point
        ├── DisplaySystem.cpp     # Shows aircraft positions, collision warnings
        └── Msg_structs.h         # Same as Lab4_ATC
```

---

## 3. How to Build on QNX

### Option A: Using QNX Momentics IDE
1. Import each project (Lab4_ATC, Lab5_Computer, Lab5_Display) as separate projects
2. Right-click project → Build Project
3. Binary will be in `build/` or `x86_64/` folder

### Option B: Command Line (on QNX target)
```bash
# For each project folder:
cd Lab4_ATC/src
qcc -o atc_radar main.cpp AirTrafficControl.cpp Aircraft.cpp Radar.cpp ATCTimer.cpp -lpthread

cd Lab5_Computer/src
qcc -o atc_computer main.cpp ComputerSystem.cpp OperatorConsole.cpp CommunicationsSystem.cpp ATCTimer.cpp -lpthread

cd Lab5_Display/src
qcc -o atc_display main.cpp DisplaySystem.cpp ATCTimer.cpp -lpthread
```

### Compiler flags you might need:
```bash
qcc -Vgcc_ntoaarch64   # for ARM64 target
qcc -Vgcc_ntox86_64    # for x86_64 target
-Wall                   # warnings
-g                      # debug symbols
-lpthread               # pthread library (required)
```

---

## 4. How to Run

### Terminal 1: Start Lab4_ATC (Radar + Aircraft)
```bash
cd /path/to/Lab4_ATC
./atc_radar
```

**What you should see:**
```
Creating Aircraft 1: Pos(95750, 59850, 29950) Speed(250, 150, 50) ArrivalTime(0)
Creating Aircraft 2: Pos(75000, 45000, 35000) Speed(500, 100, -75) ArrivalTime(5)
Radar: Shared memory initialized at /shm_Achal_Parsa_320
Radar: Channel created
Aircraft 1: Entered airspace
Aircraft 2: Entered airspace
...
```

### Terminal 2: Start Lab5_Computer
```bash
cd /path/to/Lab5_Computer
./atc_computer
```

**What you should see:**
```
=== ATC Computer System (Achal & Parsa) ===
ComputerSystem: Connected to shared memory
ComputerSystem: Monitoring started

=== ATC Operator Console (Achal & Parsa) ===
Commands:
  speed <planeID> <vx> <vy> <vz>  - Change aircraft velocity
  ...
```

### Terminal 3: Start Lab5_Display
```bash
cd /path/to/Lab5_Display
./atc_display
```

**What you should see:**
```
=== ATC Display System (Achal & Parsa) ===
Display: Connected to shared memory

┌────────────────────────────────────────────────────────────────────────┐
│              AIRSPACE DISPLAY - Tick:      5                          │
│              Achal & Parsa - COEN 320                                  │
├────────┬────────────────────────────────────────────────────────────────┤
│ Plane  │          Position              │         Velocity            │
├────────┼────────────────────────────────┼─────────────────────────────┤
│     1  │     96000,     60000,    30000 │     250,     150,      50 │
│     2  │     77500,     45500,    34625 │     500,     100,     -75 │
└────────┴────────────────────────────────┴─────────────────────────────┘
```

---

## 5. Input File: planes.txt

Located in `Lab4_ATC/src/planes.txt`

### Format:
```
<arrival_time> <plane_id> <x> <y> <z> <vx> <vy> <vz>
```

### Example planes.txt:
```
0 1 95750 59850 29950 250 150 50
5 2 75000 45000 35000 500 100 -75
3 3 95750 59850 29950 250 150 50
10 4 50000 50000 25000 -100 200 0
```

### What each field means:
| Field | Description | Example |
|-------|-------------|---------|
| arrival_time | Seconds until plane enters airspace | 0 = immediately, 5 = after 5 sec |
| plane_id | Unique ID | 1, 2, 3... |
| x, y, z | Starting position (meters) | 95750, 59850, 29950 |
| vx, vy, vz | Velocity (m/s) | 250, 150, 50 |

### Test scenarios to try:

**Scenario 1: No collision (planes far apart)**
```
0 1 0 0 25000 100 0 0
0 2 100000 100000 35000 -100 0 0
```

**Scenario 2: Collision course (same position, same time)**
```
0 1 50000 50000 25000 100 0 0
0 2 50000 50000 25000 -100 0 0
```
Should trigger collision warning!

**Scenario 3: Near miss (close but not within constraints)**
```
0 1 50000 50000 25000 100 0 0
0 2 50000 53500 25000 100 0 0
```
3500m apart in Y - just outside 3000m constraint, no collision

**Scenario 4: Delayed entry**
```
0 1 0 0 25000 500 0 0
30 2 100000 0 25000 -500 0 0
```
Plane 2 enters after 30 seconds

---

## 6. Operator Console Commands

In Terminal 2 (Lab5_Computer), you can type commands:

| Command | What it does | Example |
|---------|--------------|---------|
| `speed 1 300 200 0` | Change plane 1 velocity to (300, 200, 0) | Avoid collision |
| `pos 1 60000 60000 30000` | Teleport plane 1 to new position | Emergency move |
| `alt 1 35000` | Change plane 1 altitude to 35000m | Vertical separation |
| `info 1` | Show plane 1 details | Debug |
| `lookahead 120` | Set collision detection to 120 sec ahead | Default is 180 |
| `quit` | Exit system | |

---

## 7. What to Observe

### Successful run:
1. Lab4_ATC prints aircraft entering airspace
2. Lab5_Display shows table updating every 5 seconds
3. Aircraft positions change based on velocity
4. When planes get close, collision warning appears
5. Aircraft exit when they leave airspace boundaries

### Collision warning looks like:
```
╔══════════════════════════════════════════════════════════════╗
║           *** COLLISION WARNING ***                          ║
╠══════════════════════════════════════════════════════════════╣
║  Aircraft   1 and Aircraft   3 - COLLISION RISK!             ║
╚══════════════════════════════════════════════════════════════╝
```

### Normal exit:
```
Aircraft 1: Exiting airspace
Aircraft 2: Exiting airspace
Display: Airspace empty. Shutting down.
```

---

## 8. Common Problems & Fixes

### Problem: "name_open failed" or "Cannot connect to channel"
**Cause:** Process trying to connect before channel is created
**Fix:** Make sure Lab4_ATC is running FIRST before Lab5_Computer/Display

### Problem: "shm_open failed: Permission denied"
**Cause:** Previous run left shared memory behind
**Fix:** 
```bash
rm /dev/shmem/shm_Achal_Parsa_320
```

### Problem: "mmap failed"
**Cause:** Shared memory size mismatch or not created
**Fix:** Make sure Lab4_ATC started successfully first

### Problem: Display shows nothing / "Waiting for Radar shared memory"
**Cause:** Lab4_ATC not running or crashed
**Fix:** Check Terminal 1 for errors, restart Lab4_ATC

### Problem: Collision detection not working
**Cause:** Planes not close enough or lookahead too short
**Fix:** 
- Check planes.txt - are planes actually on collision course?
- Try `lookahead 300` to extend detection window
- Collision constraints: 3000m X, 3000m Y, 1000m Z

### Problem: Aircraft not moving
**Cause:** Timer not working or thread not started
**Fix:** Check if ATCTimer.cpp is compiled and linked

### Problem: Segmentation fault
**Cause:** Usually null pointer or bad memory access
**Fix:** 
- Check if shared memory initialized
- Check if channels opened successfully
- Run with debugger: `gdb ./atc_radar`

---

## 9. Debugging Tips

### Check if shared memory exists:
```bash
ls /dev/shmem/
# Should see: shm_Achal_Parsa_320
```

### Check if channels exist:
```bash
ls /dev/name/local/
# Should see: Achal_Parsa_320_radar, Achal_Parsa_320_computer, etc.
```

### Clean up after crash:
```bash
rm /dev/shmem/shm_Achal_Parsa_320
rm /dev/name/local/Achal_Parsa_320_*
```

### Add debug prints:
In ComputerSystem.cpp, add prints in `monitorAirspace()`:
```cpp
std::cout << "Read " << planes.size() << " planes from shm" << std::endl;
```

---

## 10. Airspace Boundaries

Aircraft exit when they leave these bounds (defined in Aircraft.cpp):
```cpp
airspace.minX = 0;      airspace.maxX = 100000;
airspace.minY = 0;      airspace.maxY = 100000;
airspace.minZ = 15000;  airspace.maxZ = 40000;
```

So if a plane's position goes outside 0-100000 in X/Y or 15000-40000 in Z, it exits.

---

## 11. Collision Constraints

Defined in ComputerSystem.cpp:
```cpp
#define CONSTRAINT_X 3000   // meters horizontal
#define CONSTRAINT_Y 3000   // meters horizontal  
#define CONSTRAINT_Z 1000   // meters vertical
```

Two planes collide if ALL THREE are true:
- |x1 - x2| < 3000m
- |y1 - y2| < 3000m
- |z1 - z2| < 1000m

---

## 12. Quick Test Checklist

- [ ] planes.txt exists in Lab4_ATC/src/
- [ ] All three binaries compiled without errors
- [ ] Terminal 1: Lab4_ATC running, shows "Shared memory initialized"
- [ ] Terminal 2: Lab5_Computer running, shows "Connected to shared memory"
- [ ] Terminal 3: Lab5_Display running, shows aircraft table
- [ ] Table updates every 5 seconds
- [ ] Aircraft positions change
- [ ] Type `info 1` in Terminal 2 - should show plane info
- [ ] Create collision scenario - warning should appear
- [ ] Aircraft exit when leaving airspace

---

## 13. File Dependencies

Make sure these files are the SAME across all projects:
- `Msg_structs.h` - channel names, message types, SharedMemory struct

If you change channel names in one, copy to all three projects!

---

## Author
Achal & Parsa  
Group: Achal_Parsa_320  
COEN 320 - Real-Time Systems
