#pragma once

// Ensure we include the actual Qt headers before any forward declarations
#include <QtCore/QtCore>          // All core functionality
#include <QtGui/QtGui>            // GUI painting, palettes
#include <QtWidgets/QtWidgets>    // Widget classes
#include <QtNetwork/QtNetwork>    // Networking classes
#include <QtMultimedia/QtMultimedia>
#include <QtWidgets/QApplication>  // For QApplication and general widgets
#include <QtGui/QPainter>          // For painting functionality
#include <QtGui/QPalette>          // For palette management

// Remove any forward declarations that might conflict with Qt's types
// Do NOT forward declare built-in Qt types like QString

// Qt6 Compatibility Macros
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #define QSTRING_COMPAT(x) QString::fromUtf8(x)
#else
    #define QSTRING_COMPAT(x) QString(x)
#endif

// Only forward declare your custom types, not Qt types
class ChessGame;
class ChessBoard;

// Optional: Ensure full inclusion of necessary headers
#include <QString>
#include <QObject>
#include <QtMultimedia/QMediaPlayer>
