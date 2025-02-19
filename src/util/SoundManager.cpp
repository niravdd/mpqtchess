// src/util/SoundManager.cpp
#include "SoundManager.h"

SoundManager& SoundManager::getInstance()
{
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager() : enabled_(true), volume_(100)
{
    player_ = new QMediaPlayer(this);
    audioOutput_ = new QAudioOutput(this);
    player_->setAudioOutput(audioOutput_);
    
    // Initialize sound files map
    soundFiles_[Move] = ":/sounds/move.wav";
    soundFiles_[Capture] = ":/sounds/capture.wav";
    soundFiles_[Check] = ":/sounds/check.wav";
    soundFiles_[Checkmate] = ":/sounds/checkmate.wav";
    soundFiles_[Start] = ":/sounds/start.wav";
    soundFiles_[DrawOffer] = ":/sounds/draw_offer.wav";
    soundFiles_[TimeOut] = ":/sounds/timeout.wav";
    soundFiles_[Resign] = ":/sounds/resign.wav";
}

void SoundManager::playSound(SoundEffect effect)
{
    if (!enabled_ || !soundFiles_.contains(effect))
        return;
        
    player_->setSource(QUrl(soundFiles_[effect]));
    audioOutput_->setVolume(volume_ / 100.0);
    player_->play();
}