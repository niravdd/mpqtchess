// src/core/ChessBoard.h
#pragma once
#include "ChessPiece.h"
#include "Position.h"
#include <array>
#include <memory>
#include <vector>

class ChessBoard {
public:
    ChessBoard();
    
    std::shared_ptr<ChessPiece> getPieceAt(const QPoint& pos) const;
    bool movePiece(const QPoint& from, const QPoint& to);
    std::vector<QPoint> getLegalMoves(const QPoint& pos) const;
    bool isInCheck(PieceColor color) const;
    bool isCheckmate(PieceColor color) const;
    bool canCastleKingside(PieceColor color) const;
    bool canCastleQueenside(PieceColor color) const;
    bool removePiece(const Position& pos);
    Position getWhiteKingPosition() const { return whiteKingPos_; }
    Position getBlackKingPosition() const { return blackKingPos_; }
    void setWhiteKingPosition(const Position& pos) { whiteKingPos_ = pos; }
    void setBlackKingPosition(const Position& pos) { blackKingPos_ = pos; }
    
private:
    std::array<std::array<std::shared_ptr<ChessPiece>, 8>, 8> board_;
    QPoint whiteKingPos_;
    QPoint blackKingPos_;
    QPoint lastPawnDoubleMove_;
    
    void initializeBoard();
    bool isValidPosition(const QPoint& pos) const;
    bool isPathClear(const QPoint& from, const QPoint& to) const;
};