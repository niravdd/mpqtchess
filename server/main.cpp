// server/main.cpp
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "../src/network/ChessServer.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("Multiplayer Chess Server");
    QCoreApplication::setApplicationVersion("1.0");

    // Command line parser setup
    QCommandLineParser parser;
    parser.setApplicationDescription("Multiplayer Chess Server");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add port option
    QCommandLineOption portOption(QStringList() << "p" << "port",
        QCoreApplication::translate("main", "Port to listen on [default: 12345]."),
        "port", "12345");
    parser.addOption(portOption);

    // Process the command line arguments
    parser.process(app);

    // Get the port number
    bool ok;
    quint16 port = parser.value(portOption).toUShort(&ok);
    if (!ok) {
        std::cerr << "Invalid port number specified" << std::endl;
        return 1;
    }

    // Create and start the server
    ChessNetworkServer server;
    if (!server.start(port)) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return 1;
    }

    std::cout << "Chess server started on port " << port << std::endl;
    std::cout << "Press Ctrl+C to quit" << std::endl;

    return app.exec();
}



// // server/main.cpp
// #include <QCoreApplication>
// #include <QCommandLineParser>
// #include "../src/network/ChessServer.h"

// int main(int argc, char *argv[])
// {
//     QCoreApplication app(argc, argv);
    
//     // Set up command line parser
//     QCommandLineParser parser;
//     parser.setApplicationDescription("Chess Game Server");
//     parser.addHelpOption();
//     parser.addVersionOption();
    
//     // Add port option
//     QCommandLineOption portOption(QStringList() << "p" << "port",
//         "Port to listen on.",
//         "port",
//         "12345");
//     parser.addOption(portOption);
    
//     parser.process(app);
    
//     // Get port number
//     bool ok;
//     quint16 port = parser.value(portOption).toUShort(&ok);
//     if (!ok) {
//         qDebug() << "Invalid port number";
//         return 1;
//     }
    
//     // Create and start server
//     ChessNetworkServer server;
//     if (!server.start(port)) {
//         return 1;
//     }
    
//     qDebug() << "Server started on port" << port;
    
//     return app.exec();
// }