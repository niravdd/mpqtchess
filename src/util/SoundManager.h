// src/util/SoundManager.h
#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QHash>

class SoundManager : public QObject {
    Q_OBJECT

public:
    enum SoundEffect {
        Move,
        Capture,
        Check,
        Checkmate,
        Start,
        DrawOffer,
        TimeOut,
        Resign
    };
    Q_ENUM(SoundEffect)

    static SoundManager& getInstance();
    
    void playSound(SoundEffect effect);
    void setEnabled(bool enabled);
    void setVolume(int volume);

private:
    SoundManager();
    
    QHash<SoundEffect, QString> soundFiles_;
    QMediaPlayer* player_;
    QAudioOutput* audioOutput_;
    bool enabled_;
    int volume_;
};