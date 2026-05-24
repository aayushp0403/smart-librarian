#pragma once

/**
 * Application.h
 *
 * The top-level application class.
 *
 * Design principles:
 *   - Owns the lifetime of all subsystems (RAII)
 *   - Constructed once in main()
 *   - run() is the event loop entry point
 *   - Destructor cleanly tears down all subsystems
 *
 * The 'sl' namespace = SmartLibrarian.
 * All our code lives in this namespace to avoid
 * collisions with third-party libraries.
 */

namespace sl {
namespace core {

class Application
{
public:
    /**
     * Constructor.
     * argc/argv passed through from main() for future
     * command-line argument parsing.
     */
    Application(int argc, char* argv[]);

    /**
     * Destructor.
     * RAII: all subsystems are destroyed in reverse
     * order of construction here, deterministically.
     */
    ~Application();

    // Delete copy — an Application is unique, cannot be copied.
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    // Allow move (though in practice we won't move it).
    Application(Application&&)            = default;
    Application& operator=(Application&&) = default;

    /**
     * run() — enters the main application loop.
     * Returns exit code (0 = success).
     */
    int run();

private:
    int   m_argc;
    char** m_argv;
};

} // namespace core
} // namespace sl