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
