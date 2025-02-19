// src/core/ChessGame.h
#pragma once
#include "ChessBoard.h"
#include "Position.h"
#include <memory>
#include <vector>
#include <string>

enum class GameResult {
    InProgress,
    WhiteWin,
    BlackWin,
    Draw
};

struct Move {
    Position from;
    Position to;
    PieceType promotionPiece;
    bool isCapture;
    bool isCheck;
    bool isCheckmate;
    bool isCastling;
    bool isEnPassant;
    
    bool isValid() const {
        return from.row >= 0 && from.row < 8 &&
               from.col >= 0 && from.col < 8 &&
               to.row >= 0 && to.row < 8 &&
               to.col >= 0 && to.col < 8;
    }
};

class MoveRecord {
public:
    int moveNumber;
    PieceType piece;
    std::string fromSquare;
    std::string toSquare;
    bool isCapture;
    bool isCheck;
    bool isCheckmate;
    PieceType promotionPiece;
};

class ChessGame {
public:
    ChessGame();
    
    bool makeMove(const Position& from, const Position& to, 
                 PieceColor playerColor, PieceType promotionPiece = PieceType::None);
    bool isValidMove(const Position& from, const Position& to, 
                    PieceColor playerColor) const;
    
    std::vector<Position> getLegalMoves(const Position& pos) const;
    bool isInCheck(PieceColor color) const;
    bool isCheckmate(PieceColor color) const;
    bool isStalemate(PieceColor color) const;
    bool isDraw() const;
    
    PieceColor getCurrentTurn() const { return currentTurn_; }
    const ChessBoard& getBoard() const { return *board_; }
    const std::vector<MoveRecord>& getMoveHistory() const { return moveHistory_; }
    
    std::shared_ptr<ChessPiece> getPieceAt(const Position& pos) const;
    bool isGameOver() const { return gameOver_; }
    
private:
    std::unique_ptr<ChessBoard> board_;
    PieceColor currentTurn_;
    std::vector<MoveRecord> moveHistory_;
    bool gameOver_;
    int halfMoveClock_;  // For fifty-move rule
    int fullMoveNumber_;
    
    void recordMove(const Move& move, PieceType piece);
    bool wouldBeInCheck(const Position& from, const Position& to, 
                       PieceColor color) const;
    void updateGameState(PieceColor color);
    bool hasLegalMoves(PieceColor color) const;
    bool hasSufficientMaterial() const;
    GameResult gameResult_;
    bool drawOffered_;
    PieceColor drawOfferingColor_;
    Move lastMove_;
};