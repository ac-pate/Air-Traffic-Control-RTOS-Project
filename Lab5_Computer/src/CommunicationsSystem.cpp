// Achal & Parsa - CommunicationsSystem

#include "CommunicationsSystem.h"

bool CommunicationsSystem::messageAircraft(int planeID, const Message_inter_process& msg) {
    std::string channelName = getAircraftChannel(planeID);
    int aircraftCoid = name_open(channelName.c_str(), 0);
    if (aircraftCoid == -1) {
        std::cerr << "CommunicationsSystem: Cannot reach aircraft " << planeID << std::endl;
        return false;
    }
    
    int reply;
    int result = MsgSend(aircraftCoid, &msg, sizeof(msg), &reply, sizeof(reply));
    name_close(aircraftCoid);
    
    return (result != -1 && reply == 0);
}
