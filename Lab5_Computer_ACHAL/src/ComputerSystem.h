// Coen320 Task: Read this file and make the ComputerSystem.cpp file with the
// implementation of the functions declared in this header file
// Achal & Parsa - collision detection by sampling future positions
#ifndef COMPUTER_SYSTEM_H
#define COMPUTER_SYSTEM_H

#include <iostream>
#include <thread>
#include <atomic>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/dispatch.h>
#include <unistd.h>
#include <vector>
#include <set>

#include "Msg_structs.h"  // Include the structure definition for msg_plane_info

// Collision constraints - minimum safe separation
const double CONSTRAINT_X = 3000;
const double CONSTRAINT_Y = 3000;
const double CONSTRAINT_Z = 1000;

class ComputerSystem {
public:
    ComputerSystem();
    ~ComputerSystem();

    bool startMonitoring();
    void joinThread();

private:
    void monitorAirspace();
    void operatorListener();  // Listens for operator commands
    bool initializeSharedMemory();
    void cleanupSharedMemory();

    //Collsion detection - replaced checkAxes with willCollide
    void checkCollision(uint64_t currentTime, const std::vector<msg_plane_info>& planes);
    bool willCollide(const msg_plane_info& p1, const msg_plane_info& p2);
    double predictDistance(const msg_plane_info& p1, const msg_plane_info& p2, double t);

    //Handle messages from operator
    void handleOperatorMessage(const Message_inter_process& msg, int rcvid);
    bool forwardToAircraft(const Message_inter_process& msg);
    void sendCollisionToDisplay(const Message_inter_process& msg);

    int collisionLookahead;  // seconds to look ahead
    int shm_fd;
    SharedMemory* shared_mem;
    std::thread monitorThread;
    std::thread operatorThread;
    std::atomic<bool> running;
    name_attach_t* computerChannel;
    
    // Track which collision pairs we've already warned about
    std::set<std::pair<int,int>> activeCollisions;
};

#endif // COMPUTER_SYSTEM_H
