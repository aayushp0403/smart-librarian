#include "core/Application.h"
#include <iostream>

namespace sl {
namespace core {

Application::Application(int argc, char* argv[])
    : m_argc(argc)
    , m_argv(argv)
{
    std::cout << "[Application] Constructed. "
              << "argc=" << m_argc << "\n";
}

Application::~Application()
{
    std::cout << "[Application] Destructor called. "
              << "Subsystems shutting down.\n";
}

int Application::run()
{
    std::cout << "[Application] Running SmartLibrarian v0.1.0\n";
    std::cout << "[Application] Build pipeline verified.\n";
    std::cout << "[Application] Ready for module integration.\n";
    return 0;
}

} // namespace core
} // namespace sl