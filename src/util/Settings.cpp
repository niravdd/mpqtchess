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
    : settings_("ChessGame", "ChessApp")
{
}

QString Settings::getCurrentTheme() const
{
    return settings_.value("theme", DEFAULT_THEME).toString();
}

void Settings::setCurrentTheme(const QString& theme)
{
    settings_.setValue("theme", theme);
}

bool Settings::isSoundEnabled() const
{
    return settings_.value("sound/enabled", true).toBool();
}

void Settings::setSoundEnabled(bool enabled)
{
    settings_.setValue("sound/enabled", enabled);
}

int Settings::getSoundVolume() const
{
    return settings_.value("sound/volume", DEFAULT_VOLUME).toInt();
}

void Settings::setSoundVolume(int volume)
{
    settings_.setValue("sound/volume", volume);
}

int Settings::getTimeControl() const
{
    return settings_.value("game/timeControl", DEFAULT_TIME_CONTROL).toInt();
}

void Settings::setTimeControl(int minutes)
{
    settings_.setValue("game/timeControl", minutes);
}

bool Settings::isAutoQueen() const
{
    return settings_.value("game/autoQueen", true).toBool();
}

void Settings::setAutoQueen(bool enabled)
{
    settings_.setValue("game/autoQueen", enabled);
}

QString Settings::getLastServer() const
{
    return settings_.value("network/lastServer", DEFAULT_SERVER).toString();
}

void Settings::setLastServer(const QString& server)
{
    settings_.setValue("network/lastServer", server);
}

int Settings::getLastPort() const
{
    return settings_.value("network/lastPort", DEFAULT_PORT).toInt();
}

void Settings::setLastPort(int port)
{
    settings_.setValue("network/lastPort", port);
}