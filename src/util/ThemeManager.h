// src/util/ThemeManager.h
#pragma once
#include <QObject>
#include <QHash>
#include <QtGui/QColor>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class ThemeManager : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(QObject *parent = nullptr);
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

    bool loadTheme(const QString& themeName);
    const ThemeConfig& getCurrentTheme() const;
    void setCurrentTheme(const QString& themeName);
    QStringList getAvailableThemes() const;
    const QString& getCurrentThemeName() const;

signals:
    void themeChanged(const QString &themeName);

private:
    ThemeManager();
    QJsonObject loadThemeFile(const QString& themeName);

    QHash<QString, ThemeConfig> themeCache_;
    QString currentThemeName_;
    ThemeConfig currentTheme_;
    QJsonObject themeData_;
    
    // Additional cached color and style properties
    QColor lightSquareColor_;
    QColor darkSquareColor_;
    QColor highlightColor_;
    QString pieceStyle_;
    double whiteScale_ = 1.0;
    double blackScale_ = 1.0;
};