// Achal & Parsa - DisplaySystem

#ifndef DISPLAY_SYSTEM_H
#define DISPLAY_SYSTEM_H

#include <atomic>
#include <thread>
#include <vector>
#include <sys/dispatch.h>

#include "Msg_structs.h"

class DisplaySystem {
public:
    DisplaySystem();
    ~DisplaySystem();

    bool start();
    void wait();

private:
    bool connectToSharedMemory();
    void cleanupSharedMemory();
    void displayLoop();       // Periodically shows aircraft positions
    void messageListener();   // Receives collision alerts from ComputerSystem
    void printAirspace(uint64_t timestamp, const std::vector<msg_plane_info>& planes);

    int shm_fd;
    SharedMemory* shared_mem;
    std::atomic<bool> running;
    std::thread displayThread;
    std::thread listenerThread;
    name_attach_t* displayChannel;
};

#endif // DISPLAY_SYSTEM_H
