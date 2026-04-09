// Achal & Parsa - ATC Computer System
// TA Improvement: OperatorConsole moved to Display for integrated control

#include "ComputerSystem.h"
#include "CommunicationsSystem.h"

int main() {
    std::cout << "=== ATC Computer System (Achal & Parsa) ===" << std::endl;
    std::cout << "Note: Operator console now integrated with Display system" << std::endl;
    
    ComputerSystem computerSystem;
    // Task 4 (You need to first implement Task 3)
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
