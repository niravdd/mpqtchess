// src/gui/ChessBoardView.h
#pragma once
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include "ChessPieceItem.h"
#include "../core/ChessGame.h"
#include "../core/Position.h"

// Helper function to generate chess notation (e.g., "e2-e4")
QString generateMoveNotation(const Position& from, const Position& to) {
    char fromFile = 'a' + from.col;
    char toFile = 'a' + to.col;
    int fromRank = 8 - from.row;
    int toRank = 8 - to.row;
    
    return QString("%1%2-%3%4")
        .arg(fromFile)
        .arg(fromRank)
        .arg(toFile)
        .arg(toRank);
}

class ChessBoardView : public QGraphicsView {
    Q_OBJECT

public:
    explicit ChessBoardView(QWidget* parent = nullptr);
    void updateTheme();
    void resetGame();
    bool connectToServer(const QString& address, quint16 port);
    bool saveGame(const QString& fileName);
    bool loadGame(const QString& fileName);
    ChessGame* getGame() const { return game_.get(); }
    void applySettings();
    void setTheme(const QString& theme);
    QString getCurrentTheme() const;

signals:
    void moveCompleted(const QString& move);
    void gameOver(const QString& result);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void setupBoard();
    void updateBoard();
    void highlightLegalMoves(const QPoint& pos);
    QPoint boardPositionAt(const QPointF& pos) const;
    void clearHighlights();
    
    QGraphicsScene* scene_;
    std::unique_ptr<ChessGame> game_;
    ChessPieceItem* selectedPiece_;
    QPointF dragStartPos_;
    QGraphicsRectItem* highlightItems_[8][8];
};