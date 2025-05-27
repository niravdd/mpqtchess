// src/core/ChessGame.cpp
#include "ChessGame.h"
#include "ChessPiece.h"
#include <algorithm>
#include <cassert>
#include <sstream>

ChessGame::ChessGame()
    : board_(std::make_unique<ChessBoard>())
    , currentTurn_(PieceColor::White)
    , gameOver_(false)
    , halfMoveClock_(0)
    , fullMoveNumber_(1)
    , lastMove_()
{
    initializeGame();
}

void ChessGame::initializeGame()
{
    board_->initializeBoard();
    moveHistory_.clear();
    currentTurn_ = PieceColor::White;
    gameOver_ = false;
    halfMoveClock_ = 0;
    fullMoveNumber_ = 1;
    lastMove_ = Move();
}

bool ChessGame::makeMove(const Position& from, const Position& to, 
                        PieceColor playerColor, PieceType promotionPiece)
{
    if (gameOver_) {
        return false;
    }

    if (playerColor != currentTurn_) {
        return false;
    }

    if (!isValidMove(from, to, playerColor)) {
        return false;
    }

    auto piece = board_->getPieceAt(from);
    auto targetPiece = board_->getPieceAt(to);

    // Create move record
    Move move;
    move.from = from;
    move.to = to;
    move.promotionPiece = promotionPiece;
    move.isCapture = (targetPiece != nullptr);
    move.isCastling = (piece->getType() == PieceType::King && 
                      abs(from.col - to.col) == 2);
    move.isEnPassant = (piece->getType() == PieceType::Pawn && 
                       from.col != to.col && !targetPiece);

    // Update half-move clock
    if (piece->getType() == PieceType::Pawn || move.isCapture) {
        halfMoveClock_ = 0;
    } else {
        halfMoveClock_++;
    }

    // Handle special moves before making the actual move
    if (move.isCastling) {
        if (!handleCastling(from, to)) {
            return false;
        }
    }

    if (move.isEnPassant) {
        if (!handleEnPassant(from, to)) {
            return false;
        }
    }

    // Make the actual move
    if (!board_->movePiece(from, to, promotionPiece)) {
        return false;
    }

    // Update king position if king was moved
    if (piece->getType() == PieceType::King) {
        if (playerColor == PieceColor::White) {
            board_->setWhiteKingPosition(to);
        } else {
            board_->setBlackKingPosition(to);
        }
    }

    // Check if this move puts opponent in check
    PieceColor opponentColor = (playerColor == PieceColor::White) ? 
                              PieceColor::Black : PieceColor::White;
    
    move.isCheck = isInCheck(opponentColor);
    move.isCheckmate = isCheckmate(opponentColor);

    // Record the move
    recordMove(move, piece->getType());
    lastMove_ = move;

    // Update turn
    if (currentTurn_ == PieceColor::Black) {
        fullMoveNumber_++;
    }
    
    currentTurn_ = (currentTurn_ == PieceColor::White) ? 
                   PieceColor::Black : PieceColor::White;

    // Update game state
    updateGameState(currentTurn_);

    return true;
}

bool ChessGame::handleCastling(const Position& from, const Position& to)
{
    int row = from.row;
    bool isKingside = (to.col > from.col);
    
    // Determine rook positions
    Position rookFrom(row, isKingside ? 7 : 0);
    Position rookTo(row, isKingside ? 5 : 3);
    
    auto rook = board_->getPieceAt(rookFrom);
    if (!rook || rook->getType() != PieceType::Rook || rook->hasMoved()) {
        return false;
    }
    
    // Move the rook
    return board_->movePiece(rookFrom, rookTo, PieceType::None);
}

bool ChessGame::handleEnPassant(const Position& from, const Position& to)
{
    // Remove the captured pawn
    Position capturedPawnPos(from.row, to.col);
    if (!board_->removePiece(capturedPawnPos)) {
        return false;
    }
    return true;
}

void ChessGame::recordMove(const Move& move, PieceType piece)
{
    MoveRecord record;
    record.moveNumber = fullMoveNumber_;
    record.piece = piece;
    record.fromSquare = positionToString(move.from);
    record.toSquare = positionToString(move.to);
    record.isCapture = move.isCapture;
    record.isCheck = move.isCheck;
    record.isCheckmate = move.isCheckmate;
    record.promotionPiece = move.promotionPiece;
    
    moveHistory_.push_back(record);
}

bool ChessGame::isValidMove(const Position& from, const Position& to, 
    PieceColor playerColor) const
{
    // Check position bounds
    if (!board_->isValidPosition(from) || !board_->isValidPosition(to)) {
        return false;
    }

    // Check if source piece exists and belongs to the player
    auto piece = board_->getPieceAt(from);
    if (!piece || piece->getColor() != playerColor) {
        return false;
    }

    // Check if target square contains player's own piece
    auto targetPiece = board_->getPieceAt(to);
    if (targetPiece && targetPiece->getColor() == playerColor) {
        return false;
    }

    // Handle special moves
    if (piece->getType() == PieceType::King && abs(from.col - to.col) == 2) {
        return isValidCastling(from, to, playerColor);
    }

    if (piece->getType() == PieceType::Pawn) {
        if (from.col != to.col && !targetPiece) {
            return isValidEnPassant(from, to, playerColor);
        }
    }

    // Check if the move is in the piece's possible moves
    auto possibleMoves = board_->getPossibleMoves(from);
    if (std::find(possibleMoves.begin(), possibleMoves.end(), to) == possibleMoves.end()) {
        return false;
    }

    // Check if move would leave or put the king in check
    if (wouldBeInCheck(from, to, playerColor)) {
        return false;
    }

    return true;
}

bool ChessGame::isValidCastling(const Position& from, const Position& to, PieceColor playerColor) const
{
    auto king = board_->getPieceAt(from);
    if (!king || king->getType() != PieceType::King || king->hasMoved()) {
        return false;
    }

    // Check if king is in check
    if (isInCheck(playerColor)) {
        return false;
    }

    int row = from.row;
    bool isKingside = (to.col > from.col);

    // Check rook
    Position rookPos(row, isKingside ? 7 : 0);
    auto rook = board_->getPieceAt(rookPos);
    if (!rook || rook->getType() != PieceType::Rook || rook->hasMoved()) {
        return false;
    }

    // Check if path is clear
    int startCol = from.col;
    int endCol = isKingside ? 7 : 0;
    int step = isKingside ? 1 : -1;

    for (int col = startCol + step; col != endCol; col += step) {
        if (board_->getPieceAt(Position(row, col))) {
            return false;
    }
    }

    // Check if squares king moves through are under attack
    for (int col = startCol; col != to.col; col += step) {
        if (isSquareUnderAttack(Position(row, col), playerColor)) {
            return false;
        }
    }

    return true;
}

bool ChessGame::isValidEnPassant(const Position& from, const Position& to, 
         PieceColor playerColor) const
{
    if (!lastMove_.isValid()) {
        return false;
    }

    auto pawn = board_->getPieceAt(from);
    if (!pawn || pawn->getType() != PieceType::Pawn) {
        return false;
    }

    // Check if last move was a pawn double move
    auto lastPawn = board_->getPieceAt(Position(from.row, to.col));
    if (!lastPawn || lastPawn->getType() != PieceType::Pawn ||
        lastPawn->getColor() == playerColor) {
        return false;
    }

    // Check if last move was a double pawn advance
    if (lastMove_.from.col != to.col || abs(lastMove_.from.row - lastMove_.to.row) != 2 || lastMove_.to.row != from.row) {
        return false;
    }

    return true;
}

bool ChessGame::isSquareUnderAttack(const Position& pos, PieceColor defendingColor) const
{
    PieceColor attackingColor = (defendingColor == PieceColor::White) ? 
            PieceColor::Black : PieceColor::White;

    // Check attacks from each piece type
    return  isDiagonallyThreatened(pos, attackingColor, *board_) ||
            isStraightThreatened(pos, attackingColor, *board_) ||
            isKnightThreatened(pos, attackingColor, *board_) ||
            isPawnThreatened(pos, attackingColor, *board_) ||
            isKingThreatened(pos, attackingColor, *board_);
}

std::vector<Position> ChessGame::getLegalMoves(const Position& pos) const
{
    std::vector<Position> legalMoves;
    auto piece = board_->getPieceAt(pos);

    if (!piece) {
        return legalMoves;
    }

    // Get all possible moves for this piece
    auto possibleMoves = board_->getPossibleMoves(pos);

    // Filter out moves that would leave the king in check
    for (const auto& move : possibleMoves) {
        if (!wouldBeInCheck(pos, move, piece->getColor())) {
            legalMoves.push_back(move);
        }
    }

    // Add castling moves for king if applicable
    if (piece->getType() == PieceType::King && !piece->hasMoved()) {
        addCastlingMoves(pos, piece->getColor(), legalMoves);
    }

    // Add en passant moves for pawns if applicable
    if (piece->getType() == PieceType::Pawn) {
        addEnPassantMoves(pos, piece->getColor(), legalMoves);
    }

    return legalMoves;
}

void ChessGame::addCastlingMoves(const Position& kingPos, PieceColor color,
          std::vector<Position>& moves) const
{
    if (isInCheck(color)) {
        return;
    }

    int row = (color == PieceColor::White) ? 7 : 0;

    // Kingside castling
    if (isValidCastling(kingPos, Position(row, 6), color)) {
        moves.push_back(Position(row, 6));
    }

    // Queenside castling
    if (isValidCastling(kingPos, Position(row, 2), color)) {
        moves.push_back(Position(row, 2));
    }
}

void ChessGame::addEnPassantMoves(const Position& pawnPos, PieceColor color,
           std::vector<Position>& moves) const
{
    if (!lastMove_.isValid() || 
        lastMove_.piece != PieceType::Pawn || 
        abs(lastMove_.from.row - lastMove_.to.row) != 2) {
        return;
    }

    int captureRow = (color == PieceColor::White) ? 2 : 5;
    if (pawnPos.row != captureRow) {
        return;
    }

    // Check adjacent columns
    for (int colOffset : {-1, 1}) {
        Position targetPos(pawnPos.row + (color == PieceColor::White ? -1 : 1), pawnPos.col + colOffset);
        moves.push_back(targetPos);
        if (isValidEnPassant(pawnPos, targetPos, color)) {
        }
    }
}

bool ChessGame::isInCheck(PieceColor color) const
{
    Position kingPos = (color == PieceColor::White) ? 
                      board_->getWhiteKingPosition() : 
                      board_->getBlackKingPosition();
    
    return isSquareUnderAttack(kingPos, color);
}

bool ChessGame::wouldBeInCheck(const Position& from, const Position& to, 
                              PieceColor color) const
{
    // Create a temporary board to test the move
    ChessBoard tempBoard = *board_;
    auto movingPiece = tempBoard.getPieceAt(from);
    auto capturedPiece = tempBoard.getPieceAt(to);
    
    // Handle en passant capture
    if (movingPiece->getType() == PieceType::Pawn && 
        from.col != to.col && !capturedPiece) {
        Position capturedPawnPos(from.row, to.col);
        tempBoard.removePiece(capturedPawnPos);
    }
    
    // Handle castling
    if (movingPiece->getType() == PieceType::King && 
        abs(from.col - to.col) == 2) {
        int row = from.row;
        bool isKingside = (to.col > from.col);
        Position rookFrom(row, isKingside ? 7 : 0);
        Position rookTo(row, isKingside ? 5 : 3);
        tempBoard.movePiece(rookFrom, rookTo, PieceType::None);
    }
    
    // Make the move on temporary board
    tempBoard.movePiece(from, to, PieceType::None);
    
    // Find king's position after the move
    Position kingPos;
    if (movingPiece->getType() == PieceType::King) {
        kingPos = to;
    } else {
        kingPos = (color == PieceColor::White) ? 
                  tempBoard.getWhiteKingPosition() : 
                  tempBoard.getBlackKingPosition();
    }
    
    // Check if the king would be under attack after the move
    return isSquareUnderAttack(kingPos, color);
}

bool ChessGame::isDiagonallyThreatened(const Position& pos, PieceColor attacker,
                                      const ChessBoard& board) const
{
    const int directions[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    
    for (const auto& dir : directions) {
        int row = pos.row + dir[0];
        int col = pos.col + dir[1];
        
        while (isValidPosition(Position(row, col))) {
            auto piece = board.getPieceAt(Position(row, col));
            if (piece) {
                if (piece->getColor() == attacker &&
                    (piece->getType() == PieceType::Bishop ||
                     piece->getType() == PieceType::Queen)) {
                    return true;
                }
                break;  // Piece blocking line of sight
            }
            row += dir[0];
            col += dir[1];
        }
    }
    return false;
}

bool ChessGame::isStraightThreatened(const Position& pos, PieceColor attacker,
                                    const ChessBoard& board) const
{
    const int directions[4][2] = {{0,1}, {0,-1}, {1,0}, {-1,0}};
    
    for (const auto& dir : directions) {
        int row = pos.row + dir[0];
        int col = pos.col + dir[1];
        
        while (isValidPosition(Position(row, col))) {
            auto piece = board.getPieceAt(Position(row, col));
            if (piece) {
                if (piece->getColor() == attacker &&
                    (piece->getType() == PieceType::Rook ||
                     piece->getType() == PieceType::Queen)) {
                    return true;
                }
                break;  // Piece blocking line of sight
            }
            row += dir[0];
            col += dir[1];
        }
    }
    return false;
}

bool ChessGame::isKnightThreatened(const Position& pos, PieceColor attacker,
                                  const ChessBoard& board) const
{
    const int knightMoves[8][2] = {
        {-2,-1}, {-2,1}, {-1,-2}, {-1,2},
        {1,-2}, {1,2}, {2,-1}, {2,1}
    };
    
    for (const auto& move : knightMoves) {
        Position checkPos(pos.row + move[0], pos.col + move[1]);
        if (isValidPosition(checkPos)) {
            auto piece = board.getPieceAt(checkPos);
            if (piece && piece->getColor() == attacker &&
                piece->getType() == PieceType::Knight) {
                return true;
            }
        }
    }
    return false;
}

bool ChessGame::isPawnThreatened(const Position& pos, PieceColor attacker,
                                const ChessBoard& board) const
{
    int direction = (attacker == PieceColor::White) ? 1 : -1;
    
    // Check both diagonal captures
    for (int colOffset : {-1, 1}) {
        Position checkPos(pos.row + direction, pos.col + colOffset);
        if (isValidPosition(checkPos)) {
            auto piece = board.getPieceAt(checkPos);
            if (piece && piece->getColor() == attacker &&
                piece->getType() == PieceType::Pawn) {
                return true;
            }
        }
    }
    return false;
}

bool ChessGame::isKingThreatened(const Position& pos, PieceColor attacker,
                                const ChessBoard& board) const
{
    const int kingMoves[8][2] = {
        {-1,-1}, {-1,0}, {-1,1},
        {0,-1},          {0,1},
        {1,-1},  {1,0},  {1,1}
    };
    
    for (const auto& move : kingMoves) {
        Position checkPos(pos.row + move[0], pos.col + move[1]);
        if (isValidPosition(checkPos)) {
            auto piece = board.getPieceAt(checkPos);
            if (piece && piece->getColor() == attacker &&
                piece->getType() == PieceType::King) {
                return true;
            }
        }
    }
    return false;
}

bool ChessGame::isCheckmate(PieceColor color) const
{
    return isInCheck(color) && !hasLegalMoves(color);
}

bool ChessGame::isStalemate(PieceColor color) const
{
    return !isInCheck(color) && !hasLegalMoves(color);
}

bool ChessGame::isDraw() const
{
    return isStalemate(currentTurn_) || 
           halfMoveClock_ >= 100 || 
           !hasSufficientMaterial() ||
           isThreefoldRepetition();
}

bool ChessGame::hasLegalMoves(PieceColor color) const
{
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Position pos(row, col);
            auto piece = board_->getPieceAt(pos);
            if (piece && piece->getColor() == color) {
                if (!getLegalMoves(pos).empty()) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool ChessGame::hasSufficientMaterial() const
{
    int whiteBishops = 0, whiteKnights = 0;
    int blackBishops = 0, blackKnights = 0;
    int totalPieces = 0;
    bool whiteBishopSquareColor = false;  // false = light, true = dark
    bool blackBishopSquareColor = false;
    bool hasSetWhiteBishopColor = false;
    bool hasSetBlackBishopColor = false;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            auto piece = board_->getPieceAt(Position(row, col));
            if (!piece) continue;
            
            totalPieces++;
            
            // If there's a pawn, rook, or queen, there's sufficient material
            if (piece->getType() == PieceType::Pawn ||
                piece->getType() == PieceType::Rook ||
                piece->getType() == PieceType::Queen) {
                return true;
            }
            
            if (piece->getColor() == PieceColor::White) {
                if (piece->getType() == PieceType::Bishop) {
                    whiteBishops++;
                    if (!hasSetWhiteBishopColor) {
                        whiteBishopSquareColor = ((row + col) % 2 == 1);
                        hasSetWhiteBishopColor = true;
                    }
                }
                if (piece->getType() == PieceType::Knight) {
                    whiteKnights++;
                }
            } else {
                if (piece->getType() == PieceType::Bishop) {
                    blackBishops++;
                    if (!hasSetBlackBishopColor) {
                        blackBishopSquareColor = ((row + col) % 2 == 1);
                        hasSetBlackBishopColor = true;
                    }
                }
                if (piece->getType() == PieceType::Knight) {
                    blackKnights++;
                }
            }
        }
    }
    
    // King vs King
    if (totalPieces == 2) {
        return false;
    }
    
    // King and minor piece vs King
    if (totalPieces == 3 && 
        (whiteBishops + whiteKnights + blackBishops + blackKnights == 1)) {
        return false;
    }
    
    // King and Bishop vs King and Bishop (same colored squares)
    if (totalPieces == 4 && whiteBishops == 1 && blackBishops == 1 &&
        whiteBishopSquareColor == blackBishopSquareColor) {
        return false;
    }
    
    // King and Knight vs King and Knight
    if (totalPieces == 4 && whiteKnights == 1 && blackKnights == 1) {
        return false;
    }
    
    // Two knights cannot force checkmate
    if (totalPieces == 3 && 
        ((whiteKnights == 2 && blackBishops == 0 && blackKnights == 0) ||
         (blackKnights == 2 && whiteBishops == 0 && whiteKnights == 0))) {
        return false;
    }
    
    return true;
}

bool ChessGame::isThreefoldRepetition() const
{
    // Create a map to store position counts
    std::map<std::string, int> positionCounts;
    
    // Generate and count current position
    std::string currentPosition = generatePositionKey();
    positionCounts[currentPosition] = 1;
    
    // Replay all moves to check for repetitions
    ChessBoard tempBoard;
    for (const auto& record : moveHistory_) {
        Position from = stringToPosition(record.fromSquare);
        Position to = stringToPosition(record.toSquare);
        tempBoard.movePiece(from, to, record.promotionPiece);
        
        std::string position = generatePositionKey(tempBoard);
        positionCounts[position]++;
        
        if (positionCounts[position] >= 3) {
            return true;
        }
    }
    
    return false;
}

std::string ChessGame::generatePositionKey(const ChessBoard& board) const
{
    std::stringstream key;
    
    // Add piece positions
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            auto piece = board.getPieceAt(Position(row, col));
            if (piece) {
                key << static_cast<int>(piece->getType())
                    << static_cast<int>(piece->getColor());
            }
            key << "|";
        }
    }
    
    // Add castling rights
    key << board.canCastleKingside(PieceColor::White)
        << board.canCastleQueenside(PieceColor::White)
        << board.canCastleKingside(PieceColor::Black)
        << board.canCastleQueenside(PieceColor::Black);
    
    // Add en passant possibilities
    if (lastMove_.isValid() && lastMove_.piece == PieceType::Pawn &&
        abs(lastMove_.from.row - lastMove_.to.row) == 2) {
        key << "|EP:" << lastMove_.to.col;
    }
    
    return key.str();
}

std::string ChessGame::generatePositionKey() const
{
    return generatePositionKey(*board_);
}

void ChessGame::updateGameState(PieceColor color)
{
    if (isCheckmate(color)) {
        gameOver_ = true;
        gameResult_ = (color == PieceColor::White) ? 
                     GameResult::BlackWin : GameResult::WhiteWin;
    }
    else if (isStalemate(color) || 
             !hasSufficientMaterial() || 
             halfMoveClock_ >= 100 ||
             isThreefoldRepetition()) {
        gameOver_ = true;
        gameResult_ = GameResult::Draw;
    }
}

std::string ChessGame::getGameResult() const
{
    switch (gameResult_) {
        case GameResult::WhiteWin:
            return "1-0";
        case GameResult::BlackWin:
            return "0-1";
        case GameResult::Draw:
            return "1/2-1/2";
        default:
            return "*";
    }
}

bool ChessGame::isValidPosition(const Position& pos) const
{
    return pos.row >= 0 && pos.row < 8 && pos.col >= 0 && pos.col < 8;
}

std::shared_ptr<ChessPiece> ChessGame::getPieceAt(const Position& pos) const
{
    return board_->getPieceAt(pos);
}

int ChessGame::getPieceCount() const
{
    int count = 0;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (board_->getPieceAt(Position(row, col))) {
                count++;
            }
        }
    }
    return count;
}

void ChessGame::clearBoard()
{
    // Remove all pieces from the board
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            board_->removePiece(Position(row, col));
        }
    }
}

const std::vector<MoveRecord>& ChessGame::getMoveHistory() const
{
    return moveHistory_;
}

std::string ChessGame::positionToString(const Position& pos) const
{
    char file = 'a' + pos.col;
    char rank = '8' - pos.row;
    return std::string{file} + rank;
}

Position ChessGame::stringToPosition(const std::string& str)
{
    if (str.length() != 2) {
        return Position(-1, -1);
    }
    return Position(('8' - str[1]), (str[0] - 'a'));
}

void ChessGame::resign(PieceColor color)
{
    gameOver_ = true;
    gameResult_ = (color == PieceColor::White) ? 
                 GameResult::BlackWin : GameResult::WhiteWin;
}

void ChessGame::offerDraw(PieceColor color)
{
    drawOffered_ = true;
    drawOfferingColor_ = color;
}

void ChessGame::acceptDraw()
{
    if (drawOffered_) {
        gameOver_ = true;
        gameResult_ = GameResult::Draw;
    }
}

void ChessGame::declineDraw()
{
    drawOffered_ = false;
}

QJsonObject ChessGame::toJson() const
{
    QJsonObject obj;
    // Save current game state: board position, current player, move history, etc.
    obj["currentTurn"] = static_cast<int>(getCurrentTurn());
    
    // Save board state
    QJsonArray boardArray;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            QPoint pos(col, row);
            const ChessPiece* piece = getPieceAt(pos).get();
            if (piece && piece->getType() != PieceType::None) {
                QJsonObject pieceObj;
                pieceObj["row"] = row;
                pieceObj["col"] = col;
                pieceObj["type"] = static_cast<int>(piece->getType());
                pieceObj["color"] = static_cast<int>(piece->getColor());
                boardArray.append(pieceObj);
            }
        }
    }
    obj["board"] = boardArray;
    
    // Save move history
    QJsonArray movesArray;
    for (const auto& move : moveHistory_) {
        QJsonObject moveObj;
        moveObj["moveNumber"] = move.moveNumber;
        moveObj["piece"] = static_cast<int>(move.piece);
        moveObj["fromSquare"] = QString::fromStdString(move.fromSquare);
        moveObj["toSquare"] = QString::fromStdString(move.toSquare);
        moveObj["isCapture"] = move.isCapture;
        moveObj["isCheck"] = move.isCheck;
        moveObj["isCheckmate"] = move.isCheckmate;
        moveObj["promotionPiece"] = static_cast<int>(move.promotionPiece);
        movesArray.append(moveObj);
    }
    obj["moves"] = movesArray;
    
    return obj;
}

bool ChessGame::fromJson(const QJsonObject& json)
{
    // Clear current game state
    resetGame();
    
    // Load current turn
    if (json.contains("currentTurn")) {
        int turn = json["currentTurn"].toInt();
        setCurrentTurn(static_cast<PieceColor>(turn));
    }
    
    // Load board state
    if (json.contains("board") && json["board"].isArray()) {
        QJsonArray boardArray = json["board"].toArray();
        for (const QJsonValue& val : boardArray) {
            if (!val.isObject()) continue;
            
            QJsonObject pieceObj = val.toObject();
            int row = pieceObj["row"].toInt();
            int col = pieceObj["col"].toInt();
            int type = pieceObj["type"].toInt();
            int color = pieceObj["color"].toInt();
            
            // Add piece to the board
            auto piece = std::make_shared<ChessPiece>(
                                static_cast<PieceType>(type),
                                static_cast<PieceColor>(color)
            );
            addPiece(QPoint(col, row), piece);
        }
    }
    
    // Load move history
    if (json.contains("moves") && json["moves"].isArray()) {
        QJsonArray movesArray = json["moves"].toArray();
        for (const QJsonValue& val : movesArray) {
            if (val.isObject()) {
                moveHistory_.push_back(MoveRecord::fromString(val.toString().toStdString()));
            }
        }
    }
    
    return true;
}

void ChessGame::setCurrentTurn(PieceColor color) {
    currentTurn_ = color;
}

void ChessGame::addPiece(const QPoint& position, std::shared_ptr<ChessPiece> piece) {
    board_->placePiece(position, piece);
}

void ChessGame::resetGame() 
{
    // Reset board state using existing initialization logic
    board_->initializeBoard();
    
    // Clear move history and game state
    moveHistory_.clear();
    gameOver_ = false;
    gameResult_ = GameResult::InProgress;

    // Reset turn/player tracking
    currentTurn_ = PieceColor::White;
    currentPlayer_ = PieceColor::White;  // Synchronize with currentTurn_

    // Reset move counters
    halfMoveClock_ = 0;    // For fifty-move rule
    fullMoveNumber_ = 1;   // Total moves counter
    
    // Reset draw state
    drawOffered_ = false;
    drawOfferingColor_ = PieceColor::White; // Neutral default
    
    // Clear special move tracking
    lastMove_ = Move();
    
    // Reset king positions through board
    board_->setWhiteKingPosition(Position(7, 4));  // e1
    board_->setBlackKingPosition(Position(0, 4));  // e8
}

std::string MoveRecord::toString() const
{
    return std::to_string(moveNumber) + "," +
           std::to_string(static_cast<int>(piece)) + "," +
           fromSquare + "," +
           toSquare + "," +
           (isCapture ? "1" : "0") + "," +
           (isCheck ? "1" : "0") + "," +
           (isCheckmate ? "1" : "0") + "," +
           std::to_string(static_cast<int>(promotionPiece));
}

MoveRecord MoveRecord::fromString(const std::string& str)
{
    MoveRecord record;
    size_t pos = 0;
    size_t next;
    int field_counter = 0;
    
    try {
        while((next = str.find(',', pos)) != std::string::npos && field_counter < 7) {
            std::string token = str.substr(pos, next - pos);
            
            switch(field_counter) {
                case 0: record.moveNumber = std::stoi(token); break;
                case 1: record.piece = static_cast<PieceType>(std::stoi(token)); break;
                case 2: record.fromSquare = token; break;
                case 3: record.toSquare = token; break;
                case 4: record.isCapture = (token == "1"); break;
                case 5: record.isCheck = (token == "1"); break;
                case 6: record.isCheckmate = (token == "1"); break;
            }
            
            pos = next + 1;
            field_counter++;
        }
        
        // Handle last field (promotionPiece)
        if(pos < str.size()) {
            record.promotionPiece = static_cast<PieceType>(std::stoi(str.substr(pos)));
        }
    } catch(const std::exception& e) {
        // Return empty record on parse failure
        return MoveRecord{};
    }
    
    return record;
}