// src/util/SoundManager.h
#pragma once
#include <QtCore/QObject>              // QObject is core functionality
#include <QtMultimedia/QMediaPlayer>   // QMediaPlayer is multimedia
#include <QtMultimedia/QAudioOutput>   // QAudioOutput is multimedia
#include <QtCore/QHash>                // QHash is core functionality

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