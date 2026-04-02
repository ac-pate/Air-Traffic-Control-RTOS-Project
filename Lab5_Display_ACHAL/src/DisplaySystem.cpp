// Achal & Parsa - DisplaySystem

#include "DisplaySystem.h"
#include "ATCTimer.h"

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <sys/mman.h>
#include <unistd.h>

DisplaySystem::DisplaySystem()
    : shm_fd(-1),
      shared_mem(nullptr),
      running(false),
      displayChannel(nullptr) {}

DisplaySystem::~DisplaySystem() {
    running.store(false);
    wait();
    cleanupSharedMemory();
}

bool DisplaySystem::connectToSharedMemory() {
    while (running.load()) {
        shm_fd = shm_open(ATC_SHM_NAME, O_RDWR, 0666);
        if (shm_fd == -1) {
            std::cout << "Display: Waiting for Radar shared memory..." << std::endl;
            sleep(1);
            continue;
        }
        
        shared_mem = static_cast<SharedMemory*>(
            mmap(NULL, ATC_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)
        );
        
        if (shared_mem == MAP_FAILED) {
            perror("Display: mmap failed");
            shared_mem = nullptr;
            close(shm_fd);
            shm_fd = -1;
            sleep(1);
            continue;
        }
        
        std::cout << "Display: Connected to shared memory" << std::endl;
        return true;
    }
    return false;
}

void DisplaySystem::cleanupSharedMemory() {
    if (shared_mem && shared_mem != MAP_FAILED) {
        munmap(shared_mem, ATC_SHM_SIZE);
        shared_mem = nullptr;
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
    if (displayChannel != nullptr) {
        name_detach(displayChannel, 0);
        displayChannel = nullptr;
    }
}

bool DisplaySystem::start() {
    running.store(true);
    
    // Create display channel first
    displayChannel = name_attach(NULL, ATC_DISPLAY_CHANNEL, 0);
    if (displayChannel == nullptr) {
        perror("Display: Failed to create channel");
        running.store(false);
        return false;
    }
    
    if (!connectToSharedMemory()) {
        running.store(false);
        return false;
    }
    
    displayThread = std::thread(&DisplaySystem::displayLoop, this);
    listenerThread = std::thread(&DisplaySystem::messageListener, this);
    
    return true;
}

void DisplaySystem::wait() {
    if (displayThread.joinable()) {
        displayThread.join();
    }
    
    // Send exit message to unblock listener
    if (listenerThread.joinable()) {
        int coid = name_open(ATC_DISPLAY_CHANNEL, 0);
        if (coid != -1) {
            Message_inter_process exitMsg{};
            exitMsg.header = true;
            exitMsg.type = MessageType::EXIT;
            int reply;
            MsgSend(coid, &exitMsg, sizeof(exitMsg), &reply, sizeof(reply));
            name_close(coid);
        }
        listenerThread.join();
    }
}

void DisplaySystem::messageListener() {
    while (running.load()) {
        Message_inter_process msg{};
        int rcvid = MsgReceive(displayChannel->chid, &msg, sizeof(msg), NULL);
        if (rcvid == -1) continue;
        if (rcvid == 0) continue;  // pulse
        
        int reply = 0;
        
        switch (msg.type) {
            case MessageType::COLLISION_DETECTED: {
                // Extract collision pairs and display warning
                size_t numPairs = msg.dataSize / sizeof(std::pair<int,int>);
                const std::pair<int,int>* pairs = 
                    reinterpret_cast<const std::pair<int,int>*>(msg.data.data());
                
                std::cout << "\n";
                std::cout << "╔══════════════════════════════════════════════════════╗" << std::endl;
                std::cout << "║           *** COLLISION WARNING ***                  ║" << std::endl;
                std::cout << "╠══════════════════════════════════════════════════════╣" << std::endl;
                for (size_t i = 0; i < numPairs; ++i) {
                    std::cout << "║  Aircraft " << std::setw(3) << pairs[i].first 
                              << " and Aircraft " << std::setw(3) << pairs[i].second 
                              << " - COLLISION RISK!   ║" << std::endl;
                }
                std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;
                std::cout << "\n";
                break;
            }
            
            case MessageType::REQUEST_AUGMENTED_INFO: {
                if (msg.dataSize >= sizeof(msg_plane_info)) {
                    msg_plane_info plane;
                    std::memcpy(&plane, msg.data.data(), sizeof(msg_plane_info));
                    
                    std::cout << "\n=== Aircraft " << plane.id << " Details ===" << std::endl;
                    std::cout << "Position: (" << plane.PositionX << ", " 
                              << plane.PositionY << ", " << plane.PositionZ << ")" << std::endl;
                    std::cout << "Velocity: (" << plane.VelocityX << ", " 
                              << plane.VelocityY << ", " << plane.VelocityZ << ")" << std::endl;
                    std::cout << "==============================\n" << std::endl;
                }
                break;
            }
            
            case MessageType::EXIT:
                running.store(false);
                break;
                
            default:
                break;
        }
        
        MsgReply(rcvid, 0, &reply, sizeof(reply));
    }
}

void DisplaySystem::displayLoop() {
    // Display every 5 seconds (as per project requirements)
    ATCTimer timer(5, 0);
    std::vector<msg_plane_info> planes;
    bool sawPlanes = false;
    int emptyCount = 0;
    
    while (running.load()) {
        // Read from shared memory
        pthread_mutex_lock(&shared_mem->accessLock);
        bool isEmpty = shared_mem->is_empty.load();
        bool started = shared_mem->start;
        uint64_t timestamp = shared_mem->timestamp;
        
        planes.clear();
        if (!isEmpty && started) {
            for (int i = 0; i < shared_mem->count && i < MAX_PLANES_IN_AIRSPACE; ++i) {
                planes.push_back(shared_mem->plane_data[i]);
            }
        }
        pthread_mutex_unlock(&shared_mem->accessLock);
        
        if (!started) {
            timer.waitTimer();
            continue;
        }
        
        if (planes.empty()) {
            if (sawPlanes) {
                emptyCount++;
                if (emptyCount >= 2) {
                    std::cout << "Display: Airspace empty. Shutting down." << std::endl;
                    running.store(false);
                    break;
                }
            }
        } else {
            sawPlanes = true;
            emptyCount = 0;
            printAirspace(timestamp, planes);
        }
        
        timer.waitTimer();
    }
}

void DisplaySystem::printAirspace(uint64_t timestamp, const std::vector<msg_plane_info>& planes) {
    // Sort by plane ID for consistent display
    std::vector<msg_plane_info> sorted = planes;
    std::sort(sorted.begin(), sorted.end(), 
              [](const msg_plane_info& a, const msg_plane_info& b) { return a.id < b.id; });
    
    // Clear screen and move cursor to top
    std::cout << "\033[2J\033[H";
    std::cout << "┌────────────────────────────────────────────────────────────────────────┐" << std::endl;
    std::cout << "│              AIRSPACE DISPLAY - Tick: " << std::setw(6) << timestamp 
              << "                          │" << std::endl;
    std::cout << "│              Achal & Parsa - COEN 320                                  │" << std::endl;
    std::cout << "├────────┬────────────────────────────────┬─────────────────────────────┤" << std::endl;
    std::cout << "│ Plane  │          Position              │         Velocity            │" << std::endl;
    std::cout << "├────────┼────────────────────────────────┼─────────────────────────────┤" << std::endl;
    
    for (const auto& plane : sorted) {
        std::cout << "│ " << std::setw(5) << plane.id << "  │ "
                  << std::setw(9) << std::fixed << std::setprecision(0) << plane.PositionX << ", "
                  << std::setw(9) << plane.PositionY << ", "
                  << std::setw(7) << plane.PositionZ << " │ "
                  << std::setw(7) << plane.VelocityX << ", "
                  << std::setw(7) << plane.VelocityY << ", "
                  << std::setw(7) << plane.VelocityZ << " │" << std::endl;
    }
    
    std::cout << "└────────┴────────────────────────────────┴─────────────────────────────┘" << std::endl;
    std::cout << "  Total aircraft in airspace: " << sorted.size() << std::endl;
    std::cout << "\n";
}
