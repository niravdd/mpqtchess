// client/main.cpp
#include <QApplication>
#include <QResource>
#include <QProcess>  // For process ID
#include <QDebug>
#include "../src/gui/MainWindow.h"
#include "../src/util/ThemeManager.h"
#include "../src/util/SoundManager.h"

void setupLogging() {
    // Generate a unique filename using timestamp and process ID
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString processId = QString::number(QCoreApplication::applicationPid());
    QString logFilename = QString("chess_client_log_%1_%2.txt")
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
    QApplication app(argc, argv);

    // Print out the Process ID
    qint64 pid = QCoreApplication::applicationPid();
    qDebug() << "Application Started - Process ID:" << pid;

    setupLogging();

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