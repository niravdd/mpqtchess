// server/main.cpp
#include <QCoreApplication>
#include <QCommandLineParser>
#include "../src/network/ChessServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Chess Game Server");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add port option
    QCommandLineOption portOption(QStringList() << "p" << "port",
        "Port to listen on.",
        "port",
        "12345");
    parser.addOption(portOption);
    
    parser.process(app);
    
    // Get port number
    bool ok;
    quint16 port = parser.value(portOption).toUShort(&ok);
    if (!ok) {
        qDebug() << "Invalid port number";
        return 1;
    }
    
    // Create and start server
    ChessNetworkServer server;
    if (!server.start(port)) {
        return 1;
    }
    
    qDebug() << "Server started on port" << port;
    
    return app.exec();
}