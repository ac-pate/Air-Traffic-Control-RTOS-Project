# TA-Suggested Improvement: Integrated Operator Console in Display System

**Authors:** Achal & Parsa  
**Course:** COEN 320 - Real-Time Systems  
**Improvement:** Move operator controls from Computer to Display for real-time aircraft management while viewing airspace

---

## Overview

The TA suggested integrating the operator console into the Display system rather than having it as a separate component in the Computer system. This improvement allows operators to control aircraft in real-time while simultaneously viewing the airspace display, providing a more intuitive and practical user experience.

---

## Architecture Change

### Before (Original Design)
```
Lab5_Computer_ACHAL:
  - ComputerSystem (collision detection)
  - OperatorConsole (user input) ← HERE
  
Lab5_Display_ACHAL:
  - DisplaySystem (airspace visualization)
```

### After (Improved Design)
```
Lab5_Computer_ACHAL:
  - ComputerSystem (collision detection)
  
Lab5_Display_ACHAL:
  - DisplaySystem (airspace visualization)
  - OperatorConsole (user input) ← MOVED HERE
```

---

## Key Code Changes

### 1. Lab5_Display_ACHAL/src/main.cpp (Modified)

**Purpose:** Integrate OperatorConsole with DisplaySystem for unified control interface

```cpp
// Achal & Parsa - ATC Display System with Integrated Operator Console
// TA Improvement: Operator controls moved to Display for real-time control while viewing airspace

#include "DisplaySystem.h"
#include "OperatorConsole.h"
#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "=== ATC Display System with Operator Console (Achal & Parsa) ===" << std::endl;
    std::cout << "Improvement: Integrated operator controls for real-time aircraft management" << std::endl;
    
    // Start display system
    DisplaySystem display;
    if (!display.start()) {
        std::cerr << "Display: Failed to start" << std::endl;
        return EXIT_FAILURE;
    }
    
    // Start operator console in parallel
    OperatorConsole console;
    
    std::cout << "\nDisplay running. Type 'help' for operator commands.\n" << std::endl;
    
    // Wait for display to finish
    display.wait();
    
    std::cout << "Display stopped. Exiting." << std::endl;
    return EXIT_SUCCESS;
}
```

**Key Changes:**
- Added `#include "OperatorConsole.h"`
- Instantiate `OperatorConsole console;` after DisplaySystem starts
- Both run in parallel threads - display updates screen, console handles user input
- User can see airspace and type commands simultaneously

---

### 2. Lab5_Display_ACHAL/src/OperatorConsole.h (New File)

**Purpose:** Operator console class definition moved from Computer to Display

```cpp
#ifndef OPERATORCONSOLE_H_
#define OPERATORCONSOLE_H_

// Achal & Parsa - operator console for aircraft commands
// Moved to Display for integrated control while viewing airspace

#include <iostream>
#include <sys/dispatch.h>
#include <thread>
#include <atomic>
#include "Msg_structs.h"

class OperatorConsole {
public:
    OperatorConsole();
    ~OperatorConsole();

private:
    void HandleConsoleInputs();
    bool sendCommand(const Message_inter_process& msg);
    void printHelp();
    std::thread consoleThread;
    std::atomic<bool> shouldExit;
};

#endif /* OPERATORCONSOLE_H_ */
```

**Key Features:**
- Runs in separate thread (`consoleThread`) to not block display updates
- Uses atomic flag for thread-safe exit signaling
- Communicates with ComputerSystem via IPC (unchanged)

---

### 3. Lab5_Display_ACHAL/src/OperatorConsole.cpp (New File)

**Purpose:** Implementation of operator command parsing and IPC communication

```cpp
// Achal & Parsa - OperatorConsole
// Moved to Display for integrated control while viewing airspace
// Parses user commands and sends to ComputerSystem via IPC

#include "OperatorConsole.h"
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/select.h>

OperatorConsole::OperatorConsole() : shouldExit(false) {
    consoleThread = std::thread(&OperatorConsole::HandleConsoleInputs, this);
}

OperatorConsole::~OperatorConsole() {
    shouldExit.store(true);
    if (consoleThread.joinable()) {
        consoleThread.join();
    }
}

void OperatorConsole::printHelp() {
    std::cout << "\n=== ATC Operator Console (Achal & Parsa) ===" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  speed <planeID> <vx> <vy> <vz>  - Change aircraft velocity" << std::endl;
    std::cout << "  pos <planeID> <x> <y> <z>      - Change aircraft position" << std::endl;
    std::cout << "  alt <planeID> <altitude>       - Change aircraft altitude" << std::endl;
    std::cout << "  info <planeID>                 - Get aircraft info" << std::endl;
    std::cout << "  lookahead <seconds>            - Set collision lookahead time" << std::endl;
    std::cout << "  help                           - Show this help" << std::endl;
    std::cout << "  quit                           - Exit" << std::endl;
    std::cout << "====================================================\n" << std::endl;
}

bool OperatorConsole::sendCommand(const Message_inter_process& msg) {
    int computerCoid = name_open(ATC_COMPUTER_CHANNEL, 0);
    if (computerCoid == -1) {
        std::cerr << "Error: Cannot connect to ComputerSystem" << std::endl;
        return false;
    }
    
    int reply;
    int result = MsgSend(computerCoid, &msg, sizeof(msg), &reply, sizeof(reply));
    name_close(computerCoid);
    
    if (result == -1) {
        std::cerr << "Error: Failed to send command" << std::endl;
        return false;
    }
    
    return (reply == 0);
}

void OperatorConsole::HandleConsoleInputs() {
    printHelp();
    
    while (!shouldExit.load()) {
        // Use select to check if input is available (with timeout)
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
        if (ready <= 0) continue;
        
        std::string line;
        std::getline(std::cin, line);
        if (!std::cin.good()) break;
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        
        Message_inter_process msg{};
        msg.header = true;
        
        if (cmd == "help") {
            printHelp();
            continue;
        }
        else if (cmd == "quit" || cmd == "exit") {
            msg.type = MessageType::EXIT;
            msg.planeID = -1;
            msg.dataSize = 0;
            sendCommand(msg);
            shouldExit.store(true);
            break;
        }
        else if (cmd == "speed") {
            int planeID;
            double vx, vy, vz;
            if (!(iss >> planeID >> vx >> vy >> vz)) {
                std::cout << "Usage: speed <planeID> <vx> <vy> <vz>" << std::endl;
                continue;
            }
            
            msg.type = MessageType::REQUEST_CHANGE_OF_HEADING;
            msg.planeID = planeID;
            
            msg_change_heading heading;
            heading.ID = planeID;
            heading.VelocityX = vx;
            heading.VelocityY = vy;
            heading.VelocityZ = vz;
            heading.altitude = -1;
            
            msg.dataSize = sizeof(msg_change_heading);
            std::memcpy(msg.data.data(), &heading, sizeof(heading));
            
            if (sendCommand(msg)) {
                std::cout << "Speed command sent to aircraft " << planeID << std::endl;
            }
        }
        else if (cmd == "pos") {
            int planeID;
            double x, y, z;
            if (!(iss >> planeID >> x >> y >> z)) {
                std::cout << "Usage: pos <planeID> <x> <y> <z>" << std::endl;
                continue;
            }
            
            msg.type = MessageType::REQUEST_CHANGE_POSITION;
            msg.planeID = planeID;
            
            msg_change_position position;
            position.x = x;
            position.y = y;
            position.z = z;
            
            msg.dataSize = sizeof(msg_change_position);
            std::memcpy(msg.data.data(), &position, sizeof(position));
            
            if (sendCommand(msg)) {
                std::cout << "Position command sent to aircraft " << planeID << std::endl;
            }
        }
        else if (cmd == "alt") {
            int planeID;
            double altitude;
            if (!(iss >> planeID >> altitude)) {
                std::cout << "Usage: alt <planeID> <altitude>" << std::endl;
                continue;
            }
            
            msg.type = MessageType::REQUEST_CHANGE_ALTITUDE;
            msg.planeID = planeID;
            msg.dataSize = sizeof(double);
            std::memcpy(msg.data.data(), &altitude, sizeof(double));
            
            if (sendCommand(msg)) {
                std::cout << "Altitude command sent to aircraft " << planeID << std::endl;
            }
        }
        else if (cmd == "lookahead") {
            int seconds;
            if (!(iss >> seconds)) {
                std::cout << "Usage: lookahead <seconds>" << std::endl;
                continue;
            }
            
            msg.type = MessageType::CHANGE_TIME_CONSTRAINT_COLLISIONS;
            msg.planeID = -1;
            msg.dataSize = sizeof(int);
            std::memcpy(msg.data.data(), &seconds, sizeof(int));
            
            if (sendCommand(msg)) {
                std::cout << "Collision lookahead set to " << seconds << " seconds" << std::endl;
            }
        }
        else {
            std::cout << "Unknown command: " << cmd << ". Type 'help' for available commands." << std::endl;
        }
    }
}
```

**Key Implementation Details:**
- **Non-blocking input:** Uses `select()` with timeout to check for user input without blocking display updates
- **Thread-safe:** Atomic flag for clean shutdown
- **IPC Communication:** Opens channel to ComputerSystem, sends commands via `MsgSend()`, closes channel
- **Command parsing:** Supports speed, position, altitude, lookahead time changes
- **Error handling:** Validates input and reports connection/send failures

---

### 4. Lab5_Computer_ACHAL/src/main.cpp (Modified)

**Purpose:** Remove OperatorConsole instantiation since it's now in Display

```cpp
// Achal & Parsa - ATC Computer System
// TA Improvement: OperatorConsole moved to Display for integrated control

#include "ComputerSystem.h"
#include "CommunicationsSystem.h"

int main() {
    std::cout << "=== ATC Computer System (Achal & Parsa) ===" << std::endl;
    std::cout << "Note: Operator console now integrated with Display system" << std::endl;
    
    ComputerSystem computerSystem;
    /*
    OperatorConsole implementation moved to Display system for real-time control
    while viewing airspace. ComputerSystem still handles operator messages via IPC.
    Message types: REQUEST_CHANGE_OF_HEADING, REQUEST_CHANGE_POSITION, REQUEST_CHANGE_ALTITUDE
    */
    
    if (computerSystem.startMonitoring()) {
        computerSystem.joinThread();
    } else {
        std::cerr << "Failed to start monitoring." << std::endl;
    }

    std::cout << "Monitoring stopped. Exiting main." << std::endl;
    return 0;
}
```

**Key Changes:**
- Removed `#include "OperatorConsole.h"`
- Removed `OperatorConsole console;` instantiation
- Added comment explaining the architectural change
- ComputerSystem still listens for operator commands via IPC (no change to ComputerSystem.cpp)

---

## Benefits of This Improvement

### 1. **Better User Experience**
- Operator sees aircraft positions and can immediately issue commands
- No need to switch between two console windows
- Real-time feedback: see command effects on display instantly

### 2. **More Intuitive Design**
- Display is the natural place for user interaction in a monitoring system
- Follows common UI/UX patterns (control where you view)
- Reduces cognitive load on operator

### 3. **Maintains Separation of Concerns**
- ComputerSystem still handles collision detection and command forwarding
- Display handles visualization and user input
- IPC communication unchanged - still uses message passing

### 4. **No Impact on Core Functionality**
- Collision detection logic unchanged
- Aircraft control mechanism unchanged
- Shared memory synchronization unchanged
- Only the location of user input changed

---

## System Flow (After Improvement)

```
User Input (Display Console)
    ↓
OperatorConsole.HandleConsoleInputs()
    ↓
sendCommand() → MsgSend() to ATC_COMPUTER_CHANNEL
    ↓
ComputerSystem.operatorListener()
    ↓
CommunicationsSystem.sendToAircraft()
    ↓
Aircraft receives command via IPC
    ↓
Aircraft updates position/velocity/altitude
    ↓
Radar polls aircraft, writes to shared memory
    ↓
DisplaySystem reads shared memory
    ↓
User sees updated display ← FEEDBACK LOOP
```

---

## Testing the Improvement

### Build Commands
```bash
cd Lab5_Display_ACHAL
make clean && make

cd ../Lab5_Computer_ACHAL
make clean && make
```

### Run Sequence
1. Start Lab4_ATC_ACHAL (Radar + Aircraft)
2. Start ATC_Computer_ACHAL (Collision Detection)
3. Start ATC_Display_ACHAL (Display + Operator Console)

### Example Usage
```
=== ATC Display System with Operator Console (Achal & Parsa) ===
Improvement: Integrated operator controls for real-time aircraft management

Display running. Type 'help' for operator commands.

┌────────────────────────────────────────────────────────────────────────┐
│              AIRSPACE DISPLAY - Tick:     45                          │
│              Achal & Parsa - COEN 320                                  │
├────────┬────────────────────────────────┬─────────────────────────────┤
│ Plane  │          Position              │         Velocity            │
├────────┼────────────────────────────────┼─────────────────────────────┤
│     1  │     96000,    60000,   30000 │     250,    150,     50 │
│     2  │     77500,    45500,   34625 │     500,    100,    -75 │
└────────┴────────────────────────────────┴─────────────────────────────┘
  Total aircraft in airspace: 2

> speed 1 300 200 75
Speed command sent to aircraft 1

> pos 2 50000 50000 25000
Position command sent to aircraft 2
```

---

## Technical Highlights for Lab Report

### 1. **Multi-threading**
- DisplaySystem runs in its own thread (periodic airspace updates)
- OperatorConsole runs in separate thread (user input handling)
- Both threads run concurrently without blocking each other

### 2. **IPC Message Passing**
- OperatorConsole → ComputerSystem: `name_open()`, `MsgSend()`, `name_close()`
- Uses `Message_inter_process` struct with 256-byte data buffer
- Synchronous communication with reply confirmation

### 3. **Screen Management**
- Display uses ANSI escape codes: `\033[2J\033[H` to clear screen
- Updates in place rather than scrolling
- Operator commands appear below display table

### 4. **Thread Safety**
- `std::atomic<bool> shouldExit` for clean shutdown
- No shared state between DisplaySystem and OperatorConsole
- Each component manages its own resources

---

## Conclusion

This improvement demonstrates understanding of:
- **Real-time system design:** Concurrent processes with IPC
- **User interface principles:** Integrated control and visualization
- **QNX Neutrino RTOS:** Message passing, threading, channels
- **Software architecture:** Separation of concerns, modularity

The change required minimal code modification (moved 2 files, updated 2 main.cpp files) but significantly improved usability and system design quality.

---

**Files Modified:**
- `Lab5_Display_ACHAL/src/main.cpp` (added OperatorConsole integration)
- `Lab5_Display_ACHAL/src/OperatorConsole.h` (new file, moved from Computer)
- `Lab5_Display_ACHAL/src/OperatorConsole.cpp` (new file, moved from Computer)
- `Lab5_Computer_ACHAL/src/main.cpp` (removed OperatorConsole instantiation)

**Files Unchanged:**
- All ComputerSystem collision detection logic
- All DisplaySystem visualization logic
- All IPC communication mechanisms
- All shared memory synchronization
