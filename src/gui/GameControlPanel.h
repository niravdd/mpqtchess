// src/gui/GameControlPanel.h
#pragma once
#include "../core/Position.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QPushButton>  // QPushButton is a widget
#include <QtWidgets/QLabel>       // QLabel is a widget
#include <QtCore/QTimer>            // QTimer is a core functionality
#include <QtMultimedia/QMediaPlayer>  // QMediaPlayer is in multimedia module
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QSlider>
#include <QtMultimedia/QAudioOutput>

class GameControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit GameControlPanel(QWidget* parent = nullptr);
    
    void resetClock();
    void setSoundSettings(bool enabled, int volume);
    bool isSoundEnabled() const { return soundEnabled_; }
    int  getVolume() const { return volume_; }
    void swapTurn();
    void setTurn(bool isWhite);
    void setWhiteTime(int time);
    void setBlackTime(int time);
    void setStartEnabled(bool enabled);
    void setGameActive(bool active);
    void setGamePaused(bool paused);
    void setDrawOffered(bool offered);
    void setResignEnabled(bool enabled);
    QPushButton* controlStartButton() { return startBtn_; }

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
    QCheckBox* soundCheckBox_ = nullptr;
    QCheckBox* animationCheckBox_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
};