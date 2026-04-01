# Air Traffic Control (ATC) System - Detailed Documentation
## COEN 320 - Real-Time Systems Project
### Achal & Parsa (Group: Achal_Parsa_320)

---

## 1. Detailed System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              ATC SYSTEM ARCHITECTURE                                 │
│                              Achal & Parsa - COEN 320                               │
└─────────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                   Lab4_ATC Process                                   │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                              AirTrafficControl                               │   │
│  │  - Reads planes.txt                                                          │   │
│  │  - Creates Aircraft objects                                                  │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                        │                                            │
│                                        ▼                                            │
│  ┌──────────────────────┐    ┌──────────────────────┐                              │
│  │     Aircraft[0]      │    │     Aircraft[n]      │                              │
│  │  (pthread thread)    │... │  (pthread thread)    │                              │
│  │                      │    │                      │                              │
│  │ Channel:             │    │ Channel:             │                              │
│  │ Achal_Parsa_320_     │    │ Achal_Parsa_320_     │                              │
│  │ plane_<id>           │    │ plane_<id>           │                              │
│  └──────────┬───────────┘    └──────────┬───────────┘                              │
│             │ MsgSend(ENTER/EXIT/POS)   │                                          │
│             └───────────┬───────────────┘                                          │
│                         ▼                                                           │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                                  Radar                                       │   │
│  │  Channel: Achal_Parsa_320_radar                                             │   │
│  │                                                                              │   │
│  │  Thread 1: ListenAirspaceArrivalAndDeparture()                              │   │
│  │            - MsgReceive() for ENTER_AIRSPACE, EXIT_AIRSPACE                 │   │
│  │                                                                              │   │
│  │  Thread 2: ListenUpdatePosition()                                           │   │
│  │            - pollAirspace() -> MsgSend(REQUEST_POSITION) to each aircraft   │   │
│  │            - writeToSharedMemory()                                          │   │
│  └──────────────────────────────────┬──────────────────────────────────────────┘   │
└─────────────────────────────────────┼───────────────────────────────────────────────┘
                                      │
                                      │ pthread_mutex_lock/unlock
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         SHARED MEMORY: /shm_Achal_Parsa_320                         │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  struct SharedMemory {                                                       │   │
│  │      msg_plane_info plane_data[100];  // aircraft positions                 │   │
│  │      int count;                        // number of planes                   │   │
│  │      atomic<bool> is_empty;            // flag for empty airspace           │   │
│  │      bool start;                       // radar has started                  │   │
│  │      uint64_t timestamp;               // tick counter                       │   │
│  │      pthread_mutex_t accessLock;       // sync (not semaphore)              │   │
│  │  }                                                                           │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────┬───────────────────────────────────────────────┘
                                      │
                                      │ pthread_mutex_lock/unlock
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              Lab5_Computer Process                                   │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                             ComputerSystem                                   │   │
│  │  Channel: Achal_Parsa_320_computer                                          │   │
│  │                                                                              │   │
│  │  Thread 1: monitorAirspace()                                                │   │
│  │            - reads SharedMemory                                             │   │
│  │            - calls checkCollision() -> willCollide()                        │   │
│  │            - sends COLLISION_DETECTED to Display                            │   │
│  │                                                                              │   │
│  │  Thread 2: operatorListener()                                               │   │
│  │            - MsgReceive() for operator commands                             │   │
│  │            - forwardToAircraft() via MsgSend to aircraft channel            │   │
│  └──────────────────────────────────┬──────────────────────────────────────────┘   │
│                                     │                                               │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                            OperatorConsole                                   │   │
│  │  - Reads stdin commands                                                      │   │
│  │  - MsgSend() to ComputerSystem channel                                      │   │
│  │  Commands: speed, pos, alt, info, lookahead, quit                           │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────┬───────────────────────────────────────────────┘
                                      │
                                      │ MsgSend(COLLISION_DETECTED)
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              Lab5_Display Process                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                             DisplaySystem                                    │   │
│  │  Channel: Achal_Parsa_320_display                                           │   │
│  │                                                                              │   │
│  │  Thread 1: displayLoop()                                                    │   │
│  │            - reads SharedMemory every 5 seconds                             │   │
│  │            - printAirspace() -> formatted table output                      │   │
│  │                                                                              │   │
│  │  Thread 2: messageListener()                                                │   │
│  │            - MsgReceive() for COLLISION_DETECTED                            │   │
│  │            - prints collision warnings                                       │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Message Flow Diagram

```
                    ENTER_AIRSPACE
Aircraft ──────────────────────────────────► Radar
    │                                           │
    │◄──────────────────────────────────────────┤ MsgReply(planeID)
    │                                           │
    │         REQUEST_POSITION                  │
    │◄──────────────────────────────────────────┤ (polling every 1 sec)
    │                                           │
    │         POSITION_UPDATE                   │
    ├──────────────────────────────────────────►│
    │                                           │
    │                                           │ writeToSharedMemory()
    │                                           ▼
    │                                    ┌──────────────┐
    │                                    │ SharedMemory │
    │                                    └──────┬───────┘
    │                                           │
    │                                           │ read by ComputerSystem
    │                                           ▼
    │                                    ComputerSystem
    │                                           │
    │    REQUEST_CHANGE_OF_HEADING              │ checkCollision()
    │◄──────────────────────────────────────────┤
    │                                           │
    │                                           │ COLLISION_DETECTED
    │                                           ▼
    │                                    DisplaySystem
    │                                           │
    │         EXIT_AIRSPACE                     │ printAirspace()
    ├──────────────────────────────────────────►│ (collision warnings)
                                         Radar  │
```

---

## 3. IPC Channel Names

| Channel Name | Owner Process | Purpose |
|--------------|---------------|---------|
| `Achal_Parsa_320_radar` | Radar | Aircraft send ENTER/EXIT messages |
| `Achal_Parsa_320_plane_<id>` | Aircraft | Radar polls position, ComputerSystem sends commands |
| `Achal_Parsa_320_computer` | ComputerSystem | OperatorConsole sends commands |
| `Achal_Parsa_320_display` | DisplaySystem | ComputerSystem sends collision alerts |

---

## 4. Code Structure (UML-Style)

### Lab4_ATC Classes

```
┌─────────────────────────────────────────────────────────────────┐
│                          Aircraft                                │
├─────────────────────────────────────────────────────────────────┤
│ - id: int                                                        │
│ - posX, posY, posZ: double                                      │
│ - speedX, speedY, speedZ: double                                │
│ - arrivalTime: int                                              │
│ - Radar_id: int                    // connection to radar       │
│ - thread_id: pthread_t                                          │
│ - airspace: airspace_struct                                     │
├─────────────────────────────────────────────────────────────────┤
│ + Aircraft(id, x, y, z, sx, sy, sz, t)                         │
│   └─► pthread_create(&thread_id, updatePositionThread)          │
│ + updatePosition(): int            // main thread loop          │
│   └─► name_open(ATC_RADAR_CHANNEL)                              │
│   └─► MsgSend(ENTER_AIRSPACE)                                   │
│   └─► name_attach(getAircraftChannel(id))                       │
│   └─► loop: update pos, MsgReceive, handle commands             │
│   └─► MsgSend(EXIT_AIRSPACE)                                    │
│ - createEnterAirspaceMessage(planeID): Message                  │
│ - createExitAirspaceMessage(planeID): Message                   │
│ - createPositionUpdateMessage(planeID, info): Message           │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                            Radar                                 │
├─────────────────────────────────────────────────────────────────┤
│ - tick_counter_ref: uint64_t&                                   │
│ - planesInAirspace: unordered_set<int>                          │
│ - planesInAirspaceData[2]: vector<msg_plane_info>  // double buf│
│ - activeBufferIndex: atomic<int>                                │
│ - sharedMemPtr: SharedMemory*                                   │
│ - shm_fd: int                                                   │
│ - Radar_channel: name_attach_t*                                 │
│ - stopThreads: atomic<bool>                                     │
├─────────────────────────────────────────────────────────────────┤
│ + Radar(tick_counter)                                           │
│   └─► setupSharedMemory()          // creates shm with mutex    │
│   └─► name_attach(ATC_RADAR_CHANNEL)                            │
│   └─► thread(ListenAirspaceArrivalAndDeparture)                 │
│   └─► thread(ListenUpdatePosition)                              │
│ + ListenAirspaceArrivalAndDeparture()                           │
│   └─► MsgReceive() -> addPlaneToAirspace/removePlaneFromAirspace│
│ + ListenUpdatePosition()                                        │
│   └─► pollAirspace() -> getAircraftData() for each plane        │
│   └─► writeToSharedMemory()                                     │
│ - setupSharedMemory(): bool        // uses mutex not semaphore  │
│ - writeToSharedMemory()            // pthread_mutex_lock        │
│ - clearSharedMemory()                                           │
│ - getAircraftData(id): msg_plane_info                           │
│   └─► name_open(getAircraftChannel(id))                         │
│   └─► MsgSend(REQUEST_POSITION)                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      AirTrafficControl                           │
├─────────────────────────────────────────────────────────────────┤
│ - planeData: vector<PlaneData>                                  │
│ - planes: vector<Aircraft*>                                     │
├─────────────────────────────────────────────────────────────────┤
│ + readPlaneData(fileName)          // parses planes.txt         │
│ + startPlanes()                    // creates Aircraft objects  │
│ + allPlanesFinished(): bool                                     │
└─────────────────────────────────────────────────────────────────┘
```

### Lab5_Computer Classes

```
┌─────────────────────────────────────────────────────────────────┐
│                        ComputerSystem                            │
├─────────────────────────────────────────────────────────────────┤
│ - shared_mem: SharedMemory*                                     │
│ - shm_fd: int                                                   │
│ - computerChannel: name_attach_t*                               │
│ - collisionLookahead: int          // default 180 sec           │
│ - activeCollisions: set<pair<int,int>>                          │
│ - running: atomic<bool>                                         │
├─────────────────────────────────────────────────────────────────┤
│ + startMonitoring(): bool                                       │
│   └─► initializeSharedMemory()     // shm_open, mmap            │
│   └─► thread(monitorAirspace)                                   │
│   └─► thread(operatorListener)                                  │
│ - monitorAirspace()                                             │
│   └─► pthread_mutex_lock(&shared_mem->accessLock)               │
│   └─► read plane_data                                           │
│   └─► checkCollision()                                          │
│ - checkCollision(time, planes)                                  │
│   └─► for each pair: willCollide()                              │
│   └─► sendCollisionToDisplay()                                  │
│ - willCollide(p1, p2): bool        // replaced checkAxes        │
│   └─► samples positions every 10 sec up to lookahead            │
│   └─► checks if within CONSTRAINT_X/Y/Z                         │
│ - operatorListener()                                            │
│   └─► name_attach(ATC_COMPUTER_CHANNEL)                         │
│   └─► MsgReceive() -> handleOperatorMessage()                   │
│ - forwardToAircraft(msg): bool                                  │
│   └─► name_open(getAircraftChannel(planeID))                    │
│   └─► MsgSend()                                                 │
│ - sendCollisionToDisplay(msg)                                   │
│   └─► name_open(ATC_DISPLAY_CHANNEL)                            │
│   └─► MsgSend(COLLISION_DETECTED)                               │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                       OperatorConsole                            │
├─────────────────────────────────────────────────────────────────┤
│ - consoleThread: thread                                         │
│ - shouldExit: atomic<bool>                                      │
├─────────────────────────────────────────────────────────────────┤
│ + OperatorConsole()                                             │
│   └─► thread(HandleConsoleInputs)                               │
│ - HandleConsoleInputs()                                         │
│   └─► select() for non-blocking stdin                           │
│   └─► parse command: speed/pos/alt/info/lookahead/quit          │
│   └─► sendCommand()                                             │
│ - sendCommand(msg): bool                                        │
│   └─► name_open(ATC_COMPUTER_CHANNEL)                           │
│   └─► MsgSend()                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Lab5_Display Classes

```
┌─────────────────────────────────────────────────────────────────┐
│                        DisplaySystem                             │
├─────────────────────────────────────────────────────────────────┤
│ - shared_mem: SharedMemory*                                     │
│ - shm_fd: int                                                   │
│ - displayChannel: name_attach_t*                                │
│ - running: atomic<bool>                                         │
├─────────────────────────────────────────────────────────────────┤
│ + start(): bool                                                 │
│   └─► name_attach(ATC_DISPLAY_CHANNEL)                          │
│   └─► connectToSharedMemory()                                   │
│   └─► thread(displayLoop)                                       │
│   └─► thread(messageListener)                                   │
│ - displayLoop()                                                 │
│   └─► every 5 sec: read SharedMemory                            │
│   └─► printAirspace()                                           │
│ - messageListener()                                             │
│   └─► MsgReceive()                                              │
│   └─► handle COLLISION_DETECTED -> print warning                │
│ - printAirspace(timestamp, planes)                              │
│   └─► formatted table output                                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 5. Key Differences from School Template

| What | School Template | Our Implementation |
|------|-----------------|-------------------|
| **Sync mechanism** | Named semaphore (`sem_open`) | `pthread_mutex` in SharedMemory struct |
| **Collision check** | `checkAxes()` placeholder | `willCollide()` - samples future positions |
| **Channel names** | `"your_group_name_or_id"` | `Achal_Parsa_320_*` macros |
| **CommunicationsSystem** | Separate class with thread | Not used - `ComputerSystem.forwardToAircraft()` instead |

---

## 6. Data Structures

### Message (intra-process)
```cpp
struct Message {
    bool header;        // 0=intra, 1=inter process
    MessageType type;   // ENTER_AIRSPACE, EXIT_AIRSPACE, etc.
    int planeID;
    void* data;
    size_t dataSize;
};
```

### Message_inter_process (between processes)
```cpp
struct Message_inter_process {
    bool header;        // always 1 for inter-process
    MessageType type;
    int planeID;
    std::array<char, 256> data;  // serialized payload
    size_t dataSize;
};
```

### msg_plane_info (aircraft state)
```cpp
typedef struct {
    int id;
    double PositionX, PositionY, PositionZ;
    double VelocityX, VelocityY, VelocityZ;
} msg_plane_info;
```

---

## 7. Thread Summary

| Process | Thread | Function | Purpose |
|---------|--------|----------|---------|
| Lab4_ATC | main | main() | Setup, wait for completion |
| Lab4_ATC | per aircraft | updatePosition() | Position updates, IPC |
| Lab4_ATC | radar-1 | ListenAirspaceArrivalAndDeparture() | Handle enter/exit |
| Lab4_ATC | radar-2 | ListenUpdatePosition() | Poll aircraft, write shm |
| Lab5_Computer | main | main() | Setup |
| Lab5_Computer | monitor | monitorAirspace() | Read shm, collision detect |
| Lab5_Computer | operator | operatorListener() | Handle operator commands |
| Lab5_Computer | console | HandleConsoleInputs() | Parse user input |
| Lab5_Display | main | main() | Setup |
| Lab5_Display | display | displayLoop() | Periodic display |
| Lab5_Display | listener | messageListener() | Collision alerts |

---

## 8. Oral Exam Q&A

### Q: Why pthread_mutex instead of semaphore?
**A:** Both work for synchronization. I put the mutex inside SharedMemory struct with `PTHREAD_PROCESS_SHARED` attribute. This keeps the lock with the data. Semaphore would be a separate named object.

### Q: How does collision detection work?
**A:** `willCollide()` projects both planes' positions forward in time using their velocities. It samples every 10 seconds up to the lookahead horizon (default 180 sec). If at any sample point the planes are within 3000m horizontal and 1000m vertical, it returns true.

### Q: What happens when an aircraft enters airspace?
**A:** 
1. Aircraft thread wakes up at `arrivalTime`
2. Opens channel to Radar (`name_open`)
3. Sends `ENTER_AIRSPACE` message (`MsgSend`)
4. Radar adds plane to `planesInAirspace` set
5. Radar starts polling that aircraft for position

### Q: How do operator commands reach aircraft?
**A:**
1. OperatorConsole parses stdin command
2. Sends `Message_inter_process` to ComputerSystem channel
3. ComputerSystem's `operatorListener()` receives it
4. `forwardToAircraft()` opens aircraft's channel and sends message
5. Aircraft's `updatePosition()` loop receives and handles it

### Q: What's the double buffer in Radar?
**A:** `planesInAirspaceData[2]` - while one buffer is being written to shared memory, the other can be filled with new poll data. `activeBufferIndex` tracks which is current.

---

## Author
**Achal & Parsa**  
Group: Achal_Parsa_320  
Course: COEN 320 - Real-Time Systems  
Concordia University
