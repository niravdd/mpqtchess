// src/core/ChessGame.h
#pragma once
#include "ChessBoard.h"
#include "Position.h"
#include <memory>
#include <vector>
#include <string>
#include <map>

enum class GameResult {
    InProgress,
    WhiteWin,
    BlackWin,
    Draw
};

struct Move {
    Position from;
    Position to;
    PieceType piece;
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
    bool isValidPosition(const Position& pos) const;
    
    PieceColor getCurrentTurn() const { return currentTurn_; }
    PieceColor getCurrentPlayer() const { return currentPlayer_; }

    const ChessBoard& getBoard() const { return *board_; }
    const std::vector<MoveRecord>& getMoveHistory() const;
    
    std::shared_ptr<ChessPiece> getPieceAt(const Position& pos) const;
    bool isGameOver() const { return gameOver_; }
    std::string getGameResult() const;
    void resign(PieceColor color);
    void offerDraw(PieceColor color);
    void acceptDraw();
    void declineDraw();
    
private:
    std::unique_ptr<ChessBoard> board_;
    PieceColor currentTurn_;
    PieceColor currentPlayer_;
    std::vector<MoveRecord> moveHistory_;
    bool gameOver_;
    int halfMoveClock_;  // For fifty-move rule
    int fullMoveNumber_;

    void recordMove(const Move& move, PieceType piece);
    bool wouldBeInCheck(const Position& from, const Position& to, PieceColor color) const;
    void updateGameState(PieceColor color);
    bool hasLegalMoves(PieceColor color) const;
    bool hasSufficientMaterial() const;
    void initializeGame();
    bool handleCastling(const Position& from, const Position& to);
    bool handleEnPassant(const Position& from, const Position& to);
    bool isValidCastling(const Position& from, const Position& to, PieceColor playerColor) const;
    bool isValidEnPassant(const Position& from, const Position& to, PieceColor playerColor) const;
    bool isSquareUnderAttack(const Position& pos, PieceColor playerColor) const;
    bool isDiagonallyThreatened(const Position& pos, PieceColor attacker, const ChessBoard& board) const;
    bool isStraightThreatened(const Position& pos, PieceColor attacker, const ChessBoard& board) const;
    bool isKnightThreatened(const Position& pos, PieceColor attacker, const ChessBoard& board) const;
    bool isPawnThreatened(const Position& pos, PieceColor attacker, const ChessBoard& board) const;
    bool isKingThreatened(const Position& pos, PieceColor attacker, const ChessBoard& board) const;
    void addCastlingMoves(const Position& pos, PieceColor color, std::vector<Position>& moves) const;
    void addEnPassantMoves(const Position& pos, PieceColor color, std::vector<Position>& moves) const;
    bool isThreefoldRepetition() const;
    std::string generatePositionKey() const;
    std::string generatePositionKey(const ChessBoard& board) const;
    static Position stringToPosition(const std::string& str);
    std::string positionToString(const Position& pos) const;

    GameResult gameResult_;
    bool drawOffered_;
    PieceColor drawOfferingColor_;
    Move lastMove_;
};