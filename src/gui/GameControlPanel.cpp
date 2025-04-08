// src/gui/GameControlPanel.cpp

#include "../util/Settings.h"
#include "GameControlPanel.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>


GameControlPanel::GameControlPanel(QWidget* parent)
    : QWidget(parent)
    , whiteTimeLeft_(1800)  // 30 minutes
    , blackTimeLeft_(1800)
    , isWhiteTurn_(true)
    , gameActive_(false)
    , soundEnabled_(true)
    , volume_(100)
{
    setupUI();
    
    gameTimer_ = new QTimer(this);
    connect(gameTimer_, &::QTimer::timeout, this, &GameControlPanel::updateClocks);
    
    soundPlayer_ = new QMediaPlayer(this);
    auto audioOutput = new QAudioOutput(this);
    soundPlayer_->setAudioOutput(audioOutput);
}

void GameControlPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Clocks group
    QGroupBox* clockGroup = new QGroupBox(tr("Time Control"));
    QVBoxLayout* clockLayout = new QVBoxLayout;
    
    whiteTimer_ = new QLCDNumber(this);
    whiteTimer_->setDigitCount(8);
    whiteTimer_->display("30:00");
    
    blackTimer_ = new QLCDNumber(this);
    blackTimer_->setDigitCount(8);
    blackTimer_->display("30:00");
    
    clockLayout->addWidget(new QLabel(tr("White")));
    clockLayout->addWidget(whiteTimer_);
    clockLayout->addWidget(new QLabel(tr("Black")));
    clockLayout->addWidget(blackTimer_);
    clockGroup->setLayout(clockLayout);
    
    // Control buttons
    QGroupBox* controlGroup = new QGroupBox(tr("Game Control"));
    QVBoxLayout* controlLayout = new QVBoxLayout;
    
    startBtn_ = new QPushButton(tr("Start"), this);
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    drawBtn_ = new QPushButton(tr("Offer Draw"), this);
    resignBtn_ = new QPushButton(tr("Resign"), this);
    turnLabel_ = new QLabel(tr("White to move"));
    
    connect(startBtn_, &::QPushButton::clicked, this, &GameControlPanel::startGame);
    connect(pauseBtn_, &::QPushButton::clicked, this, &GameControlPanel::pauseGame);
    connect(drawBtn_, &::QPushButton::clicked, this, &GameControlPanel::offerDraw);
    connect(resignBtn_, &::QPushButton::clicked, this, &GameControlPanel::resign);
    
    controlLayout->addWidget(startBtn_);
    controlLayout->addWidget(pauseBtn_);
    controlLayout->addWidget(drawBtn_);
    controlLayout->addWidget(resignBtn_);
    controlLayout->addWidget(turnLabel_);
    controlGroup->setLayout(controlLayout);
    
    mainLayout->addWidget(clockGroup);
    mainLayout->addWidget(controlGroup);
    mainLayout->addStretch();
    
    startBtn_->setEnabled(false);
    pauseBtn_->setEnabled(false);
    drawBtn_->setEnabled(false);
    resignBtn_->setEnabled(false);

    // Add settings controls
    QGroupBox* settingsGroup = new QGroupBox(tr("Preferences"));
    QVBoxLayout* settingsLayout = new QVBoxLayout;

    // Sound controls
    soundCheckBox_ = new QCheckBox(tr("Enable Sound"), this);
    volumeSlider_ = new QSlider(Qt::Horizontal, this);
    volumeSlider_->setRange(0, 100);
    
    // Animation controls
    animationCheckBox_ = new QCheckBox(tr("Enable Animations"), this);

    // Add to layout
    settingsLayout->addWidget(soundCheckBox_);
    settingsLayout->addWidget(new QLabel(tr("Volume:")));
    settingsLayout->addWidget(volumeSlider_);
    settingsLayout->addWidget(animationCheckBox_);
    settingsGroup->setLayout(settingsLayout);

    mainLayout->addWidget(settingsGroup);
    
    // Connect settings signals
    connect(soundCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        Settings::getInstance().setSoundEnabled(checked);
    });
    
    connect(volumeSlider_, &QSlider::valueChanged, this, [this](int value) {
        Settings::getInstance().setVolume(value);
    });
    
    connect(animationCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        Settings::getInstance().setAnimationsEnabled(checked);
    });
}

void GameControlPanel::updateClocks()
{
    if (isWhiteTurn_) {
        whiteTimeLeft_--;
    } else {
        blackTimeLeft_--;
    }
    
    // Update displays
    QString whiteTime = QString("%1:%2")
        .arg(whiteTimeLeft_ / 60, 2, 10, QChar('0'))
        .arg(whiteTimeLeft_ % 60, 2, 10, QChar('0'));
    whiteTimer_->display(whiteTime);
    
    QString blackTime = QString("%1:%2")
        .arg(blackTimeLeft_ / 60, 2, 10, QChar('0'))
        .arg(blackTimeLeft_ % 60, 2, 10, QChar('0'));
    blackTimer_->display(blackTime);
    
    // Check for time out
    if (whiteTimeLeft_ <= 0 || blackTimeLeft_ <= 0) {
        gameTimer_->stop();
        gameActive_ = false;
        startBtn_->setEnabled(true);
        pauseBtn_->setEnabled(false);
        
        // Play time-out sound
        if (soundEnabled_) {
            playSound(":/sounds/timeout.wav");
        }
    }
}

void GameControlPanel::startGame()
{
    gameActive_ = true;
    gameTimer_->start(1000);  // 1 second intervals
    startBtn_->setEnabled(false);
    pauseBtn_->setEnabled(true);
    drawBtn_->setEnabled(true);
    resignBtn_->setEnabled(true);
    
    if (soundEnabled_) {
        playSound(":/sounds/start.wav");
    }
    
    emit newGameRequested();
}

void GameControlPanel::pauseGame()
{
    if (gameActive_) {
        gameTimer_->stop();
        pauseBtn_->setText(tr("Resume"));
    } else {
        gameTimer_->start(1000);
        pauseBtn_->setText(tr("Pause"));
    }
    gameActive_ = !gameActive_;
}

void GameControlPanel::offerDraw()
{
    if (soundEnabled_) {
        playSound(":/sounds/draw_offer.wav");
    }
    emit drawOffered();
}

void GameControlPanel::resign()
{
    if (soundEnabled_) {
        playSound(":/sounds/resign.wav");
    }
    emit gameResigned();
}

void GameControlPanel::resetClock()
{
    gameTimer_->stop();
    whiteTimeLeft_ = 1800;
    blackTimeLeft_ = 1800;
    isWhiteTurn_ = true;
    gameActive_ = false;
    
    whiteTimer_->display("30:00");
    blackTimer_->display("30:00");
    
    startBtn_->setEnabled(true);
    pauseBtn_->setEnabled(false);
    drawBtn_->setEnabled(false);
    resignBtn_->setEnabled(false);
    turnLabel_->setText(tr("White to move"));
}

void GameControlPanel::setSoundSettings(bool enabled, int volume)
{
    soundEnabled_ = enabled;
    volume_ = volume;
    if (soundPlayer_) {
        soundPlayer_->audioOutput()->setVolume(volume / 100.0);
    }
}

void GameControlPanel::playSound(const QString& soundFile)
{
    if (soundEnabled_ && soundPlayer_) {
        soundPlayer_->setSource(QUrl::fromLocalFile(soundFile));
        soundPlayer_->audioOutput()->setVolume(volume_ / 100.0);
        soundPlayer_->play();
    }
}

void GameControlPanel::applySettings()
{
    // Get settings instance
    Settings& settings = Settings::getInstance();
    
    // Update sound settings
    soundCheckBox_->setChecked(settings.getSoundEnabled());
    
    // Update animation settings if applicable
    animationCheckBox_->setChecked(settings.getAnimationsEnabled());
    
    // Update volume slider if present
    if (volumeSlider_) {
        volumeSlider_->setValue(settings.getVolume());
    }
    
    // Update any theme-specific UI elements
    const QString& theme = settings.getCurrentTheme();
    
    // If your control panel has theme-dependent visuals, update them here
    // For example, update button styles or colors based on theme
    
    // Apply style sheets if needed
    if (theme == "dark") {
        setStyleSheet("GameControlPanel { background: #333; color: white; }");
    } else {
        setStyleSheet(""); // Use default style
    }
    
    // Refresh the panel
    update();
}

void GameControlPanel::swapTurn()
{
    isWhiteTurn_ = !isWhiteTurn_;
    turnLabel_->setText(isWhiteTurn_ ? tr("White to move") : tr("Black to move"));
}

void GameControlPanel::setTurn(bool isWhite)
{
    isWhiteTurn_ = isWhite;
    turnLabel_->setText(isWhiteTurn_ ? tr("White to move") : tr("Black to move"));
}

void GameControlPanel::setWhiteTime(int time)
{
    whiteTimeLeft_ = time;
    QString whiteTime = QString("%1:%2")
        .arg(whiteTimeLeft_ / 60, 2, 10, QChar('0'))
        .arg(whiteTimeLeft_ % 60, 2, 10, QChar('0'));
    whiteTimer_->display(whiteTime);
}

void GameControlPanel::setBlackTime(int time)
{
    blackTimeLeft_ = time;
    QString blackTime = QString("%1:%2")
        .arg(blackTimeLeft_ / 60, 2, 10, QChar('0'))
        .arg(blackTimeLeft_ % 60, 2, 10, QChar('0'));
    blackTimer_->display(blackTime);
}

void GameControlPanel::setGameActive(bool active)
{
    gameActive_ = active;
    startBtn_->setEnabled(!active);
    pauseBtn_->setEnabled(active);
    drawBtn_->setEnabled(active);
    resignBtn_->setEnabled(active);
}

void GameControlPanel::setGamePaused(bool paused)
{
    if (paused) {
        gameTimer_->stop();
        pauseBtn_->setText(tr("Resume"));
    } else {
        gameTimer_->start(1000);
        pauseBtn_->setText(tr("Pause"));
    }
    gameActive_ = !paused;
}

void GameControlPanel::setDrawOffered(bool offered)
{
    drawBtn_->setEnabled(!offered);
}

void GameControlPanel::setResignEnabled(bool enabled)
{
    resignBtn_->setEnabled(enabled);
}

void GameControlPanel::setStartEnabled(bool enabled)
{
    startBtn_->setEnabled(enabled);
}