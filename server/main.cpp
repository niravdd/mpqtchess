// server/main.cpp
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "../src/network/ChessServer.h"
#include <QProcess>  // For process ID
#include <QDebug>
#include <iostream>

void setupLogging() {
    // Generate a unique filename using timestamp and process ID
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString processId = QString::number(QCoreApplication::applicationPid());
    QString logFilename = QString("chess_server_log_%1_%2.txt")
        .arg(timestamp)
        .arg(processId);

    // Ensure log directory exists
    QDir logDir(QCoreApplication::applicationDirPath() + "/logs");
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }

    QString fullLogPath = logDir.filePath(logFilename);
    
    // Use a static pointer to ensure log file persists
    static QFile* logFile = new QFile(fullLogPath);
    
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg)
        {
            static QFile* staticLogFile = logFile;
            static QTextStream out(staticLogFile);
            
            // Determine log level string
            QString levelString;
            switch (type) {
                case QtDebugMsg:    levelString = "DEBUG"; break;
                case QtWarningMsg:  levelString = "WARNING"; break;
                case QtCriticalMsg: levelString = "CRITICAL"; break;
                case QtFatalMsg:    levelString = "FATAL"; break;
                default:             levelString = "INFO"; break;
            }
            
            // Log format: [TIMESTAMP] [LEVEL] [FILE:LINE] Message
            out << QDateTime::currentDateTime().toString(Qt::ISODate)
                << " [" << levelString << "] "
                << "[" << context.file << ":" << context.line << "] "
                << msg << "\n";
            out.flush();

            // Use qDebug() instead of std::cout
            qDebug().noquote() << msg;
        });

        // Log initial setup information
        qDebug() << "Logging initialized to file:" << fullLogPath;
    }
    else
    {
        // Use qDebug() for error reporting
        qDebug() << "Could not create log file:" << fullLogPath;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Print out the Process ID
    qint64 pid = QCoreApplication::applicationPid();
    qDebug() << "Application Started - Process ID:" << pid;

    setupLogging();

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