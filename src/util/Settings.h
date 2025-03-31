// src/util/Settings.h
#pragma once
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

private:
    Settings();
    ~Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    
    QSettings settings_;
    
    // Default values
    static const QString DEFAULT_THEME;
    static const int DEFAULT_VOLUME;
    static const int DEFAULT_TIME_CONTROL;
    static const QString DEFAULT_SERVER;
    static const int DEFAULT_PORT;
};
