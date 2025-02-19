// src/gui/MoveHistoryWidget.h
#pragma once
#include "../core/Position.h"
#include <QWidget>
#include <QTextEdit>
#include <QString>

class MoveHistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit MoveHistoryWidget(QWidget* parent = nullptr);
    void clear();
    void loadFromGame(const class ChessGame* game);

public slots:
    void addMove(const QString& move);

private:
    QTextEdit* historyText_;
    int moveNumber_;
    bool isWhiteMove_;
    
    void formatMove(const QString& move);
};