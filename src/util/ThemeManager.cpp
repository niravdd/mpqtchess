// src/util/ThemeManager.cpp
#include "ThemeManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QCoreApplication>

ThemeManager& ThemeManager::getInstance()
{
    static ThemeManager instance_;
    return instance_;
}

ThemeManager::ThemeManager() : currentThemeName_("classic")
{
    loadTheme(currentThemeName_);
}

bool ThemeManager::loadTheme(const QString& themeName)
{
    if (themeName.isEmpty()) {
        qWarning() << "Attempted to load empty theme name";
        return false;
    }

    // Load from cache if available
    if (themeCache_.contains(themeName)) {
        currentThemeName_ = themeName;
        currentTheme_ = themeCache_[themeName];
        emit themeChanged(themeName);
        return true;
    }

    // Load theme file
    QFile themeFile(QString(":/themes/%1.json").arg(themeName));
    if (!themeFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open theme file:" << themeFile.fileName();
        return false;
    }

    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(themeFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Theme JSON parse error:" << parseError.errorString();
        return false;
    }

    // Validate JSON structure
    QJsonObject themeJson = doc.object();
    if (!themeJson.contains("board") || !themeJson.contains("pieces")) {
        qWarning() << "Invalid theme JSON structure";
        return false;
    }

    // Parse theme config
    ThemeConfig config;
    try {
        // Parse board colors
        QJsonObject board = themeJson["board"].toObject();
        config.colors.lightSquares = QColor(board["lightSquares"].toString());
        config.colors.darkSquares = QColor(board["darkSquares"].toString());
        config.colors.border = QColor(board["border"].toString());
        config.colors.highlightMove = QColor(board["highlightMove"].toString());
        config.colors.highlightCheck = QColor(board["highlightCheck"].toString());

        // Parse piece configuration
        QJsonObject pieces = themeJson["pieces"].toObject();
        config.pieceSet = pieces["set"].toString();
        config.pieceScale = pieces["scale"].toDouble(1.0);
        
        // Validate scale range
        if (config.pieceScale < 0.5 || config.pieceScale > 2.0) {
            qWarning() << "Invalid piece scale, using default";
            config.pieceScale = 1.0;
        }
    } catch (const std::exception& e) {
        qCritical() << "Theme parse exception:" << e.what();
        return false;
    }

    // Update state
    themeCache_[themeName] = config;
    currentThemeName_ = themeName;
    currentTheme_ = config;
    
    emit themeChanged(themeName);
    return true;
}

QJsonObject ThemeManager::loadThemeFile(const QString& themeName)
{
    // Construct path to theme file - typically in resources
    QString themePath = QString("/assets/themes/%1.json").arg(themeName);
    
    // Open theme file
    QFile file(themePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open theme file:" << themePath;
        return QJsonObject();
    }
    
    // Read file content
    QByteArray themeData = file.readAll();
    file.close();
    
    // Parse JSON
    QJsonDocument document = QJsonDocument::fromJson(themeData);
    if (document.isNull() || !document.isObject()) {
        qWarning() << "Invalid theme file format:" << themePath;
        return QJsonObject();
    }
    
    // Store theme data
    themeData_ = document.object();
    
    // Parse and cache commonly used theme properties
    if (themeData_.contains("board")) {
        QJsonObject board = themeData_["board"].toObject();
        
        // Cache board colors
        lightSquareColor_ = QColor(board["lightSquares"].toString());
        darkSquareColor_ = QColor(board["darkSquares"].toString());
        highlightColor_ = QColor(board["highlight"].toString());
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
    
    return themeData_;
}

const ThemeManager::ThemeConfig& ThemeManager::getCurrentTheme() const
{
    return currentTheme_;
}

const QString& ThemeManager::getCurrentThemeName() const
{
    return currentThemeName_;
}

void ThemeManager::setCurrentTheme(const QString& themeName)
{
    loadTheme(themeName);
}

QStringList ThemeManager::getAvailableThemes() const
{
    // This would typically return a list of available theme names
    // You might want to implement this based on your resources or file system
    return {"classic", "minimalist", "modern"};
}