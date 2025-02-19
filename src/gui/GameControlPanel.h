// src/gui/GameControlPanel.h
#pragma once
#include "../core/Position.h"
#include <QWidget>
#include <QLCDNumber>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QMediaPlayer>

class GameControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit GameControlPanel(QWidget* parent = nullptr);
    
    void resetClock();
    void setSoundSettings(bool enabled, int volume);
    bool isSoundEnabled() const { return soundEnabled_; }
    int getVolume() const { return volume_; }
    
public slots:
    void applySettings();
    void startGame();
    void pauseGame();
    void offerDraw();
    void resign();

signals:
    void newGameRequested();
    void drawOffered();
    void gameResigned();

private:
    void setupUI();
    void updateClocks();
    void playSound(const QString& soundFile);

    // UI elements
    QLCDNumber* whiteTimer_;
    QLCDNumber* blackTimer_;
    QPushButton* startBtn_;
    QPushButton* pauseBtn_;
    QPushButton* drawBtn_;
    QPushButton* resignBtn_;
    QLabel* turnLabel_;
    
    // Game control
    QTimer* gameTimer_;
    int whiteTimeLeft_;
    int blackTimeLeft_;
    bool isWhiteTurn_;
    bool gameActive_;
    
    // Sound settings
    bool soundEnabled_;
    int volume_;
    QMediaPlayer* soundPlayer_;
};