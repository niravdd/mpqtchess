// client/main.cpp
#include <QApplication>
#include <QResource>
#include "../src/gui/MainWindow.h"
#include "../src/util/ThemeManager.h"
#include "../src/util/SoundManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Initialize resources
    Q_INIT_RESOURCE(pieces);
    Q_INIT_RESOURCE(sounds);
    Q_INIT_RESOURCE(themes);
    
    // Set application information
    QCoreApplication::setOrganizationName("Multiplayer Chess v1.00");
    QCoreApplication::setApplicationName("MPChess");
    QCoreApplication::setApplicationVersion("1.0");
    
    // Initialize managers
    ThemeManager::getInstance();
    SoundManager::getInstance();
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}