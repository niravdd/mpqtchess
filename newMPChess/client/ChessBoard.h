// ChessBoard.h
#pragma once

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QString>
#include <QPoint>
#include <QPixmap>
#include <QPair>
#include <QStringList>

class ChessBoard : public QWidget {
    Q_OBJECT

public:
    explicit ChessBoard(QWidget* parent = nullptr);
    
    void resetBoard();
    void setPosition(const QString& fen);
    void setPossibleMoves(const QStringList& moves);
    void setRecommendedMoves(const QStringList& moves);
    void clearRecommendedMoves();
    void setRotated(bool rotated);
    
signals:
    void moveMade(const QString& from, const QString& to);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    
private:
    struct Piece {
        char type;  // 'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'
        int x, y;
    };
    
    QVector<QVector<char>> board;  // 8x8 board representation
    QVector<Piece> pieces;         // All pieces on the board
    QStringList possibleMoves;     // List of valid moves in algebraic notation (e.g., "e2e4")
    QStringList recommendedMoves;  // List of recommended moves
    
    QPoint selectedSquare;         // Currently selected square (-1, -1) if none
    QPoint draggedPiece;           // Piece being dragged (-1, -1) if none
    QPoint dragPosition;           // Current drag position
    
    bool isRotated;                // Whether board is rotated (black at bottom)
    
    // Piece images
    QMap<char, QPixmap> pieceImages;
    
    // Helper functions
    void loadPieceImages();
    QRect squareRect(int x, int y) const;
    QPair<int, int> squareAtPosition(const QPoint& pos) const;
    QString squareToAlgebraic(int x, int y) const;
    QPair<int, int> algebraicToSquare(const QString& algebraic) const;
    bool isValidMove(const QString& from, const QString& to) const;
};