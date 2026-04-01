// Achal & Parsa - OperatorConsole
// parses user commands and sends to ComputerSystem via IPC

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
            heading.altitude = -1;  // Don't change altitude
            
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
        else if (cmd == "info") {
            int planeID;
            if (!(iss >> planeID)) {
                std::cout << "Usage: info <planeID>" << std::endl;
                continue;
            }
            
            msg.type = MessageType::REQUEST_AUGMENTED_INFO;
            msg.planeID = planeID;
            msg.dataSize = 0;
            
            sendCommand(msg);
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
