// src/util/Settings.cpp
#include "Settings.h"

const QString Settings::DEFAULT_THEME = "classic";
const int Settings::DEFAULT_VOLUME = 100;
const int Settings::DEFAULT_TIME_CONTROL = 30;
const QString Settings::DEFAULT_SERVER = "localhost";
const int Settings::DEFAULT_PORT = 12345;

Settings& Settings::getInstance()
{
    static Settings instance;
    return instance;
}

Settings::Settings()
    : theme_(Settings::DEFAULT_THEME)
    , whiteScale_(1.0)
    , blackScale_(1.0)
    , soundEnabled_(true)
    , volume_(Settings::DEFAULT_VOLUME)
    , animationsEnabled_(true)
    , timeControl_(Settings::DEFAULT_TIME_CONTROL)
    , autoQueen_(true)
    , lastServer_(Settings::DEFAULT_SERVER)
    , lastPort_(Settings::DEFAULT_PORT)
    , settings_("MultiPlayer Qt Chess", "ChessGame")
{
    loadSettings();
}

void Settings::loadSettings()
{
    // Load theme settings
    theme_ = settings_.value("theme", DEFAULT_THEME).toString();
    whiteScale_ = settings_.value("theme/whiteScale", 1.0).toDouble();
    blackScale_ = settings_.value("theme/blackScale", 1.0).toDouble();
    
    // Load sound settings
    soundEnabled_ = settings_.value("sound/enabled", true).toBool();
    volume_ = settings_.value("sound/volume", DEFAULT_VOLUME).toInt();
    
    // Load animation settings
    animationsEnabled_ = settings_.value("animations/enabled", true).toBool();
    
    // Load game settings
    timeControl_ = settings_.value("game/timeControl", DEFAULT_TIME_CONTROL).toInt();
    autoQueen_ = settings_.value("game/autoQueen", true).toBool();
    
    // Load network settings
    lastServer_ = settings_.value("network/lastServer", DEFAULT_SERVER).toString();
    lastPort_ = settings_.value("network/lastPort", DEFAULT_PORT).toInt();
}

void Settings::saveSettings()
{
    // Save theme settings
    settings_.setValue("theme", theme_);
    settings_.setValue("theme/whiteScale", whiteScale_);
    settings_.setValue("theme/blackScale", blackScale_);
    
    // Save sound settings
    settings_.setValue("sound/enabled", soundEnabled_);
    settings_.setValue("sound/volume", volume_);
    
    // Save animation settings
    settings_.setValue("animations/enabled", animationsEnabled_);
    
    // Save game settings
    settings_.setValue("game/timeControl", timeControl_);
    settings_.setValue("game/autoQueen", autoQueen_);
    
    // Save network settings
    settings_.setValue("network/lastServer", lastServer_);
    settings_.setValue("network/lastPort", lastPort_);
    
    // Ensure settings are written to disk
    settings_.sync();
}

QString Settings::getCurrentTheme() const
{
    return theme_;
}

void Settings::setCurrentTheme(const QString& theme)
{
    theme_ = theme;
}

bool Settings::isSoundEnabled() const
{
    return soundEnabled_;
}

void Settings::setSoundEnabled(bool enabled)
{
    soundEnabled_ = enabled;
}

int Settings::getSoundVolume() const
{
    return volume_;
}

void Settings::setSoundVolume(int volume)
{
    volume_ = volume;
}

bool Settings::getAnimationsEnabled() const
{
    return animationsEnabled_;
}

void Settings::setAnimationsEnabled(bool enabled)
{
    animationsEnabled_ = enabled;
}

int Settings::getTimeControl() const
{
    return timeControl_;
}

void Settings::setTimeControl(int minutes)
{
    timeControl_ = minutes;
}

bool Settings::isAutoQueen() const
{
    return autoQueen_;
}

void Settings::setAutoQueen(bool enabled)
{
    autoQueen_ = enabled;
}

QString Settings::getLastServer() const
{
    return lastServer_;
}

void Settings::setLastServer(const QString& server)
{
    lastServer_ = server;
}

int Settings::getLastPort() const
{
    return lastPort_;
}

void Settings::setLastPort(int port)
{
    lastPort_ = port;
}

qreal Settings::getThemeScale(PieceColor color) const 
{
    return (color == PieceColor::White) ? whiteScale_ : blackScale_;
}