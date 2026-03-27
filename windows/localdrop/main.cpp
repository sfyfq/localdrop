#include "hotkey.h"
#include <iostream>
#include <csignal>

void SignalHandler(int signum) {
    std::cout << "\nShutting down cleanly..." << std::endl;
    StopHook(); // This breaks the loop inside StartHook

}
void HandleHotkey(HotkeyAction action) {
    if (action == HotkeyAction::COPY) {
        std::cout << "Copying to peer..." << std::endl;
    }
    else {
        std::cout << "Pasting from peer..." << std::endl;
    }
}

int main() {
    // Register for Ctrl+C signal
    signal(SIGINT, SignalHandler);

    // Everything is encapsulated. main() just initializes the service.
    std::cout << "Service Started. Monitoring Hardware-Synced Hotkeys..." << std::endl;

    StartHook(HandleHotkey);
    std::cout << "Resources cleaned up. Goodbye." << std::endl;
    return 0;
    return 0;
}
