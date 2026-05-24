/**
 * SmartLibrarian — Phase 4 Entry Point
 *
 * Launches the Qt6 desktop application.
 *
 * main() is intentionally minimal.
 * All application logic lives in MainWindow.
 */

#include <QApplication>
#include "gui/MainWindow.h"

int main(int argc, char* argv[])
{
    // QApplication must be constructed before any Qt objects.
    // It initializes the event loop, font system, and platform backend.
    QApplication app(argc, argv);

    // Application metadata (used by OS and native dialogs)
    QApplication::setApplicationName("SmartLibrarian");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("SmartLibrarian Project");

    // Global stylesheet — applies to all widgets
    app.setStyleSheet(
        "QWidget { font-family: 'Segoe UI', 'DejaVu Sans', sans-serif; }"
        "QMainWindow { background: #FAFAFA; }"
    );

    sl::gui::MainWindow window;
    window.show();

    // exec() starts the event loop — blocks here until window is closed.
    // Returns 0 on normal exit.
    return app.exec();
}
