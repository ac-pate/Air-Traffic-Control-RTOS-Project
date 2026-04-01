#ifndef OPERATORCONSOLE_H_
#define OPERATORCONSOLE_H_

// Achal & Parsa - operator console for aircraft commands

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
