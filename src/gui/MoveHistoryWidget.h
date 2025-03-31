// src/gui/MoveHistoryWidget.h
#pragma once
#include "../core/Position.h"
#include "../core/ChessGame.h"  // For ChessGame
#include "../core/ChessPiece.h" // For PieceType enum
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QScrollBar>
#include <QtCore/QString>

class MoveHistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit MoveHistoryWidget(QWidget* parent = nullptr);
    ~MoveHistoryWidget();

    void clear();
    void loadFromGame(const ChessGame* game);
    void scrollToBottom();

public slots:
    void addMove(const QString& move);

private:
    QVBoxLayout* layout_;
    QTextEdit* historyText_;
    int moveNumber_;
    bool isWhiteMove_;
    
    void formatMove(const QString& move);
    QString getPieceSymbol(PieceType piece);
};