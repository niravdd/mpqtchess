// src/util/ThemeManager.cpp
#include "ThemeManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

ThemeManager& ThemeManager::getInstance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() : currentThemeName_("classic")
{
    loadTheme(currentThemeName_);
}

void ThemeManager::loadTheme(const QString& themeName)
{
    if (!themeCache_.contains(themeName)) {
        QJsonObject themeJson = loadThemeFile(themeName);
        ThemeConfig config;
        
        auto board = themeJson["board"].toObject();
        config.colors.lightSquares = QColor(board["lightSquares"].toString());
        config.colors.darkSquares = QColor(board["darkSquares"].toString());
        config.colors.border = QColor(board["border"].toString());
        config.colors.highlightMove = QColor(board["highlightMove"].toString());
        config.colors.highlightCheck = QColor(board["highlightCheck"].toString());
        
        auto pieces = themeJson["pieces"].toObject();
        config.pieceSet = pieces["set"].toString();
        config.pieceScale = pieces["scale"].toDouble();
        
        themeCache_[themeName] = config;
    }
    
    currentThemeName_ = themeName;
    currentTheme_ = themeCache_[themeName];
    emit themeChanged();
}

bool ThemeManager::loadThemeFile(const QString& themeName)
{
    // Construct path to theme file - typically in resources
    QString themePath = QString(":/themes/%1.json").arg(themeName);
    
    // Open theme file
    QFile file(themePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open theme file:" << themePath;
        return false;
    }
    
    // Read file content
    QByteArray themeData = file.readAll();
    file.close();
    
    // Parse JSON
    QJsonDocument document = QJsonDocument::fromJson(themeData);
    if (document.isNull() || !document.isObject()) {
        qWarning() << "Invalid theme file format:" << themePath;
        return false;
    }
    
    // Store theme data
    themeData_ = document.object();
    currentThemeName_ = themeName;
    
    // Parse and cache commonly used theme properties for faster access
    if (themeData_.contains("board")) {
        QJsonObject board = themeData_["board"].toObject();
        
        // Cache board colors
        if (board.contains("lightSquares")) {
            lightSquareColor_ = QColor(board["lightSquares"].toString());
        }
        if (board.contains("darkSquares")) {
            darkSquareColor_ = QColor(board["darkSquares"].toString());
        }
        if (board.contains("highlight")) {
            highlightColor_ = QColor(board["highlight"].toString());
        }
    }
    
    // Parse piece style information if available
    if (themeData_.contains("pieces")) {
        QJsonObject pieces = themeData_["pieces"].toObject();
        
        // Cache piece style info
        pieceStyle_ = pieces.value("style").toString("default");
        
        // Cache scale factors if present
        whiteScale_ = pieces.value("whiteScale").toDouble(1.0);
        blackScale_ = pieces.value("blackScale").toDouble(1.0);
    }
    
    // Emit theme changed notification
    emit themeChanged(themeName);
    
    return true;
}

const QString& ThemeManager::getCurrentTheme() const {
    return currentThemeName_;
}