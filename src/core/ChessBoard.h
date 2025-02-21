// src/core/ChessBoard.h
#pragma once
#include "ChessPiece.h"
#include "Position.h"
#include <array>
#include <memory>
#include <vector>

class ChessBoard {

    friend class ChessGame;

public:
    ChessBoard();
    
    std::shared_ptr<ChessPiece> getPieceAt(const Position& pos) const;
    std::vector<Position> getLegalMoves(const Position& pos) const;
    bool movePiece(const Position& from, const Position& to, PieceType promotionPiece = PieceType::Queen);
    std::vector<Position> getPossibleMoves(const Position& pos) const;

    bool isInCheck(PieceColor color) const;
    bool isCheckmate(PieceColor color) const;
    bool canCastleKingside(PieceColor color) const;
    bool canCastleQueenside(PieceColor color) const;
    bool removePiece(const Position& pos);
    bool isValidPosition(const Position& pos) const;
    Position getWhiteKingPosition() const { return whiteKingPos_; }
    Position getBlackKingPosition() const { return blackKingPos_; }
    void setWhiteKingPosition(const Position& pos) { whiteKingPos_ = pos; }
    void setBlackKingPosition(const Position& pos) { blackKingPos_ = pos; }
    
private:
    std::array<std::array<std::shared_ptr<ChessPiece>, 8>, 8> board_;
    Position whiteKingPos_;
    Position blackKingPos_;
    Position lastPawnDoubleMove_;
    
    void initializeBoard();
    bool isPathClear(const Position& from, const Position& to) const;

    void getPawnMoves(const Position& pos, PieceColor color, std::vector<Position>& moves) const;
    void getKnightMoves(const Position& pos, PieceColor color, std::vector<Position>& moves) const;
    void getBishopMoves(const Position& pos, PieceColor color, std::vector<Position>& moves) const;
    void getRookMoves(const Position& pos, PieceColor color, std::vector<Position>& moves) const;
    void getQueenMoves(const Position& pos, PieceColor color, std::vector<Position>& moves) const;
    void getKingMoves(const Position& pos, PieceColor color, std::vector<Position>& moves) const;
    void getSlidingMoves(const Position& pos, PieceColor color, const int directions[][2], int numDirections, std::vector<Position>& moves) const;
};