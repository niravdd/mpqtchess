// src/util/ThemeManager.h
#pragma once
#include <QObject>
#include <QHash>
#include <QtGui/QColor>
#include <QJsonObject>

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& getInstance();

    struct ThemeColors {
        QColor lightSquares;
        QColor darkSquares;
        QColor border;
        QColor highlightMove;
        QColor highlightCheck;
    };

    struct ThemeConfig {
        ThemeColors colors;
        QString pieceSet;
        qreal pieceScale;
        QString fontFamily;
        int fontSize;
    };

    const ThemeConfig& getCurrentTheme() const;
    void setCurrentTheme(const QString& themeName);
    QStringList getAvailableThemes() const;

signals:
    void themeChanged();

private:
    ThemeManager();
    void loadTheme(const QString& themeName);
    QJsonObject loadThemeFile(const QString& themeName);

    QHash<QString, ThemeConfig> themeCache_;
    QString currentThemeName_;
    ThemeConfig currentTheme_;
};