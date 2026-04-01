#ifndef SRC_COMMUNICATIONSSYSTEM_H_
#define SRC_COMMUNICATIONSSYSTEM_H_

// Achal & Parsa - CommunicationsSystem
// not used directly - ComputerSystem.forwardToAircraft() handles this instead

#include <iostream>
#include <sys/dispatch.h>
#include "Msg_structs.h"

class CommunicationsSystem {
public:
    CommunicationsSystem() = default;
    ~CommunicationsSystem() = default;
    
    // Send message to specific aircraft
    static bool messageAircraft(int planeID, const Message_inter_process& msg);
};

#endif /* SRC_COMMUNICATIONSSYSTEM_H_ */
