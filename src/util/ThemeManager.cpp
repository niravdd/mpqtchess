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