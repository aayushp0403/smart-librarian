/**
 * SmartLibrarian — Intelligent Document Archival & PDF Pipeline
 *
 * Entry point. At this stage this simply verifies the build
 * infrastructure is working correctly. Each module will be
 * wired in as we build them.
 *
 * Architecture note:
 *   main() should be thin. It initializes subsystems and
 *   hands control to the application class. Business logic
 *   never lives here.
 */

#include <iostream>
#include "core/Application.h"

int main(int argc, char* argv[])
{
    std::cout << "[SmartLibrarian] Initializing...\n";

    sl::core::Application app(argc, argv);
    return app.run();
}