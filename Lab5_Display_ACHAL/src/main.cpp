// Achal & Parsa - ATC Display System

#include "DisplaySystem.h"
#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "=== ATC Display System (Achal & Parsa) ===" << std::endl;
    
    DisplaySystem display;
    
    if (!display.start()) {
        std::cerr << "Display: Failed to start" << std::endl;
        return EXIT_FAILURE;
    }
    
    display.wait();
    
    std::cout << "Display stopped. Exiting." << std::endl;
    return EXIT_SUCCESS;
}
