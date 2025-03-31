// src/util/Settings.h
#pragma once
#include "../core/ChessPiece.h"
#include <QString>
#include <QSettings>

class Settings {
public:
    static Settings& getInstance();
    
    // Theme settings
    QString getCurrentTheme() const;
    void setCurrentTheme(const QString& theme);
    qreal getThemeScale(PieceColor color) const;
    
    // Sound settings
    bool isSoundEnabled() const;
    void setSoundEnabled(bool enabled);
    int getSoundVolume() const;
    void setSoundVolume(int volume);
    
    // Game settings
    int getTimeControl() const;
    void setTimeControl(int minutes);
    bool isAutoQueen() const;
    void setAutoQueen(bool enabled);
    
    // Network settings
    QString getLastServer() const;
    void setLastServer(const QString& server);
    int getLastPort() const;
    void setLastPort(int port);

    // Animation settings
    bool getAnimationsEnabled() const; 
    void setAnimationsEnabled(bool enabled);

    // Save/Load
    void saveSettings();
    void loadSettings(); // Load settings from storage

private:
    Settings();
    ~Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    
    // Default values
    static const QString DEFAULT_THEME;
    static const int DEFAULT_VOLUME;
    static const int DEFAULT_TIME_CONTROL;
    static const QString DEFAULT_SERVER;
    static const int DEFAULT_PORT;

    // Theme settings
    QString theme_;
    qreal whiteScale_;
    qreal blackScale_;
    
    // Sound settings
    bool soundEnabled_;
    int volume_;
    
    // Animation settings
    bool animationsEnabled_;

    // Time Control settings
    int timeControl_;
    bool autoQueen_;
    
    // Network settings
    QString lastServer_;
    int lastPort_;    

    // QSettings object to handle persistent storage
    QSettings settings_;
};