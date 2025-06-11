// MPChessServer.cpp

#include "MPChessServer.h"

// Initialize static members
std::mutex PerformanceMonitor::mutex;
std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> PerformanceMonitor::operationTimers;
std::unordered_map<std::string, PerformanceMonitor::OperationStats> PerformanceMonitor::operationStats;

// Implementation of ChessPiece class
ChessPiece::ChessPiece(PieceType type, PieceColor color)
    : type(type), color(color), moved(false) {
}

PieceType ChessPiece::getType() const {
    return type;
}

PieceColor ChessPiece::getColor() const {
    return color;
}

bool ChessPiece::hasMoved() const {
    return moved;
}

void ChessPiece::setMoved(bool moved) {
    this->moved = moved;
}

char ChessPiece::getAsciiChar() const {
    char c;
    switch (type) {
        case PieceType::PAWN:   c = 'p'; break;
        case PieceType::KNIGHT: c = 'n'; break;
        case PieceType::BISHOP: c = 'b'; break;
        case PieceType::ROOK:   c = 'r'; break;
        case PieceType::QUEEN:  c = 'q'; break;
        case PieceType::KING:   c = 'k'; break;
        default:                c = ' '; break;
    }
    
    if (color == PieceColor::WHITE) {
        c = std::toupper(c);
    }
    
    return c;
}

// Concrete piece classes
class Pawn : public ChessPiece {
public:
    Pawn(PieceColor color) : ChessPiece(PieceType::PAWN, color) {}
    
    std::vector<Position> getPossibleMoves(const Position& pos, const ChessBoard& board) const override {
        std::vector<Position> moves;
        int direction = (color == PieceColor::WHITE) ? 1 : -1;
        Position forward(pos.row + direction, pos.col);
        
        // Forward move
        if (forward.isValid() && board.getPiece(forward) == nullptr) {
            moves.push_back(forward);
            
            // Double forward move from starting position
            if (!moved) {
                Position doubleForward(pos.row + 2 * direction, pos.col);
                if (doubleForward.isValid() && board.getPiece(doubleForward) == nullptr) {
                    moves.push_back(doubleForward);
                }
            }
        }
        
        // Captures
        Position captureLeft(pos.row + direction, pos.col - 1);
        Position captureRight(pos.row + direction, pos.col + 1);
        
        if (captureLeft.isValid()) {
            const ChessPiece* piece = board.getPiece(captureLeft);
            if (piece && piece->getColor() != color) {
                moves.push_back(captureLeft);
            }
            
            // En passant capture
            Position enPassantTarget = board.getEnPassantTarget();
            if (enPassantTarget.isValid() && captureLeft == enPassantTarget) {
                moves.push_back(captureLeft);
            }
        }
        
        if (captureRight.isValid()) {
            const ChessPiece* piece = board.getPiece(captureRight);
            if (piece && piece->getColor() != color) {
                moves.push_back(captureRight);
            }
            
            // En passant capture
            Position enPassantTarget = board.getEnPassantTarget();
            if (enPassantTarget.isValid() && captureRight == enPassantTarget) {
                moves.push_back(captureRight);
            }
        }
        
        return moves;
    }
    
    std::unique_ptr<ChessPiece> clone() const override {
        auto clone = std::make_unique<Pawn>(color);
        clone->setMoved(moved);
        return clone;
    }
};

class Knight : public ChessPiece {
public:
    Knight(PieceColor color) : ChessPiece(PieceType::KNIGHT, color) {}
    
    std::vector<Position> getPossibleMoves(const Position& pos, const ChessBoard& board) const override {
        std::vector<Position> moves;
        const std::vector<std::pair<int, int>> offsets = {
            {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
            {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
        };
        
        for (const auto& offset : offsets) {
            Position newPos(pos.row + offset.first, pos.col + offset.second);
            if (newPos.isValid()) {
                const ChessPiece* piece = board.getPiece(newPos);
                if (!piece || piece->getColor() != color) {
                    moves.push_back(newPos);
                }
            }
        }
        
        return moves;
    }
    
    std::unique_ptr<ChessPiece> clone() const override {
        auto clone = std::make_unique<Knight>(color);
        clone->setMoved(moved);
        return clone;
    }
};

class Bishop : public ChessPiece {
public:
    Bishop(PieceColor color) : ChessPiece(PieceType::BISHOP, color) {}
    
    std::vector<Position> getPossibleMoves(const Position& pos, const ChessBoard& board) const override {
        std::vector<Position> moves;
        const std::vector<std::pair<int, int>> directions = {
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
        };
        
        for (const auto& dir : directions) {
            for (int i = 1; i < 8; ++i) {
                Position newPos(pos.row + i * dir.first, pos.col + i * dir.second);
                if (!newPos.isValid()) break;
                
                const ChessPiece* piece = board.getPiece(newPos);
                if (!piece) {
                    moves.push_back(newPos);
                } else {
                    if (piece->getColor() != color) {
                        moves.push_back(newPos);
                    }
                    break;
                }
            }
        }
        
        return moves;
    }
    
    std::unique_ptr<ChessPiece> clone() const override {
        auto clone = std::make_unique<Bishop>(color);
        clone->setMoved(moved);
        return clone;
    }
};

class Rook : public ChessPiece {
public:
    Rook(PieceColor color) : ChessPiece(PieceType::ROOK, color) {}
    
    std::vector<Position> getPossibleMoves(const Position& pos, const ChessBoard& board) const override {
        std::vector<Position> moves;
        const std::vector<std::pair<int, int>> directions = {
            {0, 1}, {1, 0}, {0, -1}, {-1, 0}
        };
        
        for (const auto& dir : directions) {
            for (int i = 1; i < 8; ++i) {
                Position newPos(pos.row + i * dir.first, pos.col + i * dir.second);
                if (!newPos.isValid()) break;
                
                const ChessPiece* piece = board.getPiece(newPos);
                if (!piece) {
                    moves.push_back(newPos);
                } else {
                    if (piece->getColor() != color) {
                        moves.push_back(newPos);
                    }
                    break;
                }
            }
        }
        
        return moves;
    }
    
    std::unique_ptr<ChessPiece> clone() const override {
        auto clone = std::make_unique<Rook>(color);
        clone->setMoved(moved);
        return clone;
    }
};

class Queen : public ChessPiece {
public:
    Queen(PieceColor color) : ChessPiece(PieceType::QUEEN, color) {}
    
    std::vector<Position> getPossibleMoves(const Position& pos, const ChessBoard& board) const override {
        std::vector<Position> moves;
        const std::vector<std::pair<int, int>> directions = {
            {0, 1}, {1, 0}, {0, -1}, {-1, 0},
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
        };
        
        for (const auto& dir : directions) {
            for (int i = 1; i < 8; ++i) {
                Position newPos(pos.row + i * dir.first, pos.col + i * dir.second);
                if (!newPos.isValid()) break;
                
                const ChessPiece* piece = board.getPiece(newPos);
                if (!piece) {
                    moves.push_back(newPos);
                } else {
                    if (piece->getColor() != color) {
                        moves.push_back(newPos);
                    }
                    break;
                }
            }
        }
        
        return moves;
    }
    
    std::unique_ptr<ChessPiece> clone() const override {
        auto clone = std::make_unique<Queen>(color);
        clone->setMoved(moved);
        return clone;
    }
};

class King : public ChessPiece {
public:
    King(PieceColor color) : ChessPiece(PieceType::KING, color) {}
    
    std::vector<Position> getPossibleMoves(const Position& pos, const ChessBoard& board) const override {
        std::vector<Position> moves;
        const std::vector<std::pair<int, int>> directions = {
            {0, 1}, {1, 0}, {0, -1}, {-1, 0},
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
        };
        
        for (const auto& dir : directions) {
            Position newPos(pos.row + dir.first, pos.col + dir.second);
            if (newPos.isValid()) {
                const ChessPiece* piece = board.getPiece(newPos);
                if (!piece || piece->getColor() != color) {
                    moves.push_back(newPos);
                }
            }
        }
        
        // Castling
        if (!moved && !board.isInCheck(color)) {
            // Kingside castling
            bool canCastleKingside = true;
            for (int c = pos.col + 1; c < 7; ++c) {
                if (board.getPiece(Position(pos.row, c)) != nullptr) {
                    canCastleKingside = false;
                    break;
                }
            }
            
            const ChessPiece* rookKingside = board.getPiece(Position(pos.row, 7));
            if (canCastleKingside && rookKingside && rookKingside->getType() == PieceType::ROOK &&
                rookKingside->getColor() == color && !rookKingside->hasMoved()) {
                
                // Check if the king passes through check
                bool passesThroughCheck = false;
                Position midPos(pos.row, pos.col + 1);
                if (board.isUnderAttack(midPos, color == PieceColor::WHITE ? PieceColor::BLACK : PieceColor::WHITE)) {
                    passesThroughCheck = true;
                }
                
                if (!passesThroughCheck) {
                    moves.push_back(Position(pos.row, pos.col + 2));
                }
            }
            
            // Queenside castling
            bool canCastleQueenside = true;
            for (int c = pos.col - 1; c > 0; --c) {
                if (board.getPiece(Position(pos.row, c)) != nullptr) {
                    canCastleQueenside = false;
                    break;
                }
            }
            
            const ChessPiece* rookQueenside = board.getPiece(Position(pos.row, 0));
            if (canCastleQueenside && rookQueenside && rookQueenside->getType() == PieceType::ROOK &&
                rookQueenside->getColor() == color && !rookQueenside->hasMoved()) {
                
                // Check if the king passes through check
                bool passesThroughCheck = false;
                Position midPos(pos.row, pos.col - 1);
                if (board.isUnderAttack(midPos, color == PieceColor::WHITE ? PieceColor::BLACK : PieceColor::WHITE)) {
                    passesThroughCheck = true;
                }
                
                if (!passesThroughCheck) {
                    moves.push_back(Position(pos.row, pos.col - 2));
                }
            }
        }
        
        return moves;
    }
    
    std::unique_ptr<ChessPiece> clone() const override {
        auto clone = std::make_unique<King>(color);
        clone->setMoved(moved);
        return clone;
    }
};

// Implementation of ChessMove class
ChessMove::ChessMove() : from(-1, -1), to(-1, -1), promotionType(PieceType::EMPTY) {
}

ChessMove::ChessMove(const Position& from, const Position& to, PieceType promotionType)
    : from(from), to(to), promotionType(promotionType) {
}

Position ChessMove::getFrom() const {
    return from;
}

Position ChessMove::getTo() const {
    return to;
}

PieceType ChessMove::getPromotionType() const {
    return promotionType;
}

void ChessMove::setPromotionType(PieceType type) {
    promotionType = type;
}

std::string ChessMove::toAlgebraic() const {
    std::string result = from.toAlgebraic() + to.toAlgebraic();
    
    if (promotionType != PieceType::EMPTY) {
        char promotionChar;
        switch (promotionType) {
            case PieceType::QUEEN:  promotionChar = 'q'; break;
            case PieceType::ROOK:   promotionChar = 'r'; break;
            case PieceType::BISHOP: promotionChar = 'b'; break;
            case PieceType::KNIGHT: promotionChar = 'n'; break;
            default:                promotionChar = 'q'; break;
        }
        result += promotionChar;
    }
    
    return result;
}

ChessMove ChessMove::fromAlgebraic(const std::string& algebraic) {
    if (algebraic.length() < 4) return ChessMove();
    
    Position from = Position::fromAlgebraic(algebraic.substr(0, 2));
    Position to = Position::fromAlgebraic(algebraic.substr(2, 2));
    
    PieceType promotionType = PieceType::EMPTY;
    if (algebraic.length() > 4) {
        char promotionChar = algebraic[4];
        switch (promotionChar) {
            case 'q': promotionType = PieceType::QUEEN; break;
            case 'r': promotionType = PieceType::ROOK; break;
            case 'b': promotionType = PieceType::BISHOP; break;
            case 'n': promotionType = PieceType::KNIGHT; break;
            default:  promotionType = PieceType::QUEEN; break;
        }
    }
    
    return ChessMove(from, to, promotionType);
}

std::string ChessMove::toStandardNotation(const ChessBoard& board) const {
    const ChessPiece* piece = board.getPiece(from);
    if (!piece) return "invalid";
    
    // Handle castling
    if (piece->getType() == PieceType::KING) {
        if (from.col == 4 && to.col == 6) return "O-O";
        if (from.col == 4 && to.col == 2) return "O-O-O";
    }
    
    std::string result;
    
    // Add piece letter (except for pawns)
    if (piece->getType() != PieceType::PAWN) {
        switch (piece->getType()) {
            case PieceType::KNIGHT: result += "N"; break;
            case PieceType::BISHOP: result += "B"; break;
            case PieceType::ROOK:   result += "R"; break;
            case PieceType::QUEEN:  result += "Q"; break;
            case PieceType::KING:   result += "K"; break;
            default: break;
        }
    }
    
    // Check if disambiguation is needed
    if (piece->getType() != PieceType::PAWN && piece->getType() != PieceType::KING) {
        bool sameRank = false;
        bool sameFile = false;
        
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (r == from.row && c == from.col) continue;
                
                Position pos(r, c);
                const ChessPiece* otherPiece = board.getPiece(pos);
                if (otherPiece && otherPiece->getType() == piece->getType() &&
                    otherPiece->getColor() == piece->getColor()) {
                    
                    std::vector<Position> moves = otherPiece->getPossibleMoves(pos, board);
                    if (std::find(moves.begin(), moves.end(), to) != moves.end()) {
                        if (r == from.row) sameRank = true;
                        if (c == from.col) sameFile = true;
                    }
                }
            }
        }
        
        if (sameFile && sameRank) {
            result += from.toAlgebraic();
        } else if (sameFile) {
            result += std::to_string(from.row + 1);
        } else if (sameRank) {
            result += std::string(1, 'a' + from.col);
        }
    }
    
    // Add capture symbol
    const ChessPiece* targetPiece = board.getPiece(to);
    bool isCapture = targetPiece != nullptr || board.isEnPassantCapture(*this);
    if (isCapture) {
        if (piece->getType() == PieceType::PAWN && result.empty()) {
            result += std::string(1, 'a' + from.col);
        }
        result += "x";
    }
    
    // Add destination square
    result += to.toAlgebraic();
    
    // Add promotion piece
    if (promotionType != PieceType::EMPTY) {
        result += "=";
        switch (promotionType) {
            case PieceType::QUEEN:  result += "Q"; break;
            case PieceType::ROOK:   result += "R"; break;
            case PieceType::BISHOP: result += "B"; break;
            case PieceType::KNIGHT: result += "N"; break;
            default: break;
        }
    }
    
    // Check if the move results in check or checkmate
    // This requires making the move on a temporary board
    auto tempBoard = board.clone();
    tempBoard->movePiece(*this);
    
    PieceColor opponentColor = piece->getColor() == PieceColor::WHITE ? PieceColor::BLACK : PieceColor::WHITE;
    if (tempBoard->isInCheckmate(opponentColor)) {
        result += "#";
    } else if (tempBoard->isInCheck(opponentColor)) {
        result += "+";
    }
    
    return result;
}

bool ChessMove::operator==(const ChessMove& other) const {
    return from == other.from && to == other.to && promotionType == other.promotionType;
}

bool ChessMove::operator!=(const ChessMove& other) const {
    return !(*this == other);
}

// Implementation of ChessBoard class
ChessBoard::ChessBoard() : currentTurn(PieceColor::WHITE), enPassantTarget(-1, -1), halfMoveClock(0)
{
//  initialize();
}

void ChessBoard::initialize()
{
    try {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::initialize() - Starting board initialization");
        }
        
        // Clear the board
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                board[r][c] = nullptr;
            }
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::initialize() - Board cleared, placing white pieces");
        }
        
        // Place white pieces
        try {
            // Use make_unique with explicit types
            board[0][0] = std::make_unique<Rook>(PieceColor::WHITE);
            board[0][1] = std::make_unique<Knight>(PieceColor::WHITE);
            board[0][2] = std::make_unique<Bishop>(PieceColor::WHITE);
            board[0][3] = std::make_unique<Queen>(PieceColor::WHITE);
            board[0][4] = std::make_unique<King>(PieceColor::WHITE);
            board[0][5] = std::make_unique<Bishop>(PieceColor::WHITE);
            board[0][6] = std::make_unique<Knight>(PieceColor::WHITE);
            board[0][7] = std::make_unique<Rook>(PieceColor::WHITE);
            
            for (int c = 0; c < 8; ++c) {
                board[1][c] = std::make_unique<Pawn>(PieceColor::WHITE);
            }
            
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessBoard::initialize() - White pieces placed, placing black pieces");
            }
            
            // Place black pieces
            board[7][0] = std::make_unique<Rook>(PieceColor::BLACK);
            board[7][1] = std::make_unique<Knight>(PieceColor::BLACK);
            board[7][2] = std::make_unique<Bishop>(PieceColor::BLACK);
            board[7][3] = std::make_unique<Queen>(PieceColor::BLACK);
            board[7][4] = std::make_unique<King>(PieceColor::BLACK);
            board[7][5] = std::make_unique<Bishop>(PieceColor::BLACK);
            board[7][6] = std::make_unique<Knight>(PieceColor::BLACK);
            board[7][7] = std::make_unique<Rook>(PieceColor::BLACK);
            
            for (int c = 0; c < 8; ++c) {
                board[6][c] = std::make_unique<Pawn>(PieceColor::BLACK);
            }
        } catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("ChessBoard::initialize() - Exception placing pieces: " + std::string(e.what()));
            }
            throw std::runtime_error("Error placing chess pieces: " + std::string(e.what()));
        } catch (...) {
            if (server && server->getLogger()) {
                server->getLogger()->error("ChessBoard::initialize() - Unknown exception placing pieces");
            }
            throw std::runtime_error("Unknown error placing chess pieces");
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::initialize() - All pieces placed, resetting game state");
        }
        
        // Reset state
        currentTurn = PieceColor::WHITE;
        enPassantTarget = Position(-1, -1);
        moveHistory.clear();
        capturedWhitePieces.clear();
        capturedBlackPieces.clear();
        halfMoveClock = 0;
        boardStates.clear();
        
        // Add initial board state
        boardStates.push_back(getBoardStateString());
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::initialize() - Board initialization complete");
        }
    } catch (const std::exception& e) {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::initialize() - Fatal error: " + std::string(e.what()));
        }
        throw std::runtime_error("Error initializing board: " + std::string(e.what()));
    } catch (...) {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::initialize() - Unknown fatal error");
        }
        throw std::runtime_error("Unknown error initializing board");
    }
}

const ChessPiece* ChessBoard::getPiece(const Position& pos) const {
    if (!pos.isValid()) return nullptr;
    return board[pos.row][pos.col].get();
}

MoveValidationStatus ChessBoard::movePiece(const ChessMove& move, bool validateOnly)
{
    const Position& from = move.getFrom();
    const Position& to = move.getTo();
    
    // Check if positions are valid
    if (!from.isValid() || !to.isValid()) {
        return MoveValidationStatus::INVALID_DESTINATION;
    }
    
    // Check if there is a piece at the source position
    const ChessPiece* piece = getPiece(from);
    if (!piece) {
        return MoveValidationStatus::INVALID_PIECE;
    }
    
    // Check if it's the correct player's turn
    if (piece->getColor() != currentTurn) {
        return MoveValidationStatus::WRONG_TURN;
    }
    
    // Check if the move is valid for the piece
    std::vector<Position> possibleMoves = piece->getPossibleMoves(from, *this);
    if (std::find(possibleMoves.begin(), possibleMoves.end(), to) == possibleMoves.end()) {
        return MoveValidationStatus::INVALID_PATH;
    }
    
    // Check if the move would leave the king in check
    if (wouldLeaveInCheck(move, piece->getColor())) {
        return MoveValidationStatus::KING_IN_CHECK;
    }
    
    // If we're just validating, return now
    if (validateOnly) {
        return MoveValidationStatus::VALID;
    }
    
    // Store the captured piece (if any)
    const ChessPiece* capturedPiece = nullptr;
    if (isEnPassantCapture(move)) {
        int captureRow = (piece->getColor() == PieceColor::WHITE) ? to.row - 1 : to.row + 1;
        capturedPiece = getPiece(Position(captureRow, to.col));
    } else {
        capturedPiece = getPiece(to);
    }
    
    // Execute special moves
    if (isCastlingMove(move)) {
        executeCastlingMove(move);
    } else if (isEnPassantCapture(move)) {
        executeEnPassantCapture(move);
    } else {
        // Regular move
        // Handle promotion
        if (move.getPromotionType() != PieceType::EMPTY) {
            std::unique_ptr<ChessPiece> promotedPiece;
            switch (move.getPromotionType()) {
                case PieceType::QUEEN:
                    promotedPiece = std::make_unique<Queen>(piece->getColor());
                    break;
                case PieceType::ROOK:
                    promotedPiece = std::make_unique<Rook>(piece->getColor());
                    break;
                case PieceType::BISHOP:
                    promotedPiece = std::make_unique<Bishop>(piece->getColor());
                    break;
                case PieceType::KNIGHT:
                    promotedPiece = std::make_unique<Knight>(piece->getColor());
                    break;
                default:
                    promotedPiece = std::make_unique<Queen>(piece->getColor());
                    break;
            }
            promotedPiece->setMoved(true);
            board[to.row][to.col] = std::move(promotedPiece);
        } else {
            // Move the piece
            board[to.row][to.col] = std::move(board[from.row][from.col]);
            board[to.row][to.col]->setMoved(true);
        }
    }
    
    // Update state
    updateStateAfterMove(move, capturedPiece);

    // Clear caches since the board state has changed
    clearCaches();

    return MoveValidationStatus::VALID;
}

/*
bool ChessBoard::isUnderAttack(const Position& pos, PieceColor attackerColor) const
{
    // Start timing
    PerformanceMonitor::startTimer("ChessBoard::isUnderAttack");

    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger())
    {
        server->getLogger()->debug("ChessBoard::isUnderAttack() - Checking if position " + 
                                  pos.toAlgebraic() + " is under attack by " + 
                                  (attackerColor == PieceColor::WHITE ? "white" : "black"));
    }

    // Check recursion depth
    if (!incrementRecursionDepth("isUnderAttack")) {
        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after recursion depth check: " + 
                                    std::to_string(duration) + "ms");
        }

        return false;
    }
    
    try {
        // Check cache first
        std::string cacheKey = generateAttackCacheKey(pos, attackerColor);
        auto cacheIt = attackCache.find(cacheKey);
        if (cacheIt != attackCache.end()) {
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessBoard::isUnderAttack() - Cache hit for position " + 
                                          pos.toAlgebraic());
            }
            decrementRecursionDepth();

            // End timing and log
            double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after cacheCheck: " + 
                                        std::to_string(duration) + "ms");
            }

            return cacheIt->second;
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::isUnderAttack() - Checking if position " + 
                                      pos.toAlgebraic() + " is under attack by " + 
                                      (attackerColor == PieceColor::WHITE ? "white" : "black"));
        }

        // Check pawn attacks first (most common and simple to check)
        int pawnDirection = (attackerColor == PieceColor::WHITE) ? 1 : -1;
        Position leftAttacker(pos.row - pawnDirection, pos.col - 1);
        Position rightAttacker(pos.row - pawnDirection, pos.col + 1);
        
        // Check left pawn attack
        if (leftAttacker.isValid()) {
            const ChessPiece* piece = getPiece(leftAttacker);
            if (piece && piece->getType() == PieceType::PAWN && piece->getColor() == attackerColor) {
                if (server && server->getLogger()) {
                    server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a pawn at " + 
                                              leftAttacker.toAlgebraic());
                }
                attackCache[cacheKey] = true;
                decrementRecursionDepth();

                // End timing and log
                double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after left pawn attack check: " + 
                                            std::to_string(duration) + "ms");
                }

                return true;
            }
        }
        
        // Check right pawn attack
        if (rightAttacker.isValid()) {
            const ChessPiece* piece = getPiece(rightAttacker);
            if (piece && piece->getType() == PieceType::PAWN && piece->getColor() == attackerColor) {
                if (server && server->getLogger()) {
                    server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a pawn at " + 
                                              rightAttacker.toAlgebraic());
                }
                attackCache[cacheKey] = true;
                decrementRecursionDepth();

                // End timing and log
                double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after right pawn attack check: " + 
                                            std::to_string(duration) + "ms");
                }

                return true;
            }
        }
        
        // Knight attacks (fixed offsets)
        const std::vector<std::pair<int, int>> knightOffsets = {
            {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
            {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
        };
        
        for (const auto& offset : knightOffsets) {
            Position attackerPos(pos.row + offset.first, pos.col + offset.second);
            if (attackerPos.isValid()) {
                const ChessPiece* piece = getPiece(attackerPos);
                if (piece && piece->getType() == PieceType::KNIGHT && piece->getColor() == attackerColor) {
                    if (server && server->getLogger()) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a knight at " + 
                                                  attackerPos.toAlgebraic());
                    }
                    attackCache[cacheKey] = true;
                    decrementRecursionDepth();

                    // End timing and log
                    double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after Knight attack check: " + 
                                                std::to_string(duration) + "ms");
                    }

                    return true;
                }
            }
        }
        
        // King attacks (adjacent squares)
        const std::vector<std::pair<int, int>> kingOffsets = {
            {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
        };
        
        for (const auto& offset : kingOffsets) {
            Position attackerPos(pos.row + offset.first, pos.col + offset.second);
            if (attackerPos.isValid()) {
                const ChessPiece* piece = getPiece(attackerPos);
                if (piece && piece->getType() == PieceType::KING && piece->getColor() == attackerColor) {
                    if (server && server->getLogger()) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a king at " + 
                                                  attackerPos.toAlgebraic());
                    }
                    attackCache[cacheKey] = true;
                    decrementRecursionDepth();

                    // End timing and log
                    double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after left King attack check: " + 
                                                std::to_string(duration) + "ms");
                    }

                    return true;
                }
            }
        }
        
        // Sliding piece attacks (bishop, rook, queen)
        // Define direction vectors for all 8 directions
        const std::vector<std::pair<int, int>> directions = {
            {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
        };
        
        for (const auto& dir : directions) {
            for (int dist = 1; dist < 8; ++dist) {
                Position attackerPos(pos.row + dir.first * dist, pos.col + dir.second * dist);
                if (!attackerPos.isValid()) break;
                
                const ChessPiece* piece = getPiece(attackerPos);
                if (!piece) continue;  // Empty square, continue in this direction
                
                if (piece->getColor() != attackerColor) break;  // Blocked by opponent piece
                
                bool isDiagonal = dir.first != 0 && dir.second != 0;
                bool isOrthogonal = dir.first == 0 || dir.second == 0;
                
                // Check if the piece can attack in this direction
                if ((isDiagonal && (piece->getType() == PieceType::BISHOP || piece->getType() == PieceType::QUEEN)) ||
                    (isOrthogonal && (piece->getType() == PieceType::ROOK || piece->getType() == PieceType::QUEEN))) {
                    if (server && server->getLogger()) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a " + 
                                                 std::string(piece->getType() == PieceType::BISHOP ? "bishop" : 
                                                            piece->getType() == PieceType::ROOK ? "rook" : "queen") + 
                                                 " at " + attackerPos.toAlgebraic());
                    }
                    attackCache[cacheKey] = true;
                    decrementRecursionDepth();

                    // End timing and log
                    double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after left sliding piece attack check: " + 
                                                std::to_string(duration) + "ms");
                    }

                    return true;
                }
                
                break;  // Blocked by a piece that can't attack in this direction
            }
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::isUnderAttack() - Position " + pos.toAlgebraic() + 
                                      " is not under attack");
        }
        
        // Cache the negative result
        attackCache[cacheKey] = false;
        decrementRecursionDepth();

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after all attack checks: " + 
                                    std::to_string(duration) + "ms");
        }

        return false;
    }
    catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isUnderAttack() - Exception: " + std::string(e.what()));
        }
        decrementRecursionDepth();
        return false;
    }
    catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isUnderAttack() - Unknown exception");
        }
        decrementRecursionDepth();
        return false;
    }
}
*/

bool ChessBoard::isUnderAttack(const Position& pos, PieceColor attackerColor) const
{
    // Start timing
    PerformanceMonitor::startTimer("ChessBoard::isUnderAttack");

    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3)
    {
        server->getLogger()->debug("ChessBoard::isUnderAttack() - Checking if position " + 
                                  pos.toAlgebraic() + " is under attack by " + 
                                  (attackerColor == PieceColor::WHITE ? "white" : "black"));
    }

    // Check recursion depth
    if (!incrementRecursionDepth("isUnderAttack")) {
        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after recursion depth check: " + 
                                    std::to_string(duration) + "ms");
        }

        return false;
    }
    
    try {
        // Check cache first
        std::string cacheKey = generateAttackCacheKey(pos, attackerColor);
        auto cacheIt = attackCache.find(cacheKey);
        if (cacheIt != attackCache.end()) {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::isUnderAttack() - Cache hit for position " + 
                                          pos.toAlgebraic());
            }
            decrementRecursionDepth();

            // End timing and log
            double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after cacheCheck: " + 
                                        std::to_string(duration) + "ms");
            }

            return cacheIt->second;
        }
        
        // Check pawn attacks first (most common and simple to check)
        int pawnDirection = (attackerColor == PieceColor::WHITE) ? 1 : -1;
        Position leftAttacker(pos.row - pawnDirection, pos.col - 1);
        Position rightAttacker(pos.row - pawnDirection, pos.col + 1);
        
        // Check left pawn attack
        if (leftAttacker.isValid()) {
            const ChessPiece* piece = getPiece(leftAttacker);
            if (piece && piece->getType() == PieceType::PAWN && piece->getColor() == attackerColor) {
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a pawn at " + 
                                              leftAttacker.toAlgebraic());
                }
                attackCache[cacheKey] = true;
                decrementRecursionDepth();

                // End timing and log
                double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after left pawn attack check: " + 
                                            std::to_string(duration) + "ms");
                }

                return true;
            }
        }
        
        // Check right pawn attack
        if (rightAttacker.isValid()) {
            const ChessPiece* piece = getPiece(rightAttacker);
            if (piece && piece->getType() == PieceType::PAWN && piece->getColor() == attackerColor) {
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a pawn at " + 
                                              rightAttacker.toAlgebraic());
                }
                attackCache[cacheKey] = true;
                decrementRecursionDepth();

                // End timing and log
                double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after right pawn attack check: " + 
                                            std::to_string(duration) + "ms");
                }

                return true;
            }
        }
        
        // Knight attacks (fixed offsets)
        const std::vector<std::pair<int, int>> knightOffsets = {
            {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
            {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
        };
        
        for (const auto& offset : knightOffsets) {
            Position attackerPos(pos.row + offset.first, pos.col + offset.second);
            if (attackerPos.isValid()) {
                const ChessPiece* piece = getPiece(attackerPos);
                if (piece && piece->getType() == PieceType::KNIGHT && piece->getColor() == attackerColor) {
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a knight at " + 
                                                  attackerPos.toAlgebraic());
                    }
                    attackCache[cacheKey] = true;
                    decrementRecursionDepth();

                    // End timing and log
                    double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after Knight attack check: " + 
                                                std::to_string(duration) + "ms");
                    }

                    return true;
                }
            }
        }
        
        // King attacks (adjacent squares)
        const std::vector<std::pair<int, int>> kingOffsets = {
            {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
        };
        
        for (const auto& offset : kingOffsets) {
            Position attackerPos(pos.row + offset.first, pos.col + offset.second);
            if (attackerPos.isValid()) {
                const ChessPiece* piece = getPiece(attackerPos);
                if (piece && piece->getType() == PieceType::KING && piece->getColor() == attackerColor) {
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a king at " + 
                                                  attackerPos.toAlgebraic());
                    }
                    attackCache[cacheKey] = true;
                    decrementRecursionDepth();

                    // End timing and log
                    double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after King attack check: " + 
                                                std::to_string(duration) + "ms");
                    }

                    return true;
                }
            }
        }
        
        // Sliding piece attacks (bishop, rook, queen)
        // Define direction vectors for all 8 directions
        const std::vector<std::pair<int, int>> directions = {
            {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
        };
        
        for (const auto& dir : directions) {
            for (int dist = 1; dist < 8; ++dist) {
                Position attackerPos(pos.row + dir.first * dist, pos.col + dir.second * dist);
                if (!attackerPos.isValid()) break;
                
                const ChessPiece* piece = getPiece(attackerPos);
                if (!piece) continue;  // Empty square, continue in this direction
                
                if (piece->getColor() != attackerColor) break;  // Blocked by opponent piece
                
                bool isDiagonal = dir.first != 0 && dir.second != 0;
                bool isOrthogonal = dir.first == 0 || dir.second == 0;
                
                // Check if the piece can attack in this direction
                if ((isDiagonal && (piece->getType() == PieceType::BISHOP || piece->getType() == PieceType::QUEEN)) ||
                    (isOrthogonal && (piece->getType() == PieceType::ROOK || piece->getType() == PieceType::QUEEN))) {
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Position is under attack by a " + 
                                                 std::string(piece->getType() == PieceType::BISHOP ? "bishop" : 
                                                            piece->getType() == PieceType::ROOK ? "rook" : "queen") + 
                                                 " at " + attackerPos.toAlgebraic());
                    }
                    attackCache[cacheKey] = true;
                    decrementRecursionDepth();

                    // End timing and log
                    double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
                    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                        server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after sliding piece attack check: " + 
                                                std::to_string(duration) + "ms");
                    }

                    return true;
                }
                
                break;  // Blocked by a piece that can't attack in this direction
            }
        }
        
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isUnderAttack() - Position " + pos.toAlgebraic() + 
                                      " is not under attack");
        }
        
        // Cache the negative result
        attackCache[cacheKey] = false;
        decrementRecursionDepth();

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isUnderAttack");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isUnderAttack() - Execution time after all attack checks: " + 
                                    std::to_string(duration) + "ms");
        }

        return false;
    }
    catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isUnderAttack() - Exception: " + std::string(e.what()));
        }
        decrementRecursionDepth();
        return false;
    }
    catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isUnderAttack() - Unknown exception");
        }
        decrementRecursionDepth();
        return false;
    }
}

bool ChessBoard::incrementRecursionDepth(const std::string& functionName) const
{
    MPChessServer* server = MPChessServer::getInstance();
    std::thread::id threadId = std::this_thread::get_id();
    
    // Initialize if not present
    if (recursionDepth.find(threadId) == recursionDepth.end()) {
        recursionDepth[threadId] = 0;
    }
    
    recursionDepth[threadId]++;
    
    if (recursionDepth[threadId] > MAX_RECURSION_DEPTH) {
        if (server && server->getLogger()) {
            server->getLogger()->warning("ChessBoard::" + functionName + 
                                       " - Maximum recursion depth exceeded: " + 
                                       std::to_string(recursionDepth[threadId]));
        }
        recursionDepth[threadId]--;
        return false;
    }
    
    return true;
}

void ChessBoard::decrementRecursionDepth() const
{
    std::thread::id threadId = std::this_thread::get_id();
    if (recursionDepth.find(threadId) != recursionDepth.end() && recursionDepth[threadId] > 0) {
        recursionDepth[threadId]--;
    }
}

std::string ChessBoard::generateCheckCacheKey(PieceColor color) const
{
    // Generate a unique key for the current board state and color
    std::string key = getBoardStateString() + "_check_" + 
                     (color == PieceColor::WHITE ? "white" : "black");
    return key;
}

std::string ChessBoard::generateAttackCacheKey(const Position& pos, PieceColor attackerColor) const
{
    // Generate a unique key for the current board state, position, and attacker color
    std::string key = getBoardStateString() + "_attack_" + 
                     pos.toAlgebraic() + "_" +
                     (attackerColor == PieceColor::WHITE ? "white" : "black");
    return key;
}

void ChessBoard::clearCaches() const
{
    checkCache.clear();
    attackCache.clear();
    checkResultCache.clear();
}

void ChessBoard::recordBoardDelta(const Position& pos) const
{
    if (!pos.isValid()) return;
    
    // Check if this position is already in the delta
    for (auto& delta : lastMoveDelta) {
        if (delta.position == pos) {
            // Already recorded
            return;
        }
    }
    
    // Record the current state at this position
    BoardDelta delta;
    delta.position = pos;
    delta.isModified = false;
    
    const ChessPiece* piece = getPiece(pos);
    if (piece) {
        delta.oldPiece = piece->clone();
    }
    
    lastMoveDelta.push_back(std::move(delta));
}

void ChessBoard::clearBoardDelta() const
{
    lastMoveDelta.clear();
}

void ChessBoard::restoreBoardDelta() const
{
    MPChessServer* server = MPChessServer::getInstance();
    
    if (server && server->getLogger()) {
        server->getLogger()->debug("ChessBoard::restoreBoardDelta() - Restoring " + 
                                  std::to_string(lastMoveDelta.size()) + " board positions");
    }
    
    for (auto& delta : lastMoveDelta) {
        if (delta.isModified) {
            // Restore the old piece
            // We need to cast away const-ness here since we're modifying the board in a const method
            // This is safe because we're restoring the original state
            auto& boardCell = const_cast<std::unique_ptr<ChessPiece>&>(board[delta.position.row][delta.position.col]);
            boardCell = std::move(delta.oldPiece);
        }
    }
    
    clearBoardDelta();
}

/*
bool ChessBoard::isInCheck(PieceColor color) const
{
    // Start timing
    PerformanceMonitor::startTimer("ChessBoard::isInCheck");

    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger())
    {
        server->getLogger()->debug("ChessBoard::isInCheck() - Checking if " + 
                                  std::string(color == PieceColor::WHITE ? "white" : "black") + 
                                  " king is in check");
    }
    
    // Check recursion depth
    if (!incrementRecursionDepth("isInCheck")) {
        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after recursion depth check: " + 
                                    std::to_string(duration) + "ms");
        }
        return false;
    }
    
    try {
        // Check cache first
        std::string cacheKey = generateCheckCacheKey(color);
        auto cacheIt = checkCache.find(cacheKey);
        if (cacheIt != checkCache.end()) {
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessBoard::isInCheck() - Cache hit for " + 
                                          std::string(color == PieceColor::WHITE ? "white" : "black"));
            }
            decrementRecursionDepth();

            // End timing and log
            double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after checkCache: " + 
                                        std::to_string(duration) + "ms");
            }

            return cacheIt->second;
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Checking if " + 
                                      std::string(color == PieceColor::WHITE ? "white" : "black") + 
                                      " king is in check");
        }
        
        // Find the king position
        Position kingPos = getKingPosition(color);
        if (!kingPos.isValid()) {
            if (server && server->getLogger()) {
                server->getLogger()->warning("ChessBoard::isInCheck() - King position is invalid");
            }
            decrementRecursionDepth();

            // End timing and log
            double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after kingPosInvalid: " + 
                                        std::to_string(duration) + "ms");
            }

            return false;
        }
        
        // Check if the king is under attack by the opposite color
        PieceColor opponentColor = (color == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
        bool result = isUnderAttack(kingPos, opponentColor);
        
        // Cache the result
        checkCache[cacheKey] = result;
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::isInCheck() - " + 
                                      std::string(color == PieceColor::WHITE ? "White" : "Black") + 
                                      " king is " + (result ? "in check" : "not in check"));
        }
        
        decrementRecursionDepth();

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after isUnderAttack check for king: " + 
                                    std::to_string(duration) + "ms");
        }

        return result;
    }
    catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isInCheck() - Exception: " + std::string(e.what()));
        }
        decrementRecursionDepth();

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after inner exception handler: " + 
                                    std::to_string(duration) + "ms");
        }

        return false;
    }
    catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isInCheck() - Unknown exception");
        }
        decrementRecursionDepth();

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after outer exception handler: " + 
                                    std::to_string(duration) + "ms");
        }

        return false;
    }
}
*/

bool ChessBoard::isInCheck(PieceColor color) const
{
    // Start timing
    PerformanceMonitor::startTimer("ChessBoard::isInCheck");

    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3)
    {
        server->getLogger()->debug("ChessBoard::isInCheck() - Checking if " + 
                                  std::string(color == PieceColor::WHITE ? "white" : "black") + 
                                  " king is in check");
    }
    
    // Check recursion depth
    if (!incrementRecursionDepth("isInCheck")) {
        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after recursion depth check: " + 
                                    std::to_string(duration) + "ms");
        }
        return false;
    }
    
    try {
        // Check cache first
        std::string cacheKey = generateCheckCacheKey(color);
        auto cacheIt = checkCache.find(cacheKey);
        if (cacheIt != checkCache.end()) {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::isInCheck() - Cache hit for " + 
                                          std::string(color == PieceColor::WHITE ? "white" : "black"));
            }
            decrementRecursionDepth();

            // End timing and log
            double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after checkCache: " + 
                                        std::to_string(duration) + "ms");
            }

            return cacheIt->second;
        }
        
        // Find the king position
        Position kingPos = getKingPosition(color);
        if (!kingPos.isValid()) {
            if (server && server->getLogger()) {
                server->getLogger()->warning("ChessBoard::isInCheck() - King position is invalid");
            }
            decrementRecursionDepth();

            // End timing and log
            double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after kingPosInvalid: " + 
                                        std::to_string(duration) + "ms");
            }

            return false;
        }
        
        // Check if the king is under attack by the opposite color
        PieceColor opponentColor = (color == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
        bool result = isUnderAttack(kingPos, opponentColor);
        
        // Cache the result
        checkCache[cacheKey] = result;
        
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - " + 
                                      std::string(color == PieceColor::WHITE ? "White" : "Black") + 
                                      " king is " + (result ? "in check" : "not in check"));
        }
        
        decrementRecursionDepth();

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after isUnderAttack check for king: " + 
                                    std::to_string(duration) + "ms");
        }

        return result;
    }
    catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isInCheck() - Exception: " + std::string(e.what()));
        }
        decrementRecursionDepth();

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after inner exception handler: " + 
                                    std::to_string(duration) + "ms");
        }

        return false;
    }
    catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isInCheck() - Unknown exception");
        }
        decrementRecursionDepth();

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::isInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::isInCheck() - Execution time after outer exception handler: " + 
                                    std::to_string(duration) + "ms");
        }

        return false;
    }
}

bool ChessBoard::isInCheckmate(PieceColor color) const
{
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger())
    {
        server->getLogger()->debug("ChessBoard::isInCheckmate() - Checking if " + 
                                  std::string(color == PieceColor::WHITE ? "white" : "black") + 
                                  " is in checkmate");
    }

    try {
        // First check if the king is in check
        if (!isInCheck(color)) {
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessBoard::isInCheckmate() - King is not in check, so not checkmate");
            }
            return false;
        }
        
        // Check if any move can get the king out of check
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                Position pos(r, c);
                const ChessPiece* piece = getPiece(pos);
                if (!piece || piece->getColor() != color) continue;
                
                // Get possible moves for this piece
                std::vector<Position> moves;
                try {
                    moves = piece->getPossibleMoves(pos, *this);
                }
                catch (const std::exception& e) {
                    if (server && server->getLogger()) {
                        server->getLogger()->error("ChessBoard::isInCheckmate() - Exception getting moves: " + 
                                                 std::string(e.what()));
                    }
                    continue;
                }
                
                // Check if any move can get out of check
                for (const Position& to : moves) {
                    ChessMove move(pos, to);
                    if (!wouldLeaveInCheck(move, color)) {
                        if (server && server->getLogger()) {
                            server->getLogger()->debug("ChessBoard::isInCheckmate() - Found escape move: " + 
                                                     pos.toAlgebraic() + " to " + to.toAlgebraic());
                        }
                        return false;
                    }
                }
            }
        }
        
        // If we get here, no move can escape check
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::isInCheckmate() - No escape moves found, it's checkmate");
        }
        return true;
    }
    catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isInCheckmate() - Exception: " + std::string(e.what()));
        }
        return false;
    }
    catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::isInCheckmate() - Unknown exception");
        }
        return false;
    }
}

bool ChessBoard::isInStalemate(PieceColor color) const
{
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger())
    {
        server->getLogger()->debug("ChessBoard::isInStalemate() - Checking if game is in stalemate");
    }

    if (isInCheck(color))
    {
        if (server && server->getLogger())
        {
            server->getLogger()->debug("ChessBoard::isInStalemate() - Checking, we're not in check; returning...");
        }
        return false;
    }
    
    // Check if any legal move is available
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Position pos(r, c);
            const ChessPiece* piece = getPiece(pos);
            if (piece && piece->getColor() == color) {
                std::vector<Position> moves = piece->getPossibleMoves(pos, *this);
                for (const Position& to : moves) {
                    ChessMove move(pos, to);
                    if (!wouldLeaveInCheck(move, color))
                    {
                        if (server && server->getLogger())
                        {
                            server->getLogger()->debug("ChessBoard::isInStalemate() - Its not wouldLeaveInCheck(), returning...");
                        }
                        return false;
                    }
                }
            }
        }
    }

    if (server && server->getLogger())
    {
        server->getLogger()->debug("ChessBoard::isInStalemate() - Yes, it is.");
    }
    
    return true;
}

std::vector<ChessMove> ChessBoard::getAllValidMoves(PieceColor color) const
{
    // Start timing
    PerformanceMonitor::startTimer("ChessBoard::getAllValidMoves");
    
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger())
    {
        server->getLogger()->debug("ChessBoard::getAllValidMoves() - Getting all valid moves for " +
                                  std::string(color == PieceColor::WHITE ? "white" : "black"));
    }

    std::vector<ChessMove> validMoves;
    
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Position pos(r, c);
            const ChessPiece* piece = getPiece(pos);
            if (piece && piece->getColor() == color) {
                std::vector<Position> moves = piece->getPossibleMoves(pos, *this);
                for (const Position& to : moves) {
                    ChessMove move(pos, to);
                    if (!wouldLeaveInCheck(move, color)) {
                        // Check for pawn promotion
                        if (piece->getType() == PieceType::PAWN && 
                            ((color == PieceColor::WHITE && to.row == 7) || 
                             (color == PieceColor::BLACK && to.row == 0))) {
                            // Add all promotion options
                            ChessMove promoteToQueen = move;
                            promoteToQueen.setPromotionType(PieceType::QUEEN);
                            validMoves.push_back(promoteToQueen);
                            
                            ChessMove promoteToRook = move;
                            promoteToRook.setPromotionType(PieceType::ROOK);
                            validMoves.push_back(promoteToRook);
                            
                            ChessMove promoteToBishop = move;
                            promoteToBishop.setPromotionType(PieceType::BISHOP);
                            validMoves.push_back(promoteToBishop);
                            
                            ChessMove promoteToKnight = move;
                            promoteToKnight.setPromotionType(PieceType::KNIGHT);
                            validMoves.push_back(promoteToKnight);
                        } else {
                            validMoves.push_back(move);
                        }
                    }
                }
            }
        }
    }

    // End timing and log
    double duration = PerformanceMonitor::endTimer("ChessBoard::getAllValidMoves");
    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
        server->getLogger()->debug("ChessBoard::getAllValidMoves() - Found " + 
                                  std::to_string(validMoves.size()) + " valid moves in " + 
                                  std::to_string(duration) + "ms");
    }

    return validMoves;
}

Position ChessBoard::getKingPosition(PieceColor color) const {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Position pos(r, c);
            const ChessPiece* piece = getPiece(pos);
            if (piece && piece->getType() == PieceType::KING && piece->getColor() == color) {
                return pos;
            }
        }
    }
    return Position(-1, -1);
}

bool ChessBoard::isCastlingMove(const ChessMove& move) const {
    const ChessPiece* piece = getPiece(move.getFrom());
    if (!piece || piece->getType() != PieceType::KING) return false;
    
    int colDiff = move.getTo().col - move.getFrom().col;
    return std::abs(colDiff) == 2;
}

bool ChessBoard::isEnPassantCapture(const ChessMove& move) const {
    const ChessPiece* piece = getPiece(move.getFrom());
    if (!piece || piece->getType() != PieceType::PAWN) return false;
    
    // Check if the move is a diagonal move to an empty square
    if (move.getFrom().col != move.getTo().col && getPiece(move.getTo()) == nullptr) {
        return move.getTo() == enPassantTarget;
    }
    
    return false;
}

Position ChessBoard::getEnPassantTarget() const {
    return enPassantTarget;
}

void ChessBoard::setEnPassantTarget(const Position& pos) {
    enPassantTarget = pos;
}

std::string ChessBoard::getAsciiBoard() const {
    std::stringstream ss;
    
    ss << "  +---+---+---+---+---+---+---+---+\n";
    
    for (int r = 7; r >= 0; --r) {
        ss << (r + 1) << " |";
        
        for (int c = 0; c < 8; ++c) {
            const ChessPiece* piece = getPiece(Position(r, c));
            char pieceChar = piece ? piece->getAsciiChar() : ' ';
            ss << " " << pieceChar << " |";
        }
        
        ss << "\n  +---+---+---+---+---+---+---+---+\n";
    }
    
    ss << "    a   b   c   d   e   f   g   h  \n";
    
    return ss.str();
}

std::unique_ptr<ChessBoard> ChessBoard::clone() const
{
    auto cloneBoard = std::make_unique<ChessBoard>();
    
    // Clear the cloned board
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            cloneBoard->board[r][c] = nullptr;
        }
    }
    
    // Copy pieces
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const ChessPiece* piece = getPiece(Position(r, c));
            if (piece) {
                cloneBoard->board[r][c] = piece->clone();
            }
        }
    }
    
    // Copy state
    cloneBoard->currentTurn = currentTurn;
    cloneBoard->enPassantTarget = enPassantTarget;
    cloneBoard->moveHistory = moveHistory;
    cloneBoard->capturedWhitePieces = capturedWhitePieces;
    cloneBoard->capturedBlackPieces = capturedBlackPieces;
    cloneBoard->halfMoveClock = halfMoveClock;
    cloneBoard->boardStates = boardStates;
    
    return cloneBoard;
}

PieceColor ChessBoard::getCurrentTurn() const {
    return currentTurn;
}

void ChessBoard::setCurrentTurn(PieceColor color) {
    currentTurn = color;
}

const std::vector<ChessMove>& ChessBoard::getMoveHistory() const {
    return moveHistory;
}

const std::vector<PieceType>& ChessBoard::getCapturedPieces(PieceColor color) const {
    return (color == PieceColor::WHITE) ? capturedWhitePieces : capturedBlackPieces;
}

bool ChessBoard::isGameOver() const {
    return isInCheckmate(PieceColor::WHITE) || isInCheckmate(PieceColor::BLACK) ||
           isInStalemate(PieceColor::WHITE) || isInStalemate(PieceColor::BLACK) ||
           canClaimThreefoldRepetition() || canClaimFiftyMoveRule() || hasInsufficientMaterial();
}

GameResult ChessBoard::getGameResult() const {
    if (isInCheckmate(PieceColor::WHITE)) {
        return GameResult::BLACK_WIN;
    } else if (isInCheckmate(PieceColor::BLACK)) {
        return GameResult::WHITE_WIN;
    } else if (isInStalemate(PieceColor::WHITE) || isInStalemate(PieceColor::BLACK) ||
               canClaimThreefoldRepetition() || canClaimFiftyMoveRule() || hasInsufficientMaterial()) {
        return GameResult::DRAW;
    }
    
    return GameResult::IN_PROGRESS;
}

bool ChessBoard::canClaimThreefoldRepetition() const
{
    if (boardStates.empty()) return false;
    
    std::string currentState = boardStates.back();
    int count = 0;
    
    for (const std::string& state : boardStates) {
        if (state == currentState) {
            count++;
        }
    }
    
    return count >= 3;
}

bool ChessBoard::canClaimFiftyMoveRule() const
{
    return halfMoveClock >= 100;  // 50 full moves = 100 half moves
}

bool ChessBoard::hasInsufficientMaterial() const
{
    int whitePieceCount = 0;
    int blackPieceCount = 0;
    bool whiteHasKnight = false;
    bool blackHasKnight = false;
    bool whiteHasBishop = false;
    bool blackHasBishop = false;
    int whiteBishopColor = -1;  // -1: none, 0: light square, 1: dark square
    int blackBishopColor = -1;
    
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const ChessPiece* piece = getPiece(Position(r, c));
            if (!piece) continue;
            
            if (piece->getColor() == PieceColor::WHITE) {
                whitePieceCount++;
                if (piece->getType() == PieceType::KNIGHT) whiteHasKnight = true;
                if (piece->getType() == PieceType::BISHOP) {
                    whiteHasBishop = true;
                    whiteBishopColor = (r + c) % 2;
                }
                if (piece->getType() == PieceType::PAWN ||
                    piece->getType() == PieceType::ROOK ||
                    piece->getType() == PieceType::QUEEN) {
                    return false;  // Sufficient material
                }
            } else {
                blackPieceCount++;
                if (piece->getType() == PieceType::KNIGHT) blackHasKnight = true;
                if (piece->getType() == PieceType::BISHOP) {
                    blackHasBishop = true;
                    blackBishopColor = (r + c) % 2;
                }
                if (piece->getType() == PieceType::PAWN ||
                    piece->getType() == PieceType::ROOK ||
                    piece->getType() == PieceType::QUEEN) {
                    return false;  // Sufficient material
                }
            }
        }
    }
    
    // King vs King
    if (whitePieceCount == 1 && blackPieceCount == 1) {
        return true;
    }
    
    // King + Bishop vs King or King + Knight vs King
    if ((whitePieceCount == 2 && blackPieceCount == 1 && (whiteHasBishop || whiteHasKnight)) ||
        (whitePieceCount == 1 && blackPieceCount == 2 && (blackHasBishop || blackHasKnight))) {
        return true;
    }
    
    // King + Bishop vs King + Bishop (same colored bishops)
    if (whitePieceCount == 2 && blackPieceCount == 2 && whiteHasBishop && blackHasBishop &&
        whiteBishopColor == blackBishopColor) {
        return true;
    }
    
    return false;
}

void ChessBoard::executeCastlingMove(const ChessMove& move)
{
    const Position& from = move.getFrom();
    const Position& to = move.getTo();
    
    // Move the king
    board[to.row][to.col] = std::move(board[from.row][from.col]);
    board[to.row][to.col]->setMoved(true);
    
    // Move the rook
    if (to.col > from.col) {
        // Kingside castling
        Position rookFrom(from.row, 7);
        Position rookTo(from.row, to.col - 1);
        board[rookTo.row][rookTo.col] = std::move(board[rookFrom.row][rookFrom.col]);
        board[rookTo.row][rookTo.col]->setMoved(true);
    } else {
        // Queenside castling
        Position rookFrom(from.row, 0);
        Position rookTo(from.row, to.col + 1);
        board[rookTo.row][rookTo.col] = std::move(board[rookFrom.row][rookFrom.col]);
        board[rookTo.row][rookTo.col]->setMoved(true);
    }
}

void ChessBoard::executeEnPassantCapture(const ChessMove& move)
{
    const Position& from = move.getFrom();
    const Position& to = move.getTo();
    
    // Move the pawn
    board[to.row][to.col] = std::move(board[from.row][from.col]);
    board[to.row][to.col]->setMoved(true);
    
    // Remove the captured pawn
    int captureRow = (currentTurn == PieceColor::WHITE) ? to.row - 1 : to.row + 1;
    board[captureRow][to.col] = nullptr;
}

void ChessBoard::updateStateAfterMove(const ChessMove& move, const ChessPiece* capturedPiece)
{
    // Update move history
    moveHistory.push_back(move);
    
    // Update captured pieces
    if (capturedPiece) {
        if (capturedPiece->getColor() == PieceColor::WHITE) {
            capturedWhitePieces.push_back(capturedPiece->getType());
        } else {
            capturedBlackPieces.push_back(capturedPiece->getType());
        }
    }
    
    // Update en passant target
    const ChessPiece* movedPiece = getPiece(move.getTo());
    if (movedPiece && movedPiece->getType() == PieceType::PAWN) {
        int rowDiff = move.getTo().row - move.getFrom().row;
        if (std::abs(rowDiff) == 2) {
            // Pawn moved two squares, set en passant target
            int enPassantRow = (move.getFrom().row + move.getTo().row) / 2;
            enPassantTarget = Position(enPassantRow, move.getFrom().col);
        } else {
            enPassantTarget = Position(-1, -1);
        }
    } else {
        enPassantTarget = Position(-1, -1);
    }
    
    // Update half-move clock
    if (movedPiece && movedPiece->getType() == PieceType::PAWN || capturedPiece) {
        halfMoveClock = 0;
    } else {
        halfMoveClock++;
    }
    
    // Update current turn
    currentTurn = (currentTurn == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
    
    // Update board states for repetition detection
    boardStates.push_back(getBoardStateString());
}

/*
bool ChessBoard::wouldLeaveInCheck(const ChessMove& move, PieceColor color) const
{
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger())
    {
        server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Checking move for " + 
                                  std::string(color == PieceColor::WHITE ? "white" : "black") + 
                                  "...");
    }
    
    // Check recursion depth
    if (!incrementRecursionDepth("wouldLeaveInCheck")) {
        // If we've exceeded max depth, conservatively assume it would leave in check
        return true;
    }
    
    try {
        // Create a clone of the board
        auto tempBoard = clone();
        
        // Execute the move without validation
        const Position& from = move.getFrom();
        const Position& to = move.getTo();
        
        // Handle special moves
        if (isCastlingMove(move)) {
            // For castling, we need to check if the king is in check at any point during the move
            int direction = (to.col > from.col) ? 1 : -1;
            
            // Check if the king is in check at the starting position
            if (tempBoard->isInCheck(color)) {
                decrementRecursionDepth();
                return true;
            }
            
            // Check if the king would be in check at any intermediate position
            Position midPos(from.row, from.col + direction);
            
            // Move the king to the intermediate position
            tempBoard->board[midPos.row][midPos.col] = std::move(tempBoard->board[from.row][from.col]);
            
            // Check if the king would be in check
            if (tempBoard->isInCheck(color)) {
                decrementRecursionDepth();
                return true;
            }
            
            // Move the king to the final position
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[midPos.row][midPos.col]);
        } else if (isEnPassantCapture(move)) {
            // Move the pawn
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[from.row][from.col]);
            
            // Remove the captured pawn
            int captureRow = (color == PieceColor::WHITE) ? to.row - 1 : to.row + 1;
            tempBoard->board[captureRow][to.col] = nullptr;
        } else {
            // Regular move
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[from.row][from.col]);
        }
        
        // Check if the king is in check after the move
        bool result = tempBoard->isInCheck(color);
        decrementRecursionDepth();
        return result;
    } catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::wouldLeaveInCheck() - Exception: " + std::string(e.what()));
        }
        decrementRecursionDepth();
        return true; // Conservatively assume it would leave in check
    } catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::wouldLeaveInCheck() - Unknown exception");
        }
        decrementRecursionDepth();
        return true;
    }
}
*/

/*
bool ChessBoard::wouldLeaveInCheck(const ChessMove& move, PieceColor color) const
{
    // Start timing
    PerformanceMonitor::startTimer("ChessBoard::wouldLeaveInCheck");

    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger())
    {
        server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Checking if move " + 
                                  move.toAlgebraic() + " would leave " + 
                                  std::string(color == PieceColor::WHITE ? "white" : "black") + 
                                  " king in check");
    }

    // Check recursion depth
    if (!incrementRecursionDepth("wouldLeaveInCheck")) {
        // If we've exceeded max depth, conservatively assume it would leave in check
        if (server && server->getLogger())
        {
            server->getLogger()->warning("ChessBoard::wouldLeaveInCheck() - Maximum recursion depth exceeded, assuming move would leave king in check");
        }

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Execution time after recursion depth check: " + 
                                    std::to_string(duration) + "ms");
        }

        decrementRecursionDepth();
        return true;
    }
    
    try {
        // Check cache first
        std::string cacheKey = generateCheckResultCacheKey(move, color);
        auto cacheIt = checkResultCache.find(cacheKey);
        if (cacheIt != checkResultCache.end()) {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Cache hit for move " + 
                                          move.toAlgebraic() + ", result: " + 
                                          (cacheIt->second ? "would leave in check" : "would not leave in check"));
            }
            
            // End timing for cache hit
            double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Cache hit execution time: " + 
                                        std::to_string(duration) + "ms");
            }
            
            decrementRecursionDepth();
            return cacheIt->second;
        }
        
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Cache miss, creating board clone");
        }
        
        // Create a clone of the board
        auto tempBoard = clone();
        
        // Execute the move without validation
        const Position& from = move.getFrom();
        const Position& to = move.getTo();
        
        // Handle special moves
        if (isCastlingMove(move)) {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Processing castling move");
            }
            
            // For castling, we need to check if the king is in check at any point during the move
            int direction = (to.col > from.col) ? 1 : -1;
            
            // Check if the king is in check at the starting position
            if (tempBoard->isInCheck(color)) {
                if (server && server->getLogger()) {
                    server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - King already in check, castling not allowed");
                }
                
                // Cache the result
                checkResultCache[cacheKey] = true;
                
                // End timing
                double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Execution time (king already in check): " + 
                                            std::to_string(duration) + "ms");
                }
                
                decrementRecursionDepth();
                return true;
            }
            
            // Check if the king would be in check at any intermediate position
            Position midPos(from.row, from.col + direction);
            
            // Move the king to the intermediate position
            tempBoard->board[midPos.row][midPos.col] = std::move(tempBoard->board[from.row][from.col]);
            if (tempBoard->board[midPos.row][midPos.col]) {
                tempBoard->board[midPos.row][midPos.col]->setMoved(true);
            }
            
            // Check if the king would be in check at the intermediate position
            if (tempBoard->isInCheck(color)) {
                if (server && server->getLogger()) {
                    server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - King would pass through check during castling");
                }
                
                // Cache the result
                checkResultCache[cacheKey] = true;
                
                // End timing
                double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Execution time (king passes through check): " + 
                                            std::to_string(duration) + "ms");
                }
                
                decrementRecursionDepth();
                return true;
            }
            
            // Move the king to the final position
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[midPos.row][midPos.col]);
            if (tempBoard->board[to.row][to.col]) {
                tempBoard->board[to.row][to.col]->setMoved(true);
            }
            
            // Move the rook
            int rookFromCol = (direction > 0) ? 7 : 0;
            int rookToCol = (direction > 0) ? (to.col - 1) : (to.col + 1);
            
            Position rookFrom(from.row, rookFromCol);
            Position rookTo(from.row, rookToCol);
            tempBoard->board[rookTo.row][rookTo.col] = std::move(tempBoard->board[rookFrom.row][rookFrom.col]);
            if (tempBoard->board[rookTo.row][rookTo.col]) {
                tempBoard->board[rookTo.row][rookTo.col]->setMoved(true);
            }
            
        } else if (isEnPassantCapture(move)) {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Processing en passant capture");
            }
            
            // Move the pawn
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[from.row][from.col]);
            if (tempBoard->board[to.row][to.col]) {
                tempBoard->board[to.row][to.col]->setMoved(true);
            }
            
            // Remove the captured pawn
            int captureRow = (color == PieceColor::WHITE) ? to.row - 1 : to.row + 1;
            tempBoard->board[captureRow][to.col] = nullptr;
            
        } else {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Processing regular move");
            }
            
            // Regular move
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[from.row][from.col]);
            if (tempBoard->board[to.row][to.col]) {
                tempBoard->board[to.row][to.col]->setMoved(true);
            }
        }
        
        // Check if the king is in check after the move
        bool result = tempBoard->isInCheck(color);
        
        // Cache the result
        checkResultCache[cacheKey] = result;
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Move " + 
                                      move.toAlgebraic() + " would " + 
                                      (result ? "leave king in check" : "not leave king in check"));
        }
        
        decrementRecursionDepth();
    
        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Total execution time: " + 
                                    std::to_string(duration) + "ms");
        }
        
        return result;
    
    } catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::wouldLeaveInCheck() - Exception: " + std::string(e.what()));
        }
        
        decrementRecursionDepth();
        return true; // Conservatively assume it would leave in check
    } catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::wouldLeaveInCheck() - Unknown exception");
        }
        
        decrementRecursionDepth();        
        return true; // Conservatively assume it would leave in check
    }
}
*/

bool ChessBoard::wouldLeaveInCheck(const ChessMove& move, PieceColor color) const
{
    // Start timing
    PerformanceMonitor::startTimer("ChessBoard::wouldLeaveInCheck");

    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3)
    {
        server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Checking if move " + 
                                  move.toAlgebraic() + " would leave " + 
                                  std::string(color == PieceColor::WHITE ? "white" : "black") + 
                                  " king in check");
    }

    // Check recursion depth
    if (!incrementRecursionDepth("wouldLeaveInCheck")) {
        // If we've exceeded max depth, conservatively assume it would leave in check
        if (server && server->getLogger())
        {
            server->getLogger()->warning("ChessBoard::wouldLeaveInCheck() - Maximum recursion depth exceeded, assuming move would leave king in check");
        }

        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Execution time after recursion depth check: " + 
                                    std::to_string(duration) + "ms");
        }

        decrementRecursionDepth();
        return true;
    }
    
    try {
        // Check cache first
        std::string cacheKey = generateCheckResultCacheKey(move, color);
        auto cacheIt = checkResultCache.find(cacheKey);
        if (cacheIt != checkResultCache.end()) {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Cache hit for move " + 
                                          move.toAlgebraic() + ", result: " + 
                                          (cacheIt->second ? "would leave in check" : "would not leave in check"));
            }
            
            // End timing for cache hit
            double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Cache hit execution time: " + 
                                        std::to_string(duration) + "ms");
            }
            
            decrementRecursionDepth();
            return cacheIt->second;
        }
        
        // Create a clone of the board
        auto tempBoard = clone();
        
        // Execute the move without validation
        const Position& from = move.getFrom();
        const Position& to = move.getTo();
        
        // Handle special moves
        if (isCastlingMove(move)) {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Processing castling move");
            }
            
            // For castling, we need to check if the king is in check at any point during the move
            int direction = (to.col > from.col) ? 1 : -1;
            
            // Check if the king is in check at the starting position
            if (tempBoard->isInCheck(color)) {
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - King already in check, castling not allowed");
                }
                
                // Cache the result
                checkResultCache[cacheKey] = true;
                
                // End timing
                double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Execution time (king already in check): " + 
                                            std::to_string(duration) + "ms");
                }
                
                decrementRecursionDepth();
                return true;
            }
            
            // Check if the king would be in check at any intermediate position
            Position midPos(from.row, from.col + direction);
            
            // Move the king to the intermediate position
            tempBoard->board[midPos.row][midPos.col] = std::move(tempBoard->board[from.row][from.col]);
            if (tempBoard->board[midPos.row][midPos.col]) {
                tempBoard->board[midPos.row][midPos.col]->setMoved(true);
            }
            
            // Check if the king would be in check at the intermediate position
            if (tempBoard->isInCheck(color)) {
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - King would pass through check during castling");
                }
                
                // Cache the result
                checkResultCache[cacheKey] = true;
                
                // End timing
                double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
                if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                    server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Execution time (king passes through check): " + 
                                            std::to_string(duration) + "ms");
                }
                
                decrementRecursionDepth();
                return true;
            }
            
            // Move the king to the final position
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[midPos.row][midPos.col]);
            if (tempBoard->board[to.row][to.col]) {
                tempBoard->board[to.row][to.col]->setMoved(true);
            }
            
            // Move the rook
            int rookFromCol = (direction > 0) ? 7 : 0;
            int rookToCol = (direction > 0) ? (to.col - 1) : (to.col + 1);
            
            Position rookFrom(from.row, rookFromCol);
            Position rookTo(from.row, rookToCol);
            tempBoard->board[rookTo.row][rookTo.col] = std::move(tempBoard->board[rookFrom.row][rookFrom.col]);
            if (tempBoard->board[rookTo.row][rookTo.col]) {
                tempBoard->board[rookTo.row][rookTo.col]->setMoved(true);
            }
            
        } else if (isEnPassantCapture(move)) {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Processing en passant capture");
            }
            
            // Move the pawn
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[from.row][from.col]);
            if (tempBoard->board[to.row][to.col]) {
                tempBoard->board[to.row][to.col]->setMoved(true);
            }
            
            // Remove the captured pawn
            int captureRow = (color == PieceColor::WHITE) ? to.row - 1 : to.row + 1;
            tempBoard->board[captureRow][to.col] = nullptr;
            
        } else {
            if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
                server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Processing regular move");
            }
            
            // Regular move
            tempBoard->board[to.row][to.col] = std::move(tempBoard->board[from.row][from.col]);
            if (tempBoard->board[to.row][to.col]) {
                tempBoard->board[to.row][to.col]->setMoved(true);
            }
        }
        
        // Check if the king is in check after the move
        bool result = tempBoard->isInCheck(color);
        
        // Cache the result
        checkResultCache[cacheKey] = result;
        
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Move " + 
                                      move.toAlgebraic() + " would " + 
                                      (result ? "leave king in check" : "not leave king in check"));
        }
        
        decrementRecursionDepth();
    
        // End timing and log
        double duration = PerformanceMonitor::endTimer("ChessBoard::wouldLeaveInCheck");
        if (server && server->getLogger() && server->getLogger()->getLogLevel() >= 3) {
            server->getLogger()->debug("ChessBoard::wouldLeaveInCheck() - Total execution time: " + 
                                    std::to_string(duration) + "ms");
        }
        
        return result;
    
    } catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::wouldLeaveInCheck() - Exception: " + std::string(e.what()));
        }
        
        decrementRecursionDepth();
        return true; // Conservatively assume it would leave in check
    } catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessBoard::wouldLeaveInCheck() - Unknown exception");
        }
        
        decrementRecursionDepth();        
        return true; // Conservatively assume it would leave in check
    }
}

std::string ChessBoard::generateCheckResultCacheKey(const ChessMove& move, PieceColor color) const
{
    // Generate a unique key for the current board state, move, and color
    std::string key = getBoardStateString() + "_move_" + 
                     move.toAlgebraic() + "_" +
                     (color == PieceColor::WHITE ? "white" : "black");
    return key;
}

std::string ChessBoard::getBoardStateString() const
{
    std::stringstream ss;
    
    // Add piece positions
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const ChessPiece* piece = getPiece(Position(r, c));
            if (piece) {
                ss << piece->getAsciiChar();
            } else {
                ss << '.';
            }
        }
    }
    
    // Add castling rights
    const ChessPiece* whiteKing = getPiece(Position(0, 4));
    const ChessPiece* blackKing = getPiece(Position(7, 4));
    const ChessPiece* whiteKingsideRook = getPiece(Position(0, 7));
    const ChessPiece* whiteQueensideRook = getPiece(Position(0, 0));
    const ChessPiece* blackKingsideRook = getPiece(Position(7, 7));
    const ChessPiece* blackQueensideRook = getPiece(Position(7, 0));
    
    ss << (whiteKing && !whiteKing->hasMoved() && whiteKingsideRook && !whiteKingsideRook->hasMoved() ? 'K' : '-');
    ss << (whiteKing && !whiteKing->hasMoved() && whiteQueensideRook && !whiteQueensideRook->hasMoved() ? 'Q' : '-');
    ss << (blackKing && !blackKing->hasMoved() && blackKingsideRook && !blackKingsideRook->hasMoved() ? 'k' : '-');
    ss << (blackKing && !blackKing->hasMoved() && blackQueensideRook && !blackQueensideRook->hasMoved() ? 'q' : '-');
    
    // Add en passant target
    ss << (enPassantTarget.isValid() ? enPassantTarget.toAlgebraic() : "-");
    
    // Add current turn
    ss << (currentTurn == PieceColor::WHITE ? 'w' : 'b');
    
    return ss.str();
}

// Implementation of ChessPlayer class
ChessPlayer::ChessPlayer(const std::string& username, QTcpSocket* socket)
    : username(username), rating(1200), color(PieceColor::NONE), socket(socket),
      gamesPlayed(0), wins(0), losses(0), draws(0), remainingTime(0), bot(false) {
}

ChessPlayer::~ChessPlayer() {
    // Don't delete the socket here, it's managed by Qt
}

std::string ChessPlayer::getUsername() const {
    return username;
}

int ChessPlayer::getRating() const {
    return rating;
}

void ChessPlayer::setRating(int rating) {
    this->rating = rating;
}

PieceColor ChessPlayer::getColor() const {
    return color;
}

void ChessPlayer::setColor(PieceColor color) {
    this->color = color;
}

QTcpSocket* ChessPlayer::getSocket() const {
    return socket;
}

void ChessPlayer::setSocket(QTcpSocket* socket) {
    this->socket = socket;
}

int ChessPlayer::getGamesPlayed() const {
    return gamesPlayed;
}

int ChessPlayer::getWins() const {
    return wins;
}

int ChessPlayer::getLosses() const {
    return losses;
}

int ChessPlayer::getDraws() const {
    return draws;
}

void ChessPlayer::updateStats(GameResult result) {
    gamesPlayed++;
    
    if (result == GameResult::WHITE_WIN && color == PieceColor::WHITE) {
        wins++;
    } else if (result == GameResult::BLACK_WIN && color == PieceColor::BLACK) {
        wins++;
    } else if (result == GameResult::WHITE_WIN && color == PieceColor::BLACK) {
        losses++;
    } else if (result == GameResult::BLACK_WIN && color == PieceColor::WHITE) {
        losses++;
    } else if (result == GameResult::DRAW) {
        draws++;
    }
}

qint64 ChessPlayer::getRemainingTime() const {
    return remainingTime;
}

void ChessPlayer::setRemainingTime(qint64 time) {
    remainingTime = time;
}

void ChessPlayer::decrementTime(qint64 milliseconds) {
    remainingTime -= milliseconds;
    if (remainingTime < 0) {
        remainingTime = 0;
    }
}

bool ChessPlayer::isBot() const {
    return bot;
}

void ChessPlayer::setBot(bool isBot) {
    bot = isBot;
}

const std::vector<std::string>& ChessPlayer::getGameHistory() const {
    return gameHistory;
}

void ChessPlayer::addGameToHistory(const std::string& gameId) {
    gameHistory.push_back(gameId);
}

QJsonObject ChessPlayer::toJson() const {
    QJsonObject json;
    json["username"] = QString::fromStdString(username);
    json["rating"] = rating;
    json["gamesPlayed"] = gamesPlayed;
    json["wins"] = wins;
    json["losses"] = losses;
    json["draws"] = draws;
    json["bot"] = bot;
    
    QJsonArray historyArray;
    for (const std::string& gameId : gameHistory) {
        historyArray.append(QString::fromStdString(gameId));
    }
    json["gameHistory"] = historyArray;
    
    return json;
}

ChessPlayer ChessPlayer::fromJson(const QJsonObject& json) {
    std::string username = json["username"].toString().toStdString();
    ChessPlayer player(username);
    
    player.setRating(json["rating"].toInt());
    player.gamesPlayed = json["gamesPlayed"].toInt();
    player.wins = json["wins"].toInt();
    player.losses = json["losses"].toInt();
    player.draws = json["draws"].toInt();
    player.bot = json["bot"].toBool();
    
    QJsonArray historyArray = json["gameHistory"].toArray();
    for (const QJsonValue& value : historyArray) {
        player.gameHistory.push_back(value.toString().toStdString());
    }
    
    return player;
}

// Implementation of ChessGame class
ChessGame::ChessGame(ChessPlayer* whitePlayer, ChessPlayer* blackPlayer, 
                     const std::string& gameId, TimeControlType timeControl)
    : gameId(gameId), whitePlayer(whitePlayer), blackPlayer(blackPlayer),
      result(GameResult::IN_PROGRESS), timeControl(timeControl),
      drawOffered(false), drawOfferingPlayer(nullptr)
{
    MPChessServer* server = MPChessServer::getInstance();
    
    if (server && server->getLogger()) {
        server->getLogger()->debug("ChessGame constructor - Creating game " + gameId);
    }
    
    if (!whitePlayer || !blackPlayer) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessGame constructor - Null player provided to ChessGame constructor for game " + gameId);
        }
        throw std::invalid_argument("Null player provided to ChessGame constructor");
    }
    
    try {
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessGame constructor - Creating ChessBoard for game " + gameId);
        }
        
        board = std::make_unique<ChessBoard>();
        
        if (!board) {
            if (server && server->getLogger()) {
                server->getLogger()->error("ChessGame constructor - Failed to create ChessBoard for game " + gameId);
            }
            throw std::runtime_error("Failed to create ChessBoard");
        }

        board->initialize();

        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessGame constructor - ChessBoard created successfully for game " + gameId);
        }
    } catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessGame constructor - Error creating ChessBoard for game " + gameId + ": " + std::string(e.what()));
        }
        throw std::runtime_error("Error creating ChessBoard: " + std::string(e.what()));
    }
    
    startTime = QDateTime::currentDateTime();
    lastMoveTime = startTime;
    
    if (server && server->getLogger()) {
        server->getLogger()->debug("ChessGame constructor - Game " + gameId + " created successfully with players: " + 
                                  whitePlayer->getUsername() + " (White) and " + blackPlayer->getUsername() + " (Black)");
    }
}

ChessGame::~ChessGame() {
    // Players are managed externally
}

std::string ChessGame::getGameId() const {
    return gameId;
}

ChessPlayer* ChessGame::getWhitePlayer() const {
    return whitePlayer;
}

ChessPlayer* ChessGame::getBlackPlayer() const {
    return blackPlayer;
}

ChessPlayer* ChessGame::getCurrentPlayer() const {
    PieceColor currentTurn = board->getCurrentTurn();
    return (currentTurn == PieceColor::WHITE) ? whitePlayer : blackPlayer;
}

ChessPlayer* ChessGame::getOpponentPlayer(const ChessPlayer* player) const {
    return (player == whitePlayer) ? blackPlayer : whitePlayer;
}

ChessBoard* ChessGame::getBoard() const {
    return board.get();
}

GameResult ChessGame::getResult() const {
    return result;
}

void ChessGame::setResult(GameResult result) {
    this->result = result;
}

TimeControlType ChessGame::getTimeControl() const {
    return timeControl;
}

MoveValidationStatus ChessGame::processMove(ChessPlayer* player, const ChessMove& move) {
    // Check if the game is over
    if (isOver()) {
        return MoveValidationStatus::GAME_OVER;
    }
    
    // Check if it's the player's turn
    PieceColor currentTurn = board->getCurrentTurn();
    if ((currentTurn == PieceColor::WHITE && player != whitePlayer) ||
        (currentTurn == PieceColor::BLACK && player != blackPlayer)) {
        return MoveValidationStatus::WRONG_TURN;
    }
    
    // Validate and execute the move
    MoveValidationStatus status = board->movePiece(move);
    if (status == MoveValidationStatus::VALID) {
        // Record the time taken for this move
        QDateTime now = QDateTime::currentDateTime();
        qint64 timeTaken = lastMoveTime.msecsTo(now);
        moveTimings.emplace_back(move, timeTaken);
        
        // Update the player's remaining time
        updatePlayerTime(player);
        
        // Update the last move time
        lastMoveTime = now;
        
        // Check if the game is over
        if (board->isInCheckmate(PieceColor::WHITE)) {
            end(GameResult::BLACK_WIN);
        } else if (board->isInCheckmate(PieceColor::BLACK)) {
            end(GameResult::WHITE_WIN);
        } else if (board->isInStalemate(PieceColor::WHITE) || board->isInStalemate(PieceColor::BLACK) ||
                   board->canClaimThreefoldRepetition() || board->canClaimFiftyMoveRule() || 
                   board->hasInsufficientMaterial()) {
            end(GameResult::DRAW);
        }
        
        // Reset draw offer
        drawOffered = false;
        drawOfferingPlayer = nullptr;
    }
    
    return status;
}

void ChessGame::start()
{
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->getLogger()) {
        server->getLogger()->debug("ChessGame::start() - Starting game " + gameId);
    }
    
    try {
        startTime = QDateTime::currentDateTime();
        lastMoveTime = startTime;
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessGame::start() - Initializing board for game " + gameId);
        }
        
        // Initialize the board
        try {
            if (!board) {
                if (server && server->getLogger()) {
                    server->getLogger()->error("ChessGame::start() - Board is null for game " + gameId);
                }
                throw std::runtime_error("Board is null");
            }
            board->initialize();
        } catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("ChessGame::start() - Board initialization failed for game " + gameId + ": " + std::string(e.what()));
            }
            throw std::runtime_error("Board initialization failed: " + std::string(e.what()));
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessGame::start() - Setting player colors for game " + gameId);
        }
        
        // Set the player colors
        if (whitePlayer) {
            whitePlayer->setColor(PieceColor::WHITE);
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessGame::start() - Set " + whitePlayer->getUsername() + " as WHITE for game " + gameId);
            }
        } else {
            if (server && server->getLogger()) {
                server->getLogger()->warning("ChessGame::start() - White player is null for game " + gameId);
            }
        }
        
        if (blackPlayer) {
            blackPlayer->setColor(PieceColor::BLACK);
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessGame::start() - Set " + blackPlayer->getUsername() + " as BLACK for game " + gameId);
            }
        } else {
            if (server && server->getLogger()) {
                server->getLogger()->warning("ChessGame::start() - Black player is null for game " + gameId);
            }
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessGame::start() - Initializing time control for game " + gameId);
        }
        
        // Initialize the time control
        initializeTimeControl();
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessGame::start() - Game " + gameId + " started successfully");
        }
    } catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessGame::start() - Fatal error starting game " + gameId + ": " + std::string(e.what()));
        }
        throw std::runtime_error("Error starting game: " + std::string(e.what()));
    } catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessGame::start() - Unknown fatal error starting game " + gameId);
        }
        throw std::runtime_error("Unknown error starting game");
    }
}

void ChessGame::end(GameResult result)
{
    this->result = result;
    endTime = QDateTime::currentDateTime();
    
    // Update player statistics
    whitePlayer->updateStats(result);
    blackPlayer->updateStats(result);
}

bool ChessGame::isOver() const {
    return result != GameResult::IN_PROGRESS;
}

QJsonObject ChessGame::getGameStateJson() const
{
    QJsonObject json;
    MPChessServer* server = MPChessServer::getInstance();
    
    try {
        if (server && server->getLogger()) {
            server->getLogger()->debug("getGameStateJson() - Generating game state for game " + gameId);
        }
        
        json["gameId"] = QString::fromStdString(gameId);
        
        // Player information with null checks and exception handling
        try {
            if (whitePlayer) {
                json["whitePlayer"] = QString::fromStdString(whitePlayer->getUsername());
                json["whiteRemainingTime"] = static_cast<qint64>(whitePlayer->getRemainingTime());
                if (server && server->getLogger()) {
                    server->getLogger()->debug("getGameStateJson() - White player: " + whitePlayer->getUsername() + 
                                              ", time: " + std::to_string(whitePlayer->getRemainingTime()));
                }
            } else {
                json["whitePlayer"] = "Unknown";
                json["whiteRemainingTime"] = 0;
                if (server && server->getLogger()) {
                    server->getLogger()->warning("getGameStateJson() - White player is null for game " + gameId);
                }
            }
        } catch (const std::exception& e) {
            json["whitePlayer"] = "Error";
            json["whiteRemainingTime"] = 0;
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception processing white player: " + std::string(e.what()));
            }
        }
        
        try {
            if (blackPlayer) {
                json["blackPlayer"] = QString::fromStdString(blackPlayer->getUsername());
                json["blackRemainingTime"] = static_cast<qint64>(blackPlayer->getRemainingTime());
                if (server && server->getLogger()) {
                    server->getLogger()->debug("getGameStateJson() - Black player: " + blackPlayer->getUsername() + 
                                              ", time: " + std::to_string(blackPlayer->getRemainingTime()));
                }
            } else {
                json["blackPlayer"] = "Unknown";
                json["blackRemainingTime"] = 0;
                if (server && server->getLogger()) {
                    server->getLogger()->warning("getGameStateJson() - Black player is null for game " + gameId);
                }
            }
        } catch (const std::exception& e) {
            json["blackPlayer"] = "Error";
            json["blackRemainingTime"] = 0;
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception processing black player: " + std::string(e.what()));
            }
        }
        
        // Board state with null check
        try {
            if (board) {
                json["currentTurn"] = (board->getCurrentTurn() == PieceColor::WHITE) ? "white" : "black";
                
                // Add check status with proper error handling
                try {
                    json["isCheck"] = board->isInCheck(board->getCurrentTurn());
                    json["isCheckmate"] = board->isInCheckmate(board->getCurrentTurn());
                    json["isStalemate"] = board->isInStalemate(board->getCurrentTurn());
                } catch (const std::exception& e) {
                    json["isCheck"] = false;
                    json["isCheckmate"] = false;
                    json["isStalemate"] = false;
                    if (server && server->getLogger()) {
                        server->getLogger()->error("getGameStateJson() - Exception checking game status: " + std::string(e.what()));
                    }
                }
                
                if (server && server->getLogger()) {
                    server->getLogger()->debug("getGameStateJson() - Game " + gameId + " turn: " + 
                                              (board->getCurrentTurn() == PieceColor::WHITE ? "white" : "black"));
                }
            } else {
                // Set default values if board is null
                json["currentTurn"] = "white";
                json["isCheck"] = false;
                json["isCheckmate"] = false;
                json["isStalemate"] = false;
                if (server && server->getLogger()) {
                    server->getLogger()->warning("getGameStateJson() - Board is null, so added default values, for game " + gameId);
                }
            }
        } catch (const std::exception& e) {
            json["currentTurn"] = "white";
            json["isCheck"] = false;
            json["isCheckmate"] = false;
            json["isStalemate"] = false;
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception processing board state: " + std::string(e.what()));
            }
        }
        
        // Game result
        try {
            json["result"] = [this]() -> QString {
                switch (result) {
                    case GameResult::WHITE_WIN: return "white_win";
                    case GameResult::BLACK_WIN: return "black_win";
                    case GameResult::DRAW: return "draw";
                    default: return "in_progress";
                }
            }();
            
            if (server && server->getLogger()) {
                server->getLogger()->debug("getGameStateJson() - Game " + gameId + " result: " + json["result"].toString().toStdString());
            }
        } catch (const std::exception& e) {
            json["result"] = "in_progress";
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception processing game result: " + std::string(e.what()));
            }
        }
        
        // Draw offer status
        try {
            json["drawOffered"] = drawOffered;
            if (drawOffered && drawOfferingPlayer) {
                json["drawOfferingPlayer"] = QString::fromStdString(drawOfferingPlayer->getUsername());
                if (server && server->getLogger()) {
                    server->getLogger()->debug("getGameStateJson() - Draw offered by " + drawOfferingPlayer->getUsername() + " in game " + gameId);
                }
            }
        } catch (const std::exception& e) {
            json["drawOffered"] = false;
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception processing draw offer: " + std::string(e.what()));
            }
        }
        
        // Board state - now with piece type information and proper orientation
        if (server && server->getLogger()) {
            server->getLogger()->debug("getGameStateJson() - Building board array for game " + gameId);
        }
        
        QJsonArray boardArray;
        try {
            if (board) {
                for (int r = 0; r < 8; ++r) {
                    QJsonArray rowArray;
                    for (int c = 0; c < 8; ++c) {
                        QJsonObject pieceObj;
                        Position pos(r, c);
                        const ChessPiece* piece = nullptr;
                        
                        try {
                            piece = board->getPiece(pos);
                        } catch (const std::exception& e) {
                            if (server && server->getLogger()) {
                                server->getLogger()->error("getGameStateJson() - Exception getting piece at " + 
                                                         std::to_string(r) + "," + std::to_string(c) + ": " + 
                                                         std::string(e.what()));
                            }
                            piece = nullptr;
                        }
                        
                        if (piece) {
                            // try-catch block around piece property access
                            try {
                                PieceType type = piece->getType();
                                switch (type) {
                                    case PieceType::PAWN:   pieceObj["type"] = "pawn"; break;
                                    case PieceType::KNIGHT: pieceObj["type"] = "knight"; break;
                                    case PieceType::BISHOP: pieceObj["type"] = "bishop"; break;
                                    case PieceType::ROOK:   pieceObj["type"] = "rook"; break;
                                    case PieceType::QUEEN:  pieceObj["type"] = "queen"; break;
                                    case PieceType::KING:   pieceObj["type"] = "king"; break;
                                    default:                pieceObj["type"] = "unknown"; break;
                                }
                                
                                // Get color directly from the base class
                                pieceObj["color"] = (piece->getColor() == PieceColor::WHITE) ? "white" : "black";
                            } catch (const std::exception& e) {
                                if (server && server->getLogger()) {
                                    server->getLogger()->error("getGameStateJson() - Exception processing piece at " + 
                                                             std::to_string(r) + "," + std::to_string(c) + ": " + std::string(e.what()));
                                }
                                pieceObj["type"] = "error";
                                pieceObj["color"] = "none";
                            }
                        } else {
                            pieceObj["type"] = "empty";
                            pieceObj["color"] = "none";
                        }
                        
                        rowArray.append(pieceObj);
                    }
                    boardArray.append(rowArray);
                }
            }
        } catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception building board array: " + std::string(e.what()));
            }
            
            // Create an empty board on error
            for (int r = 0; r < 8; ++r) {
                QJsonArray rowArray;
                for (int c = 0; c < 8; ++c) {
                    QJsonObject pieceObj;
                    pieceObj["type"] = "empty";
                    pieceObj["color"] = "none";
                    rowArray.append(pieceObj);
                }
                boardArray.append(rowArray);
            }
        }
        json["board"] = boardArray;
        
        // Add board orientation flag to help clients render correctly
        json["boardOrientation"] = "standard"; // Standard orientation (white at bottom)
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("getGameStateJson() - Board array built successfully for game " + gameId);
        }
        
        // Add move history
        if (server && server->getLogger()) {
            server->getLogger()->debug("getGameStateJson() - Building move history for game " + gameId);
        }
        
        QJsonArray moveHistoryArray;
        try {
            if (board) {
                const std::vector<ChessMove>& moveHistory = board->getMoveHistory();
                for (const ChessMove& move : moveHistory) {
                    QJsonObject moveObj;
                    moveObj["from"] = QString::fromStdString(move.getFrom().toAlgebraic());
                    moveObj["to"] = QString::fromStdString(move.getTo().toAlgebraic());
                    moveObj["algebraic"] = QString::fromStdString(move.toStandardNotation(*board));
                    if (move.getPromotionType() != PieceType::EMPTY) {
                        moveObj["promotion"] = [move]() -> QString {
                            switch (move.getPromotionType()) {
                                case PieceType::QUEEN: return "queen";
                                case PieceType::ROOK: return "rook";
                                case PieceType::BISHOP: return "bishop";
                                case PieceType::KNIGHT: return "knight";
                                default: return "";
                            }
                        }();
                    }
                    moveHistoryArray.append(moveObj);
                }
            }
        } catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception processing move history: " + std::string(e.what()));
            }
        }
        json["moveHistory"] = moveHistoryArray;
        
        // Add captured pieces
        if (server && server->getLogger()) {
            server->getLogger()->debug("getGameStateJson() - Building captured pieces arrays for game " + gameId);
        }
        
        QJsonArray whiteCapturedArray;
        QJsonArray blackCapturedArray;
        
        try {
            if (board) {
                for (PieceType type : board->getCapturedPieces(PieceColor::WHITE)) {
                    whiteCapturedArray.append([type]() -> QString {
                        switch (type) {
                            case PieceType::PAWN: return "pawn";
                            case PieceType::KNIGHT: return "knight";
                            case PieceType::BISHOP: return "bishop";
                            case PieceType::ROOK: return "rook";
                            case PieceType::QUEEN: return "queen";
                            default: return "";
                        }
                    }());
                }
            }
        } catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception processing white captured pieces: " + std::string(e.what()));
            }
        }
        
        try {
            if (board) {
                for (PieceType type : board->getCapturedPieces(PieceColor::BLACK)) {
                    blackCapturedArray.append([type]() -> QString {
                        switch (type) {
                            case PieceType::PAWN: return "pawn";
                            case PieceType::KNIGHT: return "knight";
                            case PieceType::BISHOP: return "bishop";
                            case PieceType::ROOK: return "rook";
                            case PieceType::QUEEN: return "queen";
                            default: return "";
                        }
                    }());
                }
            }
        } catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("getGameStateJson() - Exception processing black captured pieces: " + std::string(e.what()));
            }
        }
        
        json["whiteCaptured"] = whiteCapturedArray;
        json["blackCaptured"] = blackCapturedArray;
        
        // ASCII board representation
        if (board) {
            if (server && server->getLogger()) {
                server->getLogger()->debug("getGameStateJson() - Getting ASCII board representation for game " + gameId);
            }
            try {
                json["asciiBoard"] = QString::fromStdString(board->getAsciiBoard());
            } catch (const std::exception& e) {
                if (server && server->getLogger()) {
                    server->getLogger()->error("getGameStateJson() - Exception generating ASCII board: " + std::string(e.what()));
                }
                json["asciiBoard"] = "";
            }
        } else {
            json["asciiBoard"] = "";
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("getGameStateJson() - Successfully generated game state for game " + gameId);
        }
    } catch (const std::exception& e) {
        // Comprehensive error handling for the entire function
        if (server && server->getLogger()) {
            server->getLogger()->error("getGameStateJson() - Exception generating game state for game " + gameId + ": " + std::string(e.what()));
        }
        
        json = QJsonObject();
        json["error"] = "Error generating game state: " + QString::fromStdString(std::string(e.what()));
        json["gameId"] = QString::fromStdString(gameId);
        json["result"] = "in_progress";
    } catch (...) {
        // Handling for unknown exceptions
        if (server && server->getLogger()) {
            server->getLogger()->error("getGameStateJson() - Unknown exception generating game state for game " + gameId);
        }
        
        json = QJsonObject();
        json["error"] = "Unknown error generating game state";
        json["gameId"] = QString::fromStdString(gameId);
        json["result"] = "in_progress";
    }
    
    return json;
}

QJsonObject ChessGame::getGameHistoryJson() const
{
    QJsonObject json = getGameStateJson();
    
    // Add additional history information
    json["startTime"] = startTime.toString(Qt::ISODate);
    if (isOver()) {
        json["endTime"] = endTime.toString(Qt::ISODate);
        json["duration"] = startTime.secsTo(endTime);
    }
    
    // Add move timings
    QJsonArray moveTimingsArray;
    for (const auto& [move, time] : moveTimings) {
        QJsonObject timingObj;
        timingObj["move"] = QString::fromStdString(move.toAlgebraic());
        timingObj["timeMs"] = static_cast<qint64>(time);
        moveTimingsArray.append(timingObj);
    }
    json["moveTimings"] = moveTimingsArray;
    
    return json;
}

const std::vector<std::pair<ChessMove, qint64>>& ChessGame::getMoveTimings() const {
    return moveTimings;
}

std::string ChessGame::getBoardAscii() const {
    return board->getAsciiBoard();
}

std::vector<std::pair<ChessMove, double>> ChessGame::getMoveRecommendations(ChessPlayer* player) const
{
    MPChessServer* server = MPChessServer::getInstance();
    
    if (!player || !board) {
        if (server && server->getLogger()) {
            server->getLogger()->warning("ChessGame::getMoveRecommendations() - Null player or board");
        }
        return {};
    }
    
    try {
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessGame::getMoveRecommendations() - Generating recommendations for player " + 
                                      player->getUsername());
        }
        
        // Check if it's the player's turn
        PieceColor playerColor = player->getColor();
        PieceColor currentTurn = board->getCurrentTurn();
        
        if (playerColor != currentTurn) {
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessGame::getMoveRecommendations() - Not player's turn, returning empty list");
            }
            return {};
        }
        
        // Use a simplified approach for recommendations to avoid deep recursion
        std::vector<ChessMove> validMoves;
        try {
            validMoves = board->getAllValidMoves(playerColor);
            
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessGame::getMoveRecommendations() - Found " + 
                                          std::to_string(validMoves.size()) + " valid moves");
            }
        }
        catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("ChessGame::getMoveRecommendations() - Exception getting valid moves: " + 
                                         std::string(e.what()));
            }
            return {};
        }
        
        // Simple scoring for moves
        std::vector<std::pair<ChessMove, double>> recommendations;
        for (const ChessMove& move : validMoves) {
            double score = 0.0;
            
            // Check if the move captures a piece
            const Position& to = move.getTo();
            const ChessPiece* capturedPiece = board->getPiece(to);
            if (capturedPiece) {
                // Add value for capturing pieces
                switch (capturedPiece->getType()) {
                    case PieceType::PAWN: score += 1.0; break;
                    case PieceType::KNIGHT: score += 3.0; break;
                    case PieceType::BISHOP: score += 3.25; break;
                    case PieceType::ROOK: score += 5.0; break;
                    case PieceType::QUEEN: score += 9.0; break;
                    default: break;
                }
            }
            
            // Check if the move is a pawn promotion
            if (move.getPromotionType() != PieceType::EMPTY) {
                score += 8.0; // Heavily favor promotions
            }
            
            recommendations.emplace_back(move, score);
        }
        
        // Sort by score (best moves first)
        std::sort(recommendations.begin(), recommendations.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Limit to top 5 recommendations
        if (recommendations.size() > 5) {
            recommendations.resize(5);
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessGame::getMoveRecommendations() - Returning " + 
                                      std::to_string(recommendations.size()) + " recommendations");
        }
        
        return recommendations;
    } catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessGame::getMoveRecommendations() - Exception: " + std::string(e.what()));
        }
        return {};
    } catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessGame::getMoveRecommendations() - Unknown exception");
        }
        return {};
    }
}

bool ChessGame::handleDrawOffer(ChessPlayer* player) {
    if (isOver()) return false;
    
    drawOffered = true;
    drawOfferingPlayer = player;
    return true;
}

void ChessGame::handleDrawResponse(ChessPlayer* player, bool accepted) {
    if (!drawOffered || player == drawOfferingPlayer) return;
    
    if (accepted) {
        end(GameResult::DRAW);
    } else {
        drawOffered = false;
        drawOfferingPlayer = nullptr;
    }
}

void ChessGame::handleResignation(ChessPlayer* player) {
    if (isOver()) return;
    
    if (player == whitePlayer) {
        end(GameResult::BLACK_WIN);
    } else {
        end(GameResult::WHITE_WIN);
    }
}

void ChessGame::updateTimers() {
    if (isOver()) return;
    
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = lastMoveTime.msecsTo(now);
    
    // Only decrement time for the current player
    ChessPlayer* currentPlayer = getCurrentPlayer();
    if (currentPlayer) {
        currentPlayer->decrementTime(elapsed);
    }
    
    lastMoveTime = now;
}

bool ChessGame::hasPlayerTimedOut(ChessPlayer* player) const {
    return player->getRemainingTime() <= 0;
}

QJsonObject ChessGame::serialize() const {
    QJsonObject json = getGameHistoryJson();
    
    // Add serialized board state
    ChessSerializer serializer;
    json["boardState"] = serializer.serializeBoard(*board);
    
    return json;
}

std::unique_ptr<ChessGame> ChessGame::deserialize(const QJsonObject& json, 
                                                ChessPlayer* whitePlayer, 
                                                ChessPlayer* blackPlayer) {
    std::string gameId = json["gameId"].toString().toStdString();
    TimeControlType timeControl = [&json]() -> TimeControlType {
        QString tcStr = json["timeControl"].toString();
        if (tcStr == "rapid") return TimeControlType::RAPID;
        if (tcStr == "blitz") return TimeControlType::BLITZ;
        if (tcStr == "bullet") return TimeControlType::BULLET;
        if (tcStr == "classical") return TimeControlType::CLASSICAL;
        if (tcStr == "casual") return TimeControlType::CASUAL;
        return TimeControlType::RAPID;  // Default
    }();
    
    auto game = std::make_unique<ChessGame>(whitePlayer, blackPlayer, gameId, timeControl);
    
    // Deserialize board state
    ChessSerializer serializer;
    game->board = serializer.deserializeBoard(json["boardState"].toObject());
    
    // Set player times
    whitePlayer->setRemainingTime(json["whiteRemainingTime"].toInt());
    blackPlayer->setRemainingTime(json["blackRemainingTime"].toInt());
    
    // Set game result
    QString resultStr = json["result"].toString();
    if (resultStr == "white_win") {
        game->result = GameResult::WHITE_WIN;
    } else if (resultStr == "black_win") {
        game->result = GameResult::BLACK_WIN;
    } else if (resultStr == "draw") {
        game->result = GameResult::DRAW;
    } else {
        game->result = GameResult::IN_PROGRESS;
    }
    
    // Set times
    game->startTime = QDateTime::fromString(json["startTime"].toString(), Qt::ISODate);
    if (json.contains("endTime")) {
        game->endTime = QDateTime::fromString(json["endTime"].toString(), Qt::ISODate);
    }
    game->lastMoveTime = QDateTime::currentDateTime();
    
    // Deserialize move timings
    QJsonArray moveTimingsArray = json["moveTimings"].toArray();
    for (const QJsonValue& value : moveTimingsArray) {
        QJsonObject timingObj = value.toObject();
        ChessMove move = ChessMove::fromAlgebraic(timingObj["move"].toString().toStdString());
        qint64 time = timingObj["timeMs"].toInt();
        game->moveTimings.emplace_back(move, time);
    }
    
    return game;
}

void ChessGame::initializeTimeControl() {
    qint64 timeMs = 0;
    
    switch (timeControl) {
        case TimeControlType::RAPID:
            timeMs = 10 * 60 * 1000;  // 10 minutes
            break;
        case TimeControlType::BLITZ:
            timeMs = 5 * 60 * 1000;   // 5 minutes
            break;
        case TimeControlType::BULLET:
            timeMs = 1 * 60 * 1000;   // 1 minute
            break;
        case TimeControlType::CLASSICAL:
            timeMs = 90 * 60 * 1000;  // 90 minutes
            break;
        case TimeControlType::CASUAL:
            timeMs = 7 * 24 * 60 * 60 * 1000;  // 7 days
            break;
    }
    
    whitePlayer->setRemainingTime(timeMs);
    blackPlayer->setRemainingTime(timeMs);
}

void ChessGame::updatePlayerTime(ChessPlayer* player) {
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = lastMoveTime.msecsTo(now);
    
    player->decrementTime(elapsed);
}

// Implementation of ChessAI class
// Define the piece-square tables
const std::array<std::array<double, 8>, 8> ChessAI::pawnTable = {{
    {{ 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 }},
    {{ 5.0,  5.0,  5.0,  5.0,  5.0,  5.0,  5.0,  5.0 }},
    {{ 1.0,  1.0,  2.0,  3.0,  3.0,  2.0,  1.0,  1.0 }},
    {{ 0.5,  0.5,  1.0,  2.5,  2.5,  1.0,  0.5,  0.5 }},
    {{ 0.0,  0.0,  0.0,  2.0,  2.0,  0.0,  0.0,  0.0 }},
    {{ 0.5, -0.5, -1.0,  0.0,  0.0, -1.0, -0.5,  0.5 }},
    {{ 0.5,  1.0,  1.0, -2.0, -2.0,  1.0,  1.0,  0.5 }},
    {{ 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 }}
}};

const std::array<std::array<double, 8>, 8> ChessAI::knightTable = {{
    {{-5.0, -4.0, -3.0, -3.0, -3.0, -3.0, -4.0, -5.0 }},
    {{-4.0, -2.0,  0.0,  0.0,  0.0,  0.0, -2.0, -4.0 }},
    {{-3.0,  0.0,  1.0,  1.5,  1.5,  1.0,  0.0, -3.0 }},
    {{-3.0,  0.5,  1.5,  2.0,  2.0,  1.5,  0.5, -3.0 }},
    {{-3.0,  0.0,  1.5,  2.0,  2.0,  1.5,  0.0, -3.0 }},
    {{-3.0,  0.5,  1.0,  1.5,  1.5,  1.0,  0.5, -3.0 }},
    {{-4.0, -2.0,  0.0,  0.5,  0.5,  0.0, -2.0, -4.0 }},
    {{-5.0, -4.0, -3.0, -3.0, -3.0, -3.0, -4.0, -5.0 }}
}};

const std::array<std::array<double, 8>, 8> ChessAI::bishopTable = {{
    {{-2.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -2.0 }},
    {{-1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -1.0 }},
    {{-1.0,  0.0,  0.5,  1.0,  1.0,  0.5,  0.0, -1.0 }},
    {{-1.0,  0.5,  0.5,  1.0,  1.0,  0.5,  0.5, -1.0 }},
    {{-1.0,  0.0,  1.0,  1.0,  1.0,  1.0,  0.0, -1.0 }},
    {{-1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0, -1.0 }},
    {{-1.0,  0.5,  0.0,  0.0,  0.0,  0.0,  0.5, -1.0 }},
    {{-2.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -2.0 }}
}};

const std::array<std::array<double, 8>, 8> ChessAI::rookTable = {{
    {{ 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 }},
    {{ 0.5,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  0.5 }},
    {{-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 }},
    {{-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 }},
    {{-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 }},
    {{-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 }},
    {{-0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5 }},
    {{ 0.0,  0.0,  0.0,  0.5,  0.5,  0.0,  0.0,  0.0 }}
}};

const std::array<std::array<double, 8>, 8> ChessAI::queenTable = {{
    {{-2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0 }},
    {{-1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -1.0 }},
    {{-1.0,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -1.0 }},
    {{-0.5,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -0.5 }},
    {{ 0.0,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -0.5 }},
    {{-1.0,  0.5,  0.5,  0.5,  0.5,  0.5,  0.0, -1.0 }},
    {{-1.0,  0.0,  0.5,  0.0,  0.0,  0.0,  0.0, -1.0 }},
    {{-2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0 }}
}};

const std::array<std::array<double, 8>, 8> ChessAI::kingMiddleGameTable = {{
    {{-3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0 }},
    {{-3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0 }},
    {{-3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0 }},
    {{-3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0 }},
    {{-2.0, -3.0, -3.0, -4.0, -4.0, -3.0, -3.0, -2.0 }},
    {{-1.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -1.0 }},
    {{ 2.0,  2.0,  0.0,  0.0,  0.0,  0.0,  2.0,  2.0 }},
    {{ 2.0,  3.0,  1.0,  0.0,  0.0,  1.0,  3.0,  2.0 }}
}};

const std::array<std::array<double, 8>, 8> ChessAI::kingEndGameTable = {{
    {{-5.0, -4.0, -3.0, -2.0, -2.0, -3.0, -4.0, -5.0 }},
    {{-3.0, -2.0, -1.0,  0.0,  0.0, -1.0, -2.0, -3.0 }},
    {{-3.0, -1.0,  2.0,  3.0,  3.0,  2.0, -1.0, -3.0 }},
    {{-3.0, -1.0,  3.0,  4.0,  4.0,  3.0, -1.0, -3.0 }},
    {{-3.0, -1.0,  3.0,  4.0,  4.0,  3.0, -1.0, -3.0 }},
    {{-3.0, -1.0,  2.0,  3.0,  3.0,  2.0, -1.0, -3.0 }},
    {{-3.0, -3.0,  0.0,  0.0,  0.0,  0.0, -3.0, -3.0 }},
    {{-5.0, -3.0, -3.0, -3.0, -3.0, -3.0, -3.0, -5.0 }}
}};

ChessAI::ChessAI(int skillLevel) : skillLevel(std::min(std::max(skillLevel, 1), 10)) {
}

ChessMove ChessAI::getBestMove(const ChessBoard& board, PieceColor color)
{
    // If Stockfish is available and skill level is high enough, use it
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->stockfishConnector && server->stockfishConnector->isInitialized() && skillLevel >= 8) {
        server->stockfishConnector->setPosition(board);
        server->stockfishConnector->setSkillLevel(skillLevel * 2);  // Convert our 1-10 scale to Stockfish's 0-20
        return server->stockfishConnector->getBestMove();
    }
    
    // Otherwise, use our built-in AI
    std::vector<ChessMove> validMoves = board.getAllValidMoves(color);
    if (validMoves.empty()) {
        return ChessMove();  // No valid moves
    }
    
    // Introduce randomness based on skill level
    if (skillLevel < 10 && !validMoves.empty()) {
        // Chance to make a random move decreases with skill level
        double randomChance = 0.5 * (10 - skillLevel) / 10.0;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        
        if (dis(gen) < randomChance) {
            std::uniform_int_distribution<> moveIdx(0, validMoves.size() - 1);
            return validMoves[moveIdx(gen)];
        }
    }
    
    ChessMove bestMove;
    double bestValue = color == PieceColor::WHITE ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    int depth = getSearchDepth();
    
    for (const ChessMove& move : validMoves) {
        // Create a copy of the board and make the move
        auto tempBoard = board.clone();
        tempBoard->movePiece(move);
        
        // Evaluate the position after the move
        double value = minimax(*tempBoard, depth - 1, 
                              -std::numeric_limits<double>::infinity(), 
                              std::numeric_limits<double>::infinity(), 
                              color == PieceColor::WHITE ? false : true, color);
        
        // Update the best move
        if ((color == PieceColor::WHITE && value > bestValue) ||
            (color == PieceColor::BLACK && value < bestValue)) {
            bestValue = value;
            bestMove = move;
        }
    }
    
    return bestMove;
}

void ChessAI::setSkillLevel(int level) {
    skillLevel = std::min(std::max(level, 1), 10);
}

int ChessAI::getSkillLevel() const {
    return skillLevel;
}

double ChessAI::evaluatePosition(const ChessBoard& board, PieceColor color) const
{
    double score = 0.0;
    
    try {
        // Count material
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                Position pos(r, c);
                const ChessPiece* piece = board.getPiece(pos);
                if (piece) {
                    try {
                        score += evaluatePiece(piece, pos, board);
                    } catch (const std::exception& e) {
                        // Log the error but continue
                        MPChessServer* server = MPChessServer::getInstance();
                        if (server && server->getLogger()) {
                            server->getLogger()->error("evaluatePosition() - Exception evaluating piece: " + std::string(e.what()));
                        }
                    }
                }
            }
        }
        
        // Adjust score for check/checkmate
        if (board.isInCheckmate(PieceColor::WHITE)) {
            return -10000.0;  // Black wins
        } else if (board.isInCheckmate(PieceColor::BLACK)) {
            return 10000.0;   // White wins
        } else if (board.isInCheck(PieceColor::WHITE)) {
            score -= 50.0;    // White is in check
        } else if (board.isInCheck(PieceColor::BLACK)) {
            score += 50.0;    // Black is in check
        }
        
        // Adjust for stalemate
        if (board.isInStalemate(PieceColor::WHITE) || board.isInStalemate(PieceColor::BLACK)) {
            return 0.0;  // Draw
        }
        
        // Mobility (number of legal moves)
        std::vector<ChessMove> whiteMoves = board.getAllValidMoves(PieceColor::WHITE);
        std::vector<ChessMove> blackMoves = board.getAllValidMoves(PieceColor::BLACK);
        score += 0.1 * (whiteMoves.size() - blackMoves.size());
    } catch (const std::exception& e) {
        // Log the error but return a default score
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("evaluatePosition() - Exception: " + std::string(e.what()));
        }
        return 0.0;
    } catch (...) {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("evaluatePosition() - Unknown exception");
        }
        return 0.0;
    }
    
    // Adjust score based on the perspective
    return color == PieceColor::WHITE ? score : -score;
}

double ChessAI::quickEvaluateMove(const ChessBoard& board, const ChessMove& move, PieceColor color) const
{
    MPChessServer* server = MPChessServer::getInstance();
    double score = 0.0;
    
    try {
        const Position& from = move.getFrom();
        const Position& to = move.getTo();
        
        // Get the moving piece
        const ChessPiece* piece = board.getPiece(from);
        if (!piece) return 0.0;
        
        // 1. Material value of captured piece
        const ChessPiece* capturedPiece = board.getPiece(to);
        if (capturedPiece) {
            switch (capturedPiece->getType()) {
                case PieceType::PAWN:   score += 1.0; break;
                case PieceType::KNIGHT: score += 3.0; break;
                case PieceType::BISHOP: score += 3.25; break;
                case PieceType::ROOK:   score += 5.0; break;
                case PieceType::QUEEN:  score += 9.0; break;
                default: break;
            }
        }
        
        // 2. Pawn promotion value
        if (piece->getType() == PieceType::PAWN) {
            if ((color == PieceColor::WHITE && to.row == 7) || 
                (color == PieceColor::BLACK && to.row == 0)) {
                score += 9.0; // Value of a queen
            }
        }
        
        // 3. Center control bonus
        if ((to.row >= 3 && to.row <= 4) && (to.col >= 3 && to.col <= 4)) {
            score += 0.3;
        }
        
        // 4. Development bonus in opening
        if (board.getMoveHistory().size() < 10) {
            if (piece->getType() == PieceType::KNIGHT || piece->getType() == PieceType::BISHOP) {
                score += 0.2;
            }
        }
        
        // 5. King safety in opening/middlegame
        if (piece->getType() == PieceType::KING && board.getMoveHistory().size() < 20) {
            // Penalize early king movement
            score -= 0.5;
        }
        
        // 6. Castling bonus
        if (piece->getType() == PieceType::KING && !piece->hasMoved()) {
            if (std::abs(to.col - from.col) == 2) {
                score += 1.0; // Bonus for castling
            }
        }
        
        // 7. Pawn advancement
        if (piece->getType() == PieceType::PAWN) {
            if (color == PieceColor::WHITE) {
                score += 0.05 * to.row; // Bonus for advancing white pawns
            } else {
                score += 0.05 * (7 - to.row); // Bonus for advancing black pawns
            }
        }
        
        // 8. Mobility (simplified)
        // Just count the number of squares the piece can move to from the destination
        auto tempBoard = board.clone();
        tempBoard->movePiece(move, true); // Just validate, don't actually move
        
        const ChessPiece* movedPiece = tempBoard->getPiece(to);
        if (movedPiece) {
            std::vector<Position> possibleMoves = movedPiece->getPossibleMoves(to, *tempBoard);
            score += 0.05 * possibleMoves.size();
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessAI::quickEvaluateMove() - Move " + move.toAlgebraic() + 
                                      " evaluated to " + std::to_string(score));
        }
        
        return score;
    } catch (const std::exception& e) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessAI::quickEvaluateMove() - Exception: " + std::string(e.what()));
        }
        return 0.0;
    } catch (...) {
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessAI::quickEvaluateMove() - Unknown exception");
        }
        return 0.0;
    }
}

/*
std::vector<std::pair<ChessMove, double>> ChessAI::getMoveRecommendations(
    const ChessBoard& board, PieceColor color, int maxRecommendations)
{
    std::vector<std::pair<ChessMove, double>> recommendations;
    
    try {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessAI::getMoveRecommendations() - Generating recommendations for " + 
                                      std::string(color == PieceColor::WHITE ? "white" : "black"));
        }
        
        // Get all valid moves for the current color
        std::vector<ChessMove> validMoves;
        try {
            validMoves = board.getAllValidMoves(color);
            
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessAI::getMoveRecommendations() - Found " + 
                                          std::to_string(validMoves.size()) + " valid moves");
            }
        }
        catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("ChessAI::getMoveRecommendations() - Exception getting valid moves: " + 
                                         std::string(e.what()));
            }
            return recommendations;
        }
        
        // Use a simplified evaluation for each move
        for (const ChessMove& move : validMoves) {
            double score = 0.0;
            
            // Check if the move captures a piece
            const Position& to = move.getTo();
            const ChessPiece* capturedPiece = board.getPiece(to);
            if (capturedPiece) {
                // Add value for capturing pieces
                switch (capturedPiece->getType()) {
                    case PieceType::PAWN: score += 1.0; break;
                    case PieceType::KNIGHT: score += 3.0; break;
                    case PieceType::BISHOP: score += 3.25; break;
                    case PieceType::ROOK: score += 5.0; break;
                    case PieceType::QUEEN: score += 9.0; break;
                    default: break;
                }
            }
            
            // Check if the move is a pawn promotion
            if (move.getPromotionType() != PieceType::EMPTY) {
                score += 8.0; // Heavily favor promotions
            }
            
            // Add positional score based on the destination square
            const Position& from = move.getFrom();
            const ChessPiece* piece = board.getPiece(from);
            if (piece) {
                // Center control bonus
                if ((to.row >= 3 && to.row <= 4) && (to.col >= 3 && to.col <= 4)) {
                    score += 0.5;
                }
                
                // Pawn advancement bonus
                if (piece->getType() == PieceType::PAWN) {
                    if (color == PieceColor::WHITE) {
                        score += 0.1 * to.row; // Bonus for advancing white pawns
                    } else {
                        score += 0.1 * (7 - to.row); // Bonus for advancing black pawns
                    }
                }
                
                // Development bonus for knights and bishops in opening
                if ((piece->getType() == PieceType::KNIGHT || piece->getType() == PieceType::BISHOP) && 
                    board.getMoveHistory().size() < 10) {
                    score += 0.3;
                }
            }
            
            recommendations.emplace_back(move, score);
        }
        
        // Sort by score (best moves first)
        std::sort(recommendations.begin(), recommendations.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Limit the number of recommendations
        if (recommendations.size() > maxRecommendations) {
            recommendations.resize(maxRecommendations);
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessAI::getMoveRecommendations() - Generated " + 
                                      std::to_string(recommendations.size()) + " recommendations");
        }
    } catch (const std::exception& e) {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessAI::getMoveRecommendations() - Exception: " + std::string(e.what()));
        }
    } catch (...) {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessAI::getMoveRecommendations() - Unknown exception");
        }
    }
    
    return recommendations;
}
*/

std::vector<std::pair<ChessMove, double>> ChessAI::getMoveRecommendations(
    const ChessBoard& board, PieceColor color, int maxRecommendations)
{
    std::vector<std::pair<ChessMove, double>> recommendations;
    
    try {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessAI::getMoveRecommendations() - Generating recommendations for " + 
                                      std::string(color == PieceColor::WHITE ? "white" : "black"));
        }
        
        // Get all valid moves for the current color
        std::vector<ChessMove> validMoves;
        try {
            validMoves = board.getAllValidMoves(color);
            
            if (server && server->getLogger()) {
                server->getLogger()->debug("ChessAI::getMoveRecommendations() - Found " + 
                                          std::to_string(validMoves.size()) + " valid moves");
            }
        }
        catch (const std::exception& e) {
            if (server && server->getLogger()) {
                server->getLogger()->error("ChessAI::getMoveRecommendations() - Exception getting valid moves: " + 
                                         std::string(e.what()));
            }
            return recommendations;
        }
        
        // Use quick evaluation for each move
        for (const ChessMove& move : validMoves) {
            double score = quickEvaluateMove(board, move, color);
            recommendations.emplace_back(move, score);
        }
        
        // Sort by score (best moves first)
        std::sort(recommendations.begin(), recommendations.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Limit to requested number of recommendations
        if (recommendations.size() > maxRecommendations) {
            recommendations.resize(maxRecommendations);
        }
        
        if (server && server->getLogger()) {
            server->getLogger()->debug("ChessAI::getMoveRecommendations() - Returning " + 
                                      std::to_string(recommendations.size()) + " recommendations");
        }
    } catch (const std::exception& e) {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessAI::getMoveRecommendations() - Exception: " + std::string(e.what()));
        }
    } catch (...) {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("ChessAI::getMoveRecommendations() - Unknown exception");
        }
    }
    
    return recommendations;
}

double ChessAI::minimax(const ChessBoard& board, int depth, double alpha, double beta, 
                       bool maximizingPlayer, PieceColor aiColor) {
    if (depth == 0 || board.isGameOver()) {
        return evaluatePosition(board, aiColor);
    }
    
    PieceColor currentColor = maximizingPlayer ? PieceColor::WHITE : PieceColor::BLACK;
    std::vector<ChessMove> validMoves = board.getAllValidMoves(currentColor);
    
    if (validMoves.empty()) {
        // No valid moves, but not checkmate or stalemate (should not happen)
        return evaluatePosition(board, aiColor);
    }
    
    if (maximizingPlayer) {
        double maxEval = -std::numeric_limits<double>::infinity();
        for (const ChessMove& move : validMoves) {
            auto tempBoard = board.clone();
            tempBoard->movePiece(move);
            double eval = minimax(*tempBoard, depth - 1, alpha, beta, false, aiColor);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                break;  // Beta cutoff
            }
        }
        return maxEval;
    } else {
        double minEval = std::numeric_limits<double>::infinity();
        for (const ChessMove& move : validMoves) {
            auto tempBoard = board.clone();
            tempBoard->movePiece(move);
            double eval = minimax(*tempBoard, depth - 1, alpha, beta, true, aiColor);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                break;  // Alpha cutoff
            }
        }
        return minEval;
    }
}

int ChessAI::getSearchDepth() const {
    // Adjust search depth based on skill level
    switch (skillLevel) {
        case 1: return 1;
        case 2: return 1;
        case 3: return 2;
        case 4: return 2;
        case 5: return 3;
        case 6: return 3;
        case 7: return 4;
        case 8: return 4;
        case 9: return 5;
        case 10: return 5;
        default: return 3;
    }
}

double ChessAI::evaluatePiece(const ChessPiece* piece, const Position& pos, const ChessBoard& board) const
{
    if (!piece) return 0.0;
    
    double value = 0.0;
    
    try {
        // Base piece values
        PieceType type;
        try {
            type = piece->getType();
        } catch (const std::exception& e) {
            // Use RTTI to determine piece type
            if (dynamic_cast<const Pawn*>(piece)) {
                type = PieceType::PAWN;
            } else if (dynamic_cast<const Knight*>(piece)) {
                type = PieceType::KNIGHT;
            } else if (dynamic_cast<const Bishop*>(piece)) {
                type = PieceType::BISHOP;
            } else if (dynamic_cast<const Rook*>(piece)) {
                type = PieceType::ROOK;
            } else if (dynamic_cast<const Queen*>(piece)) {
                type = PieceType::QUEEN;
            } else if (dynamic_cast<const King*>(piece)) {
                type = PieceType::KING;
            } else {
                type = PieceType::EMPTY;
            }
        }
        
        switch (type) {
            case PieceType::PAWN:   value = 1.0; break;
            case PieceType::KNIGHT: value = 3.0; break;
            case PieceType::BISHOP: value = 3.25; break;
            case PieceType::ROOK:   value = 5.0; break;
            case PieceType::QUEEN:  value = 9.0; break;
            case PieceType::KING:   value = 100.0; break;
            case PieceType::EMPTY:  value = 0.0; break;
            default: value = 0.0; break;
        }
        
        // Adjust for position using piece-square tables
        int row = pos.row;
        int col = pos.col;
        
        // Flip the row for black pieces to use the same tables
        PieceColor color;
        try {
            color = piece->getColor();
        } catch (const std::exception& e) {
            // Default to WHITE if we can't get the color
            color = PieceColor::WHITE;
        }
        
        if (color == PieceColor::BLACK) {
            row = 7 - row;
        }
        
        switch (type) {
            case PieceType::PAWN:
                value += pawnTable[row][col] * 0.1;
                break;
            case PieceType::KNIGHT:
                value += knightTable[row][col] * 0.1;
                break;
            case PieceType::BISHOP:
                value += bishopTable[row][col] * 0.1;
                break;
            case PieceType::ROOK:
                value += rookTable[row][col] * 0.1;
                break;
            case PieceType::QUEEN:
                value += queenTable[row][col] * 0.1;
                break;
            case PieceType::KING:
                {
                    // Use different tables for middle game and end game
                    // Simple heuristic: if both sides have queens, it's middle game
                    bool isEndGame = true;
                    for (int r = 0; r < 8; ++r) {
                        for (int c = 0; c < 8; ++c) {
                            const ChessPiece* p = board.getPiece(Position(r, c));
                            if (p && p->getType() == PieceType::QUEEN) {
                                isEndGame = false;
                                break;
                            }
                        }
                        if (!isEndGame) break;
                    }
                    
                    if (isEndGame) {
                        value += kingEndGameTable[row][col] * 0.1;
                    } else {
                        value += kingMiddleGameTable[row][col] * 0.1;
                    }
                    break;
                }
            case PieceType::EMPTY:
                value += 0.0;
                break;
        }
    } catch (const std::exception& e) {
        // Log the error but return a default value
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("evaluatePiece() - Exception: " + std::string(e.what()));
        }
        return 0.0;
    } catch (...) {
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->getLogger()) {
            server->getLogger()->error("evaluatePiece() - Unknown exception");
        }
        return 0.0;
    }
    
    // Adjust sign based on color
    return piece->getColor() == PieceColor::WHITE ? value : -value;
}

// Implementation of ChessMatchmaker class
ChessMatchmaker::ChessMatchmaker() {
}

void ChessMatchmaker::addPlayer(ChessPlayer* player) {
    // Check if player is already in the queue
    if (std::find(playerQueue.begin(), playerQueue.end(), player) != playerQueue.end()) {
        return;
    }
    
    playerQueue.push_back(player);
    queueTimes[player] = QDateTime::currentDateTime();
}

void ChessMatchmaker::removePlayer(ChessPlayer* player) {
    auto it = std::find(playerQueue.begin(), playerQueue.end(), player);
    if (it != playerQueue.end()) {
        playerQueue.erase(it);
        queueTimes.erase(player);
    }
}

std::vector<std::pair<ChessPlayer*, ChessPlayer*>> ChessMatchmaker::matchPlayers()
{
    std::vector<std::pair<ChessPlayer*, ChessPlayer*>> matches;
    
    // Make a copy of the queue to avoid modifying it while iterating
    std::vector<ChessPlayer*> queue = playerQueue;
    
    // Track players that have been matched to avoid duplicates
//  std::set<ChessPlayer*, std::less<ChessPlayer*>, std::allocator<ChessPlayer*>> matchedPlayers;
    std::set<ChessPlayer*> matchedPlayers;
    
    for (size_t i = 0; i < queue.size(); ++i) {
        ChessPlayer* player = queue[i];
        
        // Skip if player is null
        if (!player) continue;
        
        // Skip if player has already been matched
        if (matchedPlayers.find(player) != matchedPlayers.end()) {
            continue;
        }

        // Skip if player is already in a game
        MPChessServer* server = MPChessServer::getInstance();
        if (server && server->isPlayerInGame(player)) {
            continue;
        }
        
        ChessPlayer* bestMatch = findBestMatch(player);
        if (bestMatch && matchedPlayers.find(bestMatch) == matchedPlayers.end() && 
            !(server && server->isPlayerInGame(bestMatch))) {
            matches.emplace_back(player, bestMatch);
            
            // Mark both players as matched
            matchedPlayers.insert(player);
            matchedPlayers.insert(bestMatch);
            
            // Remove matched players from the queue
            removePlayer(player);
            removePlayer(bestMatch);
        }
    }
    
    return matches;
}

std::vector<ChessPlayer*> ChessMatchmaker::checkTimeouts(int timeoutSeconds)
{
    std::vector<ChessPlayer*> timedOutPlayers;
    QDateTime now = QDateTime::currentDateTime();
    
    for (auto it = queueTimes.begin(); it != queueTimes.end(); ) {
        ChessPlayer* player = it->first;
        QDateTime queueTime = it->second;
        
        if (queueTime.secsTo(now) > timeoutSeconds) {
            timedOutPlayers.push_back(player);
            it = queueTimes.erase(it);
            
            // Remove from playerQueue as well
            auto queueIt = std::find(playerQueue.begin(), playerQueue.end(), player);
            if (queueIt != playerQueue.end()) {
                playerQueue.erase(queueIt);
            }
        } else {
            ++it;
        }
    }
    
    return timedOutPlayers;
}

int ChessMatchmaker::getQueueSize() const {
    return static_cast<int>(playerQueue.size());
}

void ChessMatchmaker::clearQueue() {
    playerQueue.clear();
    queueTimes.clear();
}

ChessPlayer* ChessMatchmaker::findBestMatch(ChessPlayer* player) const
{
    if (!player) return nullptr;
    
    ChessPlayer* bestMatch = nullptr;
    double bestScore = std::numeric_limits<double>::infinity();
    QDateTime now = QDateTime::currentDateTime();
    
    for (ChessPlayer* candidate : playerQueue) {
        if (!candidate || candidate == player) continue;
        
        // Calculate rating difference score
        double score = getRatingDifferenceScore(player->getRating(), candidate->getRating());
        
        // Consider waiting time as well
        auto it = queueTimes.find(candidate);
        if (it != queueTimes.end()) {
            int waitTime = it->second.secsTo(now);
            score -= waitTime * 0.1;  // Reduce score for players waiting longer
        }
        
        if (score < bestScore) {
            bestScore = score;
            bestMatch = candidate;
        }
    }
    
    return bestMatch;
}

double ChessMatchmaker::getRatingDifferenceScore(int rating1, int rating2) const {
    return std::abs(rating1 - rating2);
}

// Implementation of ChessRatingSystem class
ChessRatingSystem::ChessRatingSystem() {
}

std::pair<int, int> ChessRatingSystem::calculateNewRatings(int rating1, int rating2, GameResult result) {
    double score1, score2;
    
    switch (result) {
        case GameResult::WHITE_WIN:
            score1 = 1.0;
            score2 = 0.0;
            break;
        case GameResult::BLACK_WIN:
            score1 = 0.0;
            score2 = 1.0;
            break;
        case GameResult::DRAW:
            score1 = 0.5;
            score2 = 0.5;
            break;
        default:
            // Game in progress, no rating change
            return {rating1, rating2};
    }
    
    double expected1 = calculateExpectedScore(rating1, rating2);
    double expected2 = calculateExpectedScore(rating2, rating1);
    
    int k1 = getKFactor(rating1, 0);  // Assuming we don't track games played here
    int k2 = getKFactor(rating2, 0);
    
    int newRating1 = static_cast<int>(rating1 + k1 * (score1 - expected1));
    int newRating2 = static_cast<int>(rating2 + k2 * (score2 - expected2));
    
    return {newRating1, newRating2};
}

double ChessRatingSystem::calculateExpectedScore(int rating1, int rating2) const {
    return 1.0 / (1.0 + std::pow(10.0, (rating2 - rating1) / 400.0));
}

int ChessRatingSystem::getKFactor(int rating, int gamesPlayed) const {
    if (rating >= MASTER_RATING_THRESHOLD) {
        return MASTER_K_FACTOR;
    } else if (gamesPlayed >= GAMES_THRESHOLD) {
        return EXPERIENCED_K_FACTOR;
    } else {
        return DEFAULT_K_FACTOR;
    }
}

// Implementation of ChessAnalysisEngine class
ChessAnalysisEngine::ChessAnalysisEngine() : analysisAI(10) {
}

QJsonObject ChessAnalysisEngine::analyzeGame(const ChessGame& game) {
    QJsonObject analysis;

    // If Stockfish is available, use it for analysis
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->stockfishConnector && server->stockfishConnector->isInitialized()) {
        return server->stockfishConnector->analyzeGame(game);
    }

    // Otherwise, use our built-in analysis    
    // Game overview
    analysis["gameId"] = QString::fromStdString(game.getGameId());
    analysis["whitePlayer"] = QString::fromStdString(game.getWhitePlayer()->getUsername());
    analysis["blackPlayer"] = QString::fromStdString(game.getBlackPlayer()->getUsername());
    analysis["result"] = [&game]() -> QString {
        switch (game.getResult()) {
            case GameResult::WHITE_WIN: return "white_win";
            case GameResult::BLACK_WIN: return "black_win";
            case GameResult::DRAW: return "draw";
            default: return "in_progress";
        }
    }();
    
    // Analyze each move
    QJsonArray moveAnalysis;
    const ChessBoard* board = game.getBoard();
    auto moveHistory = board->getMoveHistory();
    
    // Create a temporary board to replay the game
    auto tempBoard = std::make_unique<ChessBoard>();
    
    for (size_t i = 0; i < moveHistory.size(); ++i) {
        const ChessMove& move = moveHistory[i];
        
        // Analyze the position before the move
        double evalBefore = evaluatePositionDeeply(*tempBoard, tempBoard->getCurrentTurn());
        
        // Make the move
        tempBoard->movePiece(move);
        
        // Analyze the position after the move
        double evalAfter = evaluatePositionDeeply(*tempBoard, tempBoard->getCurrentTurn());
        
        // Calculate evaluation change
        double evalChange = evalAfter - evalBefore;
        
        // Classify the move
        std::string classification = classifyMove(evalBefore, evalAfter);
        
        // Create move analysis object
        QJsonObject moveObj;
        moveObj["moveNumber"] = static_cast<int>(i / 2) + 1;
        moveObj["color"] = (i % 2 == 0) ? "white" : "black";
        moveObj["move"] = QString::fromStdString(move.toAlgebraic());
        moveObj["standardNotation"] = QString::fromStdString(move.toStandardNotation(*tempBoard));
        moveObj["evaluationBefore"] = evalBefore;
        moveObj["evaluationAfter"] = evalAfter;
        moveObj["evaluationChange"] = evalChange;
        moveObj["classification"] = QString::fromStdString(classification);
        moveObj["isCapture"] = isCapture(*tempBoard, move);
        moveObj["isCheck"] = putsInCheck(*tempBoard, move);
        
        moveAnalysis.append(moveObj);
    }
    
    analysis["moveAnalysis"] = moveAnalysis;
    
    // Add mistakes analysis
    analysis["mistakes"] = identifyMistakes(game);
    
    // Add critical moments
    analysis["criticalMoments"] = identifyCriticalMoments(game);
    
    // Add game summary
    analysis["summary"] = QString::fromStdString(generateGameSummary(game));
    
    return analysis;
}

QJsonObject ChessAnalysisEngine::analyzeMove(const ChessBoard& boardBefore, const ChessMove& move) {
    QJsonObject analysis;

    // If Stockfish is available, use it for analysis
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->stockfishConnector && server->stockfishConnector->isInitialized()) {
        // Create a copy of the board and make the move
        auto boardAfter = boardBefore.clone();
        boardAfter->movePiece(move);
        
        // Analyze both positions
        QJsonObject beforeAnalysis = server->stockfishConnector->analyzePosition(boardBefore);
        QJsonObject afterAnalysis = server->stockfishConnector->analyzePosition(*boardAfter);
        
        // Combine the analyses
        analysis["move"] = QString::fromStdString(move.toAlgebraic());
        analysis["standardNotation"] = QString::fromStdString(move.toStandardNotation(boardBefore));
        analysis["evaluationBefore"] = beforeAnalysis["evaluation"];
        analysis["evaluationAfter"] = afterAnalysis["evaluation"];
        analysis["evaluationChange"] = afterAnalysis["evaluation"].toDouble() - beforeAnalysis["evaluation"].toDouble();
        
        // Classify the move
        double evalChange = analysis["evaluationChange"].toDouble();
        if (evalChange > 2.0) analysis["classification"] = "Brilliant";
        else if (evalChange > 1.0) analysis["classification"] = "Good";
        else if (evalChange > 0.3) analysis["classification"] = "Accurate";
        else if (evalChange > -0.3) analysis["classification"] = "Normal";
        else if (evalChange > -1.0) analysis["classification"] = "Inaccuracy";
        else if (evalChange > -2.0) analysis["classification"] = "Mistake";
        else analysis["classification"] = "Blunder";
        
        analysis["isCapture"] = isCapture(boardBefore, move);
        analysis["isCheck"] = putsInCheck(*boardAfter, move);
        analysis["alternatives"] = beforeAnalysis["bestMoves"];
        
        return analysis;
    }

    // Otherwise, use our built-in analysis
    // Create a copy of the board and make the move
    auto boardAfter = boardBefore.clone();
    boardAfter->movePiece(move);
    
    // Evaluate positions
    double evalBefore = evaluatePositionDeeply(boardBefore, boardBefore.getCurrentTurn());
    double evalAfter = evaluatePositionDeeply(*boardAfter, boardAfter->getCurrentTurn());
    
    // Calculate evaluation change
    double evalChange = evalAfter - evalBefore;
    
    // Classify the move
    std::string classification = classifyMove(evalBefore, evalAfter);
    
    // Get alternative moves
    std::vector<std::pair<ChessMove, double>> alternatives = 
        getMoveRecommendations(boardBefore, boardBefore.getCurrentTurn(), 3);
    
    // Create the analysis object
    analysis["move"] = QString::fromStdString(move.toAlgebraic());
    analysis["standardNotation"] = QString::fromStdString(move.toStandardNotation(boardBefore));
    analysis["evaluationBefore"] = evalBefore;
    analysis["evaluationAfter"] = evalAfter;
    analysis["evaluationChange"] = evalChange;
    analysis["classification"] = QString::fromStdString(classification);
    analysis["isCapture"] = isCapture(boardBefore, move);
    analysis["isCheck"] = putsInCheck(*boardAfter, move);
    
    // Add alternative moves
    QJsonArray alternativesArray;
    for (const auto& [altMove, altEval] : alternatives) {
        if (altMove == move) continue;  // Skip the actual move
        
        QJsonObject altObj;
        altObj["move"] = QString::fromStdString(altMove.toAlgebraic());
        altObj["standardNotation"] = QString::fromStdString(altMove.toStandardNotation(boardBefore));
        altObj["evaluation"] = altEval;
        alternativesArray.append(altObj);
    }
    analysis["alternatives"] = alternativesArray;
    
    return analysis;
}

std::vector<std::pair<ChessMove, double>> ChessAnalysisEngine::getMoveRecommendations(
    const ChessBoard& board, PieceColor color, int maxRecommendations) {

    // If Stockfish is available, use it for recommendations
    MPChessServer* server = MPChessServer::getInstance();
    if (server && server->stockfishConnector && server->stockfishConnector->isInitialized()) {
        server->stockfishConnector->setPosition(board);
        return server->stockfishConnector->getMoveRecommendations(maxRecommendations);
    }

    // Otherwise, use our built-in AI
    return analysisAI.getMoveRecommendations(board, color, maxRecommendations);
}

QJsonObject ChessAnalysisEngine::identifyMistakes(const ChessGame& game) {
    QJsonObject mistakes;
    
    QJsonArray blunders;
    QJsonArray errors;
    QJsonArray inaccuracies;
    
    const ChessBoard* board = game.getBoard();
    auto moveHistory = board->getMoveHistory();
    
    // Create a temporary board to replay the game
    auto tempBoard = std::make_unique<ChessBoard>();
    
    for (size_t i = 0; i < moveHistory.size(); ++i) {
        const ChessMove& move = moveHistory[i];
        
        // Analyze the position before the move
        double evalBefore = evaluatePositionDeeply(*tempBoard, tempBoard->getCurrentTurn());
        
        // Make the move
        tempBoard->movePiece(move);
        
        // Analyze the position after the move
        double evalAfter = evaluatePositionDeeply(*tempBoard, tempBoard->getCurrentTurn());
        
        // Calculate evaluation change
        double evalChange = evalAfter - evalBefore;
        
        // Determine if this is a mistake
        QJsonObject mistakeObj;
        mistakeObj["moveNumber"] = static_cast<int>(i / 2) + 1;
        mistakeObj["color"] = (i % 2 == 0) ? "white" : "black";
        mistakeObj["move"] = QString::fromStdString(move.toAlgebraic());
        mistakeObj["standardNotation"] = QString::fromStdString(move.toStandardNotation(*tempBoard));
        mistakeObj["evaluationBefore"] = evalBefore;
        mistakeObj["evaluationAfter"] = evalAfter;
        mistakeObj["evaluationChange"] = evalChange;
        
        // Classify based on evaluation change
        if (std::abs(evalChange) >= 2.0) {
            blunders.append(mistakeObj);
        } else if (std::abs(evalChange) >= 1.0) {
            errors.append(mistakeObj);
        } else if (std::abs(evalChange) >= 0.5) {
            inaccuracies.append(mistakeObj);
        }
    }
    
    mistakes["blunders"] = blunders;
    mistakes["errors"] = errors;
    mistakes["inaccuracies"] = inaccuracies;
    
    return mistakes;
}

QJsonObject ChessAnalysisEngine::identifyCriticalMoments(const ChessGame& game) {
    QJsonObject criticalMoments;
    
    QJsonArray openingMoments;
    QJsonArray middleGameMoments;
    QJsonArray endGameMoments;
    
    const ChessBoard* board = game.getBoard();
    auto moveHistory = board->getMoveHistory();
    
    // Create a temporary board to replay the game
    auto tempBoard = std::make_unique<ChessBoard>();
    
    // Track the largest evaluation swings
    double largestSwing = 0.0;
    size_t largestSwingIndex = 0;
    
    // Track material count to identify game phases
    int phase = 0;  // 0: opening, 1: middlegame, 2: endgame
    
    for (size_t i = 0; i < moveHistory.size(); ++i) {
        const ChessMove& move = moveHistory[i];
        
        // Analyze the position before the move
        double evalBefore = evaluatePositionDeeply(*tempBoard, tempBoard->getCurrentTurn());
        
        // Make the move
        tempBoard->movePiece(move);
        
        // Analyze the position after the move
        double evalAfter = evaluatePositionDeeply(*tempBoard, tempBoard->getCurrentTurn());
        
        // Calculate evaluation change
        double evalChange = evalAfter - evalBefore;
        
        // Track largest swing
        if (std::abs(evalChange) > std::abs(largestSwing)) {
            largestSwing = evalChange;
            largestSwingIndex = i;
        }
        
        // Create moment object
        QJsonObject momentObj;
        momentObj["moveNumber"] = static_cast<int>(i / 2) + 1;
        momentObj["color"] = (i % 2 == 0) ? "white" : "black";
        momentObj["move"] = QString::fromStdString(move.toAlgebraic());
        momentObj["standardNotation"] = QString::fromStdString(move.toStandardNotation(*tempBoard));
        momentObj["evaluationBefore"] = evalBefore;
        momentObj["evaluationAfter"] = evalAfter;
        momentObj["evaluationChange"] = evalChange;
        
        // Determine game phase based on move number and material
        if (i < 10) {
            phase = 0;  // Opening
        } else {
            // Count material to determine phase
            int materialCount = 0;
            for (int r = 0; r < 8; ++r) {
                for (int c = 0; c < 8; ++c) {
                    const ChessPiece* piece = tempBoard->getPiece(Position(r, c));
                    if (piece && piece->getType() != PieceType::KING) {
                        materialCount++;
                    }
                }
            }
            
            if (materialCount <= 12) {
                phase = 2;  // Endgame
            } else {
                phase = 1;  // Middlegame
            }
        }
        
        // Add to the appropriate phase array
        if (std::abs(evalChange) >= 0.5) {  // Only add significant moments
            if (phase == 0) {
                openingMoments.append(momentObj);
            } else if (phase == 1) {
                middleGameMoments.append(momentObj);
            } else {
                endGameMoments.append(momentObj);
            }
        }
    }
    
    criticalMoments["opening"] = openingMoments;
    criticalMoments["middleGame"] = middleGameMoments;
    criticalMoments["endGame"] = endGameMoments;
    
    // Add the largest swing
    if (largestSwingIndex < moveHistory.size()) {
        const ChessMove& move = moveHistory[largestSwingIndex];
        QJsonObject largestSwingObj;
        largestSwingObj["moveNumber"] = static_cast<int>(largestSwingIndex / 2) + 1;
        largestSwingObj["color"] = (largestSwingIndex % 2 == 0) ? "white" : "black";
        largestSwingObj["move"] = QString::fromStdString(move.toAlgebraic());
        largestSwingObj["evaluationChange"] = largestSwing;
        criticalMoments["largestSwing"] = largestSwingObj;
    }
    
    return criticalMoments;
}

std::string ChessAnalysisEngine::generateGameSummary(const ChessGame& game) {
    std::stringstream summary;
    
    // Get basic game info
    summary << "Game between " << game.getWhitePlayer()->getUsername() << " (White) and "
            << game.getBlackPlayer()->getUsername() << " (Black)\n";
    
    // Get result
    switch (game.getResult()) {
        case GameResult::WHITE_WIN:
            summary << "Result: 1-0 (White won)\n";
            break;
        case GameResult::BLACK_WIN:
            summary << "Result: 0-1 (Black won)\n";
            break;
        case GameResult::DRAW:
            summary << "Result: 1/2-1/2 (Draw)\n";
            break;
        default:
            summary << "Result: Game in progress\n";
            break;
    }
    
    // Analyze the game
    QJsonObject analysis = analyzeGame(game);
    
    // Get mistake counts
    QJsonObject mistakes = analysis["mistakes"].toObject();
    int blunderCount = mistakes["blunders"].toArray().size();
    int errorCount = mistakes["errors"].toArray().size();
    int inaccuracyCount = mistakes["inaccuracies"].toArray().size();
    
    summary << "\nGame Statistics:\n";
    summary << "- White blunders: " << countPlayerMistakes(mistakes["blunders"].toArray(), "white") << "\n";
    summary << "- White errors: " << countPlayerMistakes(mistakes["errors"].toArray(), "white") << "\n";
    summary << "- White inaccuracies: " << countPlayerMistakes(mistakes["inaccuracies"].toArray(), "white") << "\n";
    summary << "- Black blunders: " << countPlayerMistakes(mistakes["blunders"].toArray(), "black") << "\n";
    summary << "- Black errors: " << countPlayerMistakes(mistakes["errors"].toArray(), "black") << "\n";
    summary << "- Black inaccuracies: " << countPlayerMistakes(mistakes["inaccuracies"].toArray(), "black") << "\n";
    
    // Add critical moment
    if (analysis["criticalMoments"].toObject().contains("largestSwing")) {
        QJsonObject largestSwing = analysis["criticalMoments"].toObject()["largestSwing"].toObject();
        summary << "\nCritical Moment:\n";
        summary << "Move " << largestSwing["moveNumber"].toInt() << " by " 
                << largestSwing["color"].toString().toStdString() << ": " 
                << largestSwing["move"].toString().toStdString() << "\n";
        summary << "This move caused an evaluation change of " 
                << largestSwing["evaluationChange"].toDouble() << "\n";
    }
    
    // Add overall assessment
    summary << "\nOverall Assessment:\n";
    if (blunderCount == 0 && errorCount <= 1) {
        summary << "Excellent game with very few mistakes.\n";
    } else if (blunderCount <= 1 && errorCount <= 3) {
        summary << "Good game with some minor errors.\n";
    } else if (blunderCount <= 3) {
        summary << "Average game with several mistakes.\n";
    } else {
        summary << "Game had multiple significant mistakes.\n";
    }
    
    return summary.str();
}

double ChessAnalysisEngine::evaluatePositionDeeply(const ChessBoard& board, PieceColor color) {
    return analysisAI.evaluatePosition(board, color);
}

std::string ChessAnalysisEngine::classifyMove(double evalBefore, double evalAfter) {
    double diff = evalAfter - evalBefore;
    
    if (diff > 2.0) return "Brilliant";
    if (diff > 1.0) return "Good";
    if (diff > 0.3) return "Accurate";
    if (diff > -0.3) return "Normal";
    if (diff > -1.0) return "Inaccuracy";
    if (diff > -2.0) return "Mistake";
    return "Blunder";
}

bool ChessAnalysisEngine::isCapture(const ChessBoard& board, const ChessMove& move) {
    return board.getPiece(move.getTo()) != nullptr || board.isEnPassantCapture(move);
}

bool ChessAnalysisEngine::putsInCheck(const ChessBoard& board, const ChessMove& move) {
    // Get the color of the piece being moved
    const ChessPiece* piece = board.getPiece(move.getFrom());
    if (!piece) return false;
    
    PieceColor color = piece->getColor();
    PieceColor opponentColor = (color == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
    
    // Create a copy of the board and make the move
    auto tempBoard = board.clone();
    tempBoard->movePiece(move);
    
    // Check if the opponent is in check
    return tempBoard->isInCheck(opponentColor);
}

int ChessAnalysisEngine::countPlayerMistakes(const QJsonArray& mistakes, const QString& color) {
    int count = 0;
    for (const QJsonValue& value : mistakes) {
        QJsonObject mistake = value.toObject();
        if (mistake["color"].toString() == color) {
            count++;
        }
    }
    return count;
}

// Implementation of ChessSerializer class
ChessSerializer::ChessSerializer() {
}

QJsonObject ChessSerializer::serializeGame(const ChessGame& game) {
    return game.serialize();
}

std::unique_ptr<ChessGame> ChessSerializer::deserializeGame(const QJsonObject& json, 
                                                         ChessPlayer* whitePlayer, 
                                                         ChessPlayer* blackPlayer) {
    return ChessGame::deserialize(json, whitePlayer, blackPlayer);
}

bool ChessSerializer::saveGameToFile(const ChessGame& game, const std::string& filename) {
    QJsonObject json = serializeGame(game);
    QJsonDocument doc(json);
    
    QFile file(QString::fromStdString(filename));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

std::unique_ptr<ChessGame> ChessSerializer::loadGameFromFile(const std::string& filename,
                                                          ChessPlayer* whitePlayer,
                                                          ChessPlayer* blackPlayer) {
    QFile file(QString::fromStdString(filename));
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        return nullptr;
    }
    
    return deserializeGame(doc.object(), whitePlayer, blackPlayer);
}

QJsonObject ChessSerializer::serializePlayer(const ChessPlayer& player) {
    return player.toJson();
}

std::unique_ptr<ChessPlayer> ChessSerializer::deserializePlayer(const QJsonObject& json) {
    ChessPlayer player = ChessPlayer::fromJson(json);
    return std::make_unique<ChessPlayer>(player);
}

bool ChessSerializer::savePlayerToFile(const ChessPlayer& player, const std::string& filename) {
    QJsonObject json = serializePlayer(player);
    QJsonDocument doc(json);
    
    QFile file(QString::fromStdString(filename));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

std::unique_ptr<ChessPlayer> ChessSerializer::loadPlayerFromFile(const std::string& filename) {
    QFile file(QString::fromStdString(filename));
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        return nullptr;
    }
    
    return deserializePlayer(doc.object());
}

QJsonObject ChessSerializer::serializeBoard(const ChessBoard& board) {
    QJsonObject json;
    
    // Serialize pieces
    QJsonArray piecesArray;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Position pos(r, c);
            const ChessPiece* piece = board.getPiece(pos);
            if (piece) {
                piecesArray.append(serializePiece(piece, pos));
            }
        }
    }
    json["pieces"] = piecesArray;
    
    // Serialize current turn
    json["currentTurn"] = (board.getCurrentTurn() == PieceColor::WHITE) ? "white" : "black";
    
    // Serialize en passant target
    Position enPassantTarget = board.getEnPassantTarget();
    if (enPassantTarget.isValid()) {
        json["enPassantTarget"] = QString::fromStdString(enPassantTarget.toAlgebraic());
    } else {
        json["enPassantTarget"] = "";
    }
    
    // Serialize move history
    QJsonArray moveHistoryArray;
    for (const ChessMove& move : board.getMoveHistory()) {
        moveHistoryArray.append(serializeMove(move));
    }
    json["moveHistory"] = moveHistoryArray;
    
    // Serialize captured pieces
    QJsonArray whiteCapturedArray;
    for (PieceType type : board.getCapturedPieces(PieceColor::WHITE)) {
        whiteCapturedArray.append([type]() -> QString {
            switch (type) {
                case PieceType::PAWN: return "pawn";
                case PieceType::KNIGHT: return "knight";
                case PieceType::BISHOP: return "bishop";
                case PieceType::ROOK: return "rook";
                case PieceType::QUEEN: return "queen";
                case PieceType::KING: return "king";
                case PieceType::EMPTY: return "";
                default: return "";
            }
        }());
    }
    json["whiteCaptured"] = whiteCapturedArray;
    
    QJsonArray blackCapturedArray;
    for (PieceType type : board.getCapturedPieces(PieceColor::BLACK)) {
        blackCapturedArray.append([type]() -> QString {
            switch (type) {
                case PieceType::PAWN: return "pawn";
                case PieceType::KNIGHT: return "knight";
                case PieceType::BISHOP: return "bishop";
                case PieceType::ROOK: return "rook";
                case PieceType::QUEEN: return "queen";
                case PieceType::KING: return "king";
                case PieceType::EMPTY: return "";
                default: return "";
            }
        }());
    }
    json["blackCaptured"] = blackCapturedArray;
    
    return json;
}

std::unique_ptr<ChessBoard> ChessSerializer::deserializeBoard(const QJsonObject& json) {
    auto board = std::make_unique<ChessBoard>();
    
    // Clear the board
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            board->board[r][c] = nullptr;
        }
    }
    
    // Deserialize pieces
    QJsonArray piecesArray = json["pieces"].toArray();
    for (const QJsonValue& value : piecesArray) {
        QJsonObject pieceObj = value.toObject();
        Position pos = Position::fromAlgebraic(pieceObj["position"].toString().toStdString());
        if (pos.isValid()) {
            board->board[pos.row][pos.col] = deserializePiece(pieceObj);
        }
    }
    
    // Deserialize current turn
    board->setCurrentTurn(json["currentTurn"].toString() == "white" ? PieceColor::WHITE : PieceColor::BLACK);
    
    // Deserialize en passant target
    std::string enPassantStr = json["enPassantTarget"].toString().toStdString();
    if (!enPassantStr.empty()) {
        board->setEnPassantTarget(Position::fromAlgebraic(enPassantStr));
    }
    
    // Deserialize move history
    QJsonArray moveHistoryArray = json["moveHistory"].toArray();
    for (const QJsonValue& value : moveHistoryArray) {
        board->moveHistory.push_back(deserializeMove(value.toObject()));
    }
    
    // Deserialize captured pieces
    QJsonArray whiteCapturedArray = json["whiteCaptured"].toArray();
    for (const QJsonValue& value : whiteCapturedArray) {
        std::string typeStr = value.toString().toStdString();
        PieceType type = PieceType::PAWN;  // Default
        if (typeStr == "pawn") type = PieceType::PAWN;
        else if (typeStr == "knight") type = PieceType::KNIGHT;
        else if (typeStr == "bishop") type = PieceType::BISHOP;
        else if (typeStr == "rook") type = PieceType::ROOK;
        else if (typeStr == "queen") type = PieceType::QUEEN;
        
        board->capturedWhitePieces.push_back(type);
    }
    
    QJsonArray blackCapturedArray = json["blackCaptured"].toArray();
    for (const QJsonValue& value : blackCapturedArray) {
        std::string typeStr = value.toString().toStdString();
        PieceType type = PieceType::PAWN;  // Default
        if (typeStr == "pawn") type = PieceType::PAWN;
        else if (typeStr == "knight") type = PieceType::KNIGHT;
        else if (typeStr == "bishop") type = PieceType::BISHOP;
        else if (typeStr == "rook") type = PieceType::ROOK;
        else if (typeStr == "queen") type = PieceType::QUEEN;
        
        board->capturedBlackPieces.push_back(type);
    }
    
    return board;
}

QJsonObject ChessSerializer::serializePiece(const ChessPiece* piece, const Position& pos) {
    QJsonObject json;
    
    json["position"] = QString::fromStdString(pos.toAlgebraic());
    json["type"] = [piece]() -> QString {
        switch (piece->getType()) {
            case PieceType::PAWN: return "pawn";
            case PieceType::KNIGHT: return "knight";
            case PieceType::BISHOP: return "bishop";
            case PieceType::ROOK: return "rook";
            case PieceType::QUEEN: return "queen";
            case PieceType::KING: return "king";
            default: return "empty";
        }
    }();
    
    json["color"] = (piece->getColor() == PieceColor::WHITE) ? "white" : "black";
    json["moved"] = piece->hasMoved();
    
    return json;
}

std::unique_ptr<ChessPiece> ChessSerializer::deserializePiece(const QJsonObject& json) {
    std::string typeStr = json["type"].toString().toStdString();
    std::string colorStr = json["color"].toString().toStdString();
    bool moved = json["moved"].toBool();
    
    PieceColor color = (colorStr == "white") ? PieceColor::WHITE : PieceColor::BLACK;
    std::unique_ptr<ChessPiece> piece;
    
    if (typeStr == "pawn") {
        piece = std::make_unique<Pawn>(color);
    } else if (typeStr == "knight") {
        piece = std::make_unique<Knight>(color);
    } else if (typeStr == "bishop") {
        piece = std::make_unique<Bishop>(color);
    } else if (typeStr == "rook") {
        piece = std::make_unique<Rook>(color);
    } else if (typeStr == "queen") {
        piece = std::make_unique<Queen>(color);
    } else if (typeStr == "king") {
        piece = std::make_unique<King>(color);
    } else {
        return nullptr;
    }
    
    piece->setMoved(moved);
    return piece;
}

QJsonObject ChessSerializer::serializeMove(const ChessMove& move) {
    QJsonObject json;
    
    json["from"] = QString::fromStdString(move.getFrom().toAlgebraic());
    json["to"] = QString::fromStdString(move.getTo().toAlgebraic());
    
    if (move.getPromotionType() != PieceType::EMPTY) {
        json["promotion"] = [move]() -> QString {
            switch (move.getPromotionType()) {
                case PieceType::QUEEN: return "queen";
                case PieceType::ROOK: return "rook";
                case PieceType::BISHOP: return "bishop";
                case PieceType::KNIGHT: return "knight";
                default: return "";
            }
        }();
    }
    
    return json;
}

ChessMove ChessSerializer::deserializeMove(const QJsonObject& json) {
    Position from = Position::fromAlgebraic(json["from"].toString().toStdString());
    Position to = Position::fromAlgebraic(json["to"].toString().toStdString());
    
    PieceType promotionType = PieceType::EMPTY;
    if (json.contains("promotion")) {
        std::string promotionStr = json["promotion"].toString().toStdString();
        if (promotionStr == "queen") promotionType = PieceType::QUEEN;
        else if (promotionStr == "rook") promotionType = PieceType::ROOK;
        else if (promotionStr == "bishop") promotionType = PieceType::BISHOP;
        else if (promotionStr == "knight") promotionType = PieceType::KNIGHT;
    }
    
    return ChessMove(from, to, promotionType);
}

// Implementation of ChessLogger class
ChessLogger::ChessLogger(const std::string& logFilePath) : logLevel(0)
{
    logFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
    }
    
    log(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Multiplayer Chess Server Logger initialized <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}

ChessLogger::~ChessLogger()
{
    if (logFile.is_open()) {
        log("Chess Server Logger shutting down");
        logFile.close();
    }
}

void ChessLogger::log(const std::string& message, bool console)
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string logMessage = timestamp + " [INFO] " + message;
    
    if (logFile.is_open()) {
        logFile << logMessage << std::endl;
    }
    
    if (console) {
        std::cout << logMessage << std::endl;
    }
}

void ChessLogger::error(const std::string& message, bool console)
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string logMessage = timestamp + " [ERROR] " + message;
    
    if (logFile.is_open()) {
        logFile << logMessage << std::endl;
    }
    
    if (console) {
        std::cerr << logMessage << std::endl;
    }
}

void ChessLogger::warning(const std::string& message, bool console)
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string logMessage = timestamp + " [WARNING] " + message;
    
    if (logFile.is_open()) {
        logFile << logMessage << std::endl;
    }
    
    if (console) {
        std::cout << logMessage << std::endl;
    }
}

void ChessLogger::debug(const std::string& message, bool console)
{
    if (logLevel < 1) return;  // Skip debug messages if log level is too low
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string logMessage = timestamp + " [DEBUG] " + message;
    
    if (logFile.is_open()) {
        logFile << logMessage << std::endl;
    }
    
    if (console) {
        std::cout << logMessage << std::endl;
    }
}

void ChessLogger::logGameState(const ChessGame& game)
{
    if (logLevel < 2) return;  // Skip detailed game state if log level is too low
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string gameId = game.getGameId();
    std::string whitePlayer = game.getWhitePlayer()->getUsername();
    std::string blackPlayer = game.getBlackPlayer()->getUsername();
    std::string currentTurn = (game.getBoard()->getCurrentTurn() == PieceColor::WHITE) ? "White" : "Black";
    std::string asciiBoard = game.getBoardAscii();
    
    std::stringstream ss;
    ss << "Game State [" << gameId << "]:\n"
       << "White: " << whitePlayer << ", Black: " << blackPlayer << "\n"
       << "Current Turn: " << currentTurn << "\n"
       << asciiBoard;
    
    if (logFile.is_open()) {
        logFile << timestamp << " [GAME] " << ss.str() << std::endl;
    }
}

void ChessLogger::logPlayerAction(const ChessPlayer& player, const std::string& action)
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string username = player.getUsername();
    
    std::stringstream ss;
    ss << "Player " << username << ": " << action;
    
    if (logFile.is_open()) {
        logFile << timestamp << " [PLAYER] " << ss.str() << std::endl;
    }
}

void ChessLogger::logServerEvent(const std::string& event)
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTimestamp();
    
    if (logFile.is_open()) {
        logFile << timestamp << " [SERVER] " << event << std::endl;
    }
}

void ChessLogger::logNetworkMessage(const std::string& direction, const QJsonObject& message) {
    if (logLevel < 3) return;  // Skip network messages if log level is too low
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTimestamp();
    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    if (logFile.is_open()) {
        logFile << timestamp << " [NETWORK] " << direction << ": " 
                << jsonString.toStdString() << std::endl;
    }
}

void ChessLogger::setLogLevel(int level)
{
    logLevel = level;
}

int ChessLogger::getLogLevel() const
{
    return logLevel;
}

void ChessLogger::flush()
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (logFile.is_open()) {
        logFile.flush();
    }
}

std::string ChessLogger::getCurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    
    return ss.str();
}

// Implementation of ChessAuthenticator class
ChessAuthenticator::ChessAuthenticator(const std::string& userDbPath) : userDbPath(userDbPath)
{
    // Create the directory if it doesn't exist
    QDir dir(QString::fromStdString(userDbPath));
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    loadPasswordDb();
}

ChessAuthenticator::~ChessAuthenticator()
{
    savePasswordDb();
}

bool ChessAuthenticator::authenticatePlayer(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(authMutex);
    
    if (passwordCache.find(username) == passwordCache.end()) {
        return false;  // User not found
    }
    
    std::string storedHash = passwordCache[username];
    std::string salt = storedHash.substr(0, 16);  // First 16 characters are the salt
    std::string hash = hashPassword(password, salt);
    
    return hash == storedHash;
}

bool ChessAuthenticator::registerPlayer(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(authMutex);
    
    if (passwordCache.find(username) != passwordCache.end()) {
        return false;  // User already exists
    }
    
    std::string salt = generateSalt();
    std::string hash = hashPassword(password, salt);
    passwordCache[username] = hash;
    
    // Create a new player
    ChessPlayer player(username);
    
    // Save the player data
    if (!savePlayer(player)) {
        passwordCache.erase(username);
        return false;
    }
    
    savePasswordDb();
    return true;
}

bool ChessAuthenticator::usernameExists(const std::string& username) {
    std::lock_guard<std::mutex> lock(authMutex);
    return passwordCache.find(username) != passwordCache.end();
}

std::unique_ptr<ChessPlayer> ChessAuthenticator::getPlayer(const std::string& username) {
    std::string playerFilePath = getPlayerFilePath(username);
    
    QFile file(QString::fromStdString(playerFilePath));
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        return nullptr;
    }
    
    ChessPlayer player = ChessPlayer::fromJson(doc.object());
    return std::make_unique<ChessPlayer>(player);
}

bool ChessAuthenticator::savePlayer(const ChessPlayer& player) {
    std::string playerFilePath = getPlayerFilePath(player.getUsername());
    
    QJsonObject json = player.toJson();
    QJsonDocument doc(json);
    
    QFile file(QString::fromStdString(playerFilePath));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

std::vector<std::string> ChessAuthenticator::getAllPlayerUsernames() {
    std::lock_guard<std::mutex> lock(authMutex);
    
    std::vector<std::string> usernames;
    for (const auto& pair : passwordCache) {
        usernames.push_back(pair.first);
    }
    
    return usernames;
}

bool ChessAuthenticator::deletePlayer(const std::string& username) {
    std::lock_guard<std::mutex> lock(authMutex);
    
    if (passwordCache.find(username) == passwordCache.end()) {
        return false;  // User not found
    }
    
    // Remove from password cache
    passwordCache.erase(username);
    savePasswordDb();
    
    // Delete player file
    std::string playerFilePath = getPlayerFilePath(username);
    QFile file(QString::fromStdString(playerFilePath));
    if (file.exists()) {
        return file.remove();
    }
    
    return true;
}

std::string ChessAuthenticator::hashPassword(const std::string& password, const std::string& salt) {
    std::string saltedPassword = salt + password;
    QByteArray hash = QCryptographicHash::hash(
        QByteArray::fromStdString(saltedPassword),
        QCryptographicHash::Sha256
    );
    
    return salt + QString(hash.toHex()).toStdString();
}

std::string ChessAuthenticator::generateSalt(int length) {
    const std::string chars = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);
    
    std::string salt;
    salt.reserve(length);
    
    for (int i = 0; i < length; ++i) {
        salt += chars[distribution(generator)];
    }
    
    return salt;
}

void ChessAuthenticator::loadPasswordDb() {
    std::string passwordDbPath = userDbPath + "/passwords.json";
    
    QFile file(QString::fromStdString(passwordDbPath));
    if (!file.open(QIODevice::ReadOnly)) {
        return;  // File doesn't exist yet
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        return;
    }
    
    QJsonObject json = doc.object();
    for (auto it = json.begin(); it != json.end(); ++it) {
        std::string username = it.key().toStdString();
        std::string hash = it.value().toString().toStdString();
        passwordCache[username] = hash;
    }
}

void ChessAuthenticator::savePasswordDb() {
    std::string passwordDbPath = userDbPath + "/passwords.json";
    
    QJsonObject json;
    for (const auto& pair : passwordCache) {
        json[QString::fromStdString(pair.first)] = QString::fromStdString(pair.second);
    }
    
    QJsonDocument doc(json);
    
    QFile file(QString::fromStdString(passwordDbPath));
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    
    file.write(doc.toJson());
}

std::string ChessAuthenticator::getPlayerFilePath(const std::string& username) {
    return userDbPath + "/player_" + username + ".json";
}

MPChessServer* MPChessServer::mpChessServerInstance = nullptr;

// Implementation of MPChessServer class
MPChessServer::MPChessServer(QObject* parent, const std::string& stockfishPath) : QObject(parent), server(nullptr),
    totalGamesPlayed(0), totalPlayersRegistered(0), peakConcurrentPlayers(0), totalMovesPlayed(0)
{    
    // Initialize directories
    initializeServerDirectories();

    // Set the singleton instance
    mpChessServerInstance = this;
    
    // Initialize logger
    logger = std::make_unique<ChessLogger>(getLogsPath() + "/server.log");
    logger->setLogLevel(3);  // Increase log level for more detailed logging

    // Initialize thread pool
    threadPool = new QThreadPool(this);
    threadPool->setMaxThreadCount(4); // Adjust based on your server's capabilities
    logger->log("Thread pool initialized with " + std::to_string(threadPool->maxThreadCount()) + " threads");

    // Initialize performance timer
    performanceTimer = new QTimer(this);
    connect(performanceTimer, &QTimer::timeout, this, &MPChessServer::logPerformanceStats);
    performanceTimer->start(300000); // Log every 5 minutes

    // Initialize authenticator
    authenticator = std::make_unique<ChessAuthenticator>(getPlayerDataPath());
    
    // Initialize leaderboard
    leaderboard = std::make_unique<ChessLeaderboard>(getPlayerDataPath());
    
    // Initialize matchmaker
    matchmaker = std::make_unique<ChessMatchmaker>();
    
    // Initialize rating system
    ratingSystem = std::make_unique<ChessRatingSystem>();
    
    // Initialize analysis engine
    analysisEngine = std::make_unique<ChessAnalysisEngine>();
    
    // Initialize serializer
    serializer = std::make_unique<ChessSerializer>();
    
    // Initialize Stockfish connector if path is provided
    if (!stockfishPath.empty()) {
        stockfishConnector = std::make_unique<StockfishConnector>(stockfishPath);
        if (stockfishConnector->initialize()) {
            logger->log("StockfishConnector initialized with engine at: " + stockfishPath, true);
        } else {
            logger->error("Failed to initialize StockfishConnector with engine at: " + stockfishPath, true);
            stockfishConnector.reset();
        }
    }
    
    // Initialize timers
    matchmakingTimer = new QTimer(this);
    connect(matchmakingTimer, &QTimer::timeout, this, &MPChessServer::handleMatchmakingTimer);
    
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &MPChessServer::handleGameTimerUpdate);
    
    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MPChessServer::handleServerStatusUpdate);
    
    leaderboardTimer = new QTimer(this);
    connect(leaderboardTimer, &QTimer::timeout, this, &MPChessServer::handleLeaderboardRefresh);
    
    logger->log("MPChessServer initialized");
}

MPChessServer::~MPChessServer()
{
    logger->log("MPChessServer shutting down");
    
    stop();

    // Wait for thread pool to finish
    threadPool->waitForDone();

    delete matchmakingTimer;
    delete gameTimer;
    delete statusTimer;
    delete leaderboardTimer;
    delete performanceTimer;
    
    // Log final performance stats
    logPerformanceStats();
    
    // Clear the singleton instance if this is the current instance
    if (mpChessServerInstance == this) {
        mpChessServerInstance = nullptr;
    }
    
    logger->log("MPChessServer destroyed");
}

void MPChessServer::logPerformanceStats()
{
    if (logger && logger->getLogLevel() >= 2) {
        std::string stats = PerformanceMonitor::getStatsSummary();
        logger->log("Performance Statistics:\n" + stats);
        
        // Reset stats after logging if desired
        // PerformanceMonitor::resetStats();
    }
}

bool MPChessServer::isPlayerInGame(ChessPlayer* player) const
{
    if (!player) return false;
    return playerToGameId.contains(player);
}

bool MPChessServer::start(int port)
{
    if (server) {
        logger->warning("Server already running");
        return false;
    }
    
    server = new QTcpServer(this);
    
    connect(server, &QTcpServer::newConnection, this, &MPChessServer::handleNewConnection);
    
    if (!server->listen(QHostAddress::Any, port)) {
        logger->error("Failed to start server: " + server->errorString().toStdString());
        delete server;
        server = nullptr;
        return false;
    }
    
    startTime = QDateTime::currentDateTime();
    
    // Start timers
    matchmakingTimer->start(1000);  // Check matchmaking every second
    gameTimer->start(100);          // Update game timers every 100ms
    statusTimer->start(60000);      // Update server status every minute
    leaderboardTimer->start(600000); // Refresh leaderboard every 10 minutes
    
    logger->log("Server started on port " + std::to_string(port), true);
    return true;
}

void MPChessServer::stop()
{
    if (!server) {
        return;
    }
    
    // Stop timers
    matchmakingTimer->stop();
    gameTimer->stop();
    statusTimer->stop();
    leaderboardTimer->stop();
    
    // Disconnect all clients
    for (auto it = socketToPlayer.begin(); it != socketToPlayer.end(); ++it) {
        QTcpSocket* socket = it.key();
        if (socket) {
            socket->disconnectFromHost();
        }
    }
    
    // Clear all games
    activeGames.clear();
    
    // Clear all players
    socketToPlayer.clear();
    usernamesToPlayers.clear();
    playerToGameId.clear();
    
    // Stop the server
    server->close();
    delete server;
    server = nullptr;
    
    logger->log("Server stopped", true);
}

void MPChessServer::setLogLevel(int level)
{
    if (logger) {
        logger->setLogLevel(level);
        logger->log("Log level set to " + std::to_string(level), true);
    }
}

bool MPChessServer::isRunning() const
{
    return server != nullptr && server->isListening();
}

int MPChessServer::getPort() const
{
    return server ? server->serverPort() : -1;
}

int MPChessServer::getConnectedClientCount() const
{
    return socketToPlayer.size();
}

int MPChessServer::getActiveGameCount() const
{
    return activeGames.size();
}

qint64 MPChessServer::getUptime() const
{
    return startTime.secsTo(QDateTime::currentDateTime());
}

QJsonObject MPChessServer::getServerStats() const
{
    QJsonObject stats;
    
    stats["uptime"] = getUptime();
    stats["connectedClients"] = getConnectedClientCount();
    stats["activeGames"] = getActiveGameCount();
    stats["totalGamesPlayed"] = totalGamesPlayed;
    stats["totalPlayersRegistered"] = totalPlayersRegistered;
    stats["peakConcurrentPlayers"] = peakConcurrentPlayers;
    stats["totalMovesPlayed"] = totalMovesPlayed;
    stats["playersInMatchmaking"] = matchmaker->getQueueSize();
    
    return stats;
}

QString MPChessServer::getBoardOrientationForPlayer(ChessPlayer* player, const std::string& gameId) const
{
    if (!player) {
        logger->error("getBoardOrientationForPlayer() - Null player");
        return "standard";
    }
    
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("getBoardOrientationForPlayer() - Game not found: " + gameId);
        return "standard";
    }
    
    ChessGame* game = it->second.get();
    if (!game) {
        logger->error("getBoardOrientationForPlayer() - Null game object for ID: " + gameId);
        return "standard";
    }
    
    // White player sees the board in standard orientation (white at bottom)
    // Black player sees the board flipped (black at bottom)
    if (player == game->getWhitePlayer()) {
        return "standard";
    } else if (player == game->getBlackPlayer()) {
        return "flipped";
    } else {
        logger->warning("getBoardOrientationForPlayer() - Player " + player->getUsername() + 
                       " is not part of game " + gameId);
        return "standard";
    }
}

void MPChessServer::sendGameStateToPlayers(const std::string& gameId)
{
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("sendGameStateToPlayers() - Game not found: " + gameId);
        return;
    }
    
    ChessGame* game = it->second.get();
    if (!game) {
        logger->error("sendGameStateToPlayers() - Null game object for ID: " + gameId);
        return;
    }
    
    ChessPlayer* whitePlayer = game->getWhitePlayer();
    ChessPlayer* blackPlayer = game->getBlackPlayer();
    
    if (!whitePlayer || !blackPlayer) {
        logger->error("sendGameStateToPlayers() - Null player(s) in game " + gameId);
        return;
    }
    
    try {
        logger->debug("sendGameStateToPlayers() - Preparing game state messages for game " + gameId);
        
        // Get the base game state
        QJsonObject baseGameState = game->getGameStateJson();
        
        // Create white player's game state message
        QJsonObject whiteGameStateMessage;
        whiteGameStateMessage["type"] = static_cast<int>(MessageType::GAME_STATE);
        whiteGameStateMessage["gameState"] = baseGameState;
        whiteGameStateMessage["gameState"].toObject()["boardOrientation"] = "standard";
        
        // Create black player's game state message
        QJsonObject blackGameStateMessage;
        blackGameStateMessage["type"] = static_cast<int>(MessageType::GAME_STATE);
        blackGameStateMessage["gameState"] = baseGameState;
        blackGameStateMessage["gameState"].toObject()["boardOrientation"] = "flipped";
        
        if (whitePlayer->getSocket()) {
            logger->debug("sendGameStateToPlayers() - Sending game state to white player: " + whitePlayer->getUsername());
            sendMessage(whitePlayer->getSocket(), whiteGameStateMessage);
        } else {
            logger->warning("sendGameStateToPlayers() - White player has no socket: " + whitePlayer->getUsername());
        }
        
        if (blackPlayer->getSocket()) {
            logger->debug("sendGameStateToPlayers() - Sending game state to black player: " + blackPlayer->getUsername());
            sendMessage(blackPlayer->getSocket(), blackGameStateMessage);
        } else {
            logger->warning("sendGameStateToPlayers() - Black player has no socket: " + blackPlayer->getUsername());
        }
        
        logger->debug("sendGameStateToPlayers() - Game state sent successfully for game " + gameId);
    } catch (const std::exception& e) {
        logger->error("sendGameStateToPlayers() - Exception: " + std::string(e.what()));
    } catch (...) {
        logger->error("sendGameStateToPlayers() - Unknown exception");
    }
}

void MPChessServer::generateMoveRecommendationsAsync(const std::string& gameId, ChessPlayer* player)
{
    if (!player) {
        logger->error("generateMoveRecommendationsAsync() - Null player");
        return;
    }
    
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("generateMoveRecommendationsAsync() - Game not found: " + gameId);
        return;
    }
    
    ChessGame* game = it->second.get();
    if (!game) {
        logger->error("generateMoveRecommendationsAsync() - Null game object for ID: " + gameId);
        return;
    }
    
    // Check if it's the player's turn
    if (game->getCurrentPlayer() != player) {
        logger->debug("generateMoveRecommendationsAsync() - Not player's turn, skipping recommendations");
        return;
    }
    
    logger->debug("generateMoveRecommendationsAsync() - Starting async recommendation generation for game " + gameId);
    
    // Create a task for recommendation generation
    MoveRecommendationTask* task = new MoveRecommendationTask(
        *game->getBoard(), player->getColor(), 5, analysisEngine.get());
    
    // Connect to the task's signal
    connect(task, &MoveRecommendationTask::recommendationsReady, 
            this, [this, gameId, player](const std::vector<std::pair<ChessMove, double>>& recommendations) {
        
        logger->debug("Async recommendations ready for game " + gameId);
        
        // Check if the player is still connected and in the same game
        if (!player->getSocket() || !playerToGameId.contains(player) || playerToGameId[player] != gameId) {
            logger->debug("Player disconnected or changed games, discarding recommendations");
            return;
        }
        
        // Send recommendations to the player
        QJsonObject recommendationsMessage;
        recommendationsMessage["type"] = static_cast<int>(MessageType::MOVE_RECOMMENDATIONS);
        
        QJsonArray recommendationsArray;
        for (const auto& [move, evaluation] : recommendations) {
            QJsonObject recObj;
            recObj["move"] = QString::fromStdString(move.toAlgebraic());
            recObj["evaluation"] = evaluation;
            
            // Get the standard notation if possible
            auto gameIt = activeGames.find(gameId);
            if (gameIt != activeGames.end() && gameIt->second) {
                recObj["standardNotation"] = QString::fromStdString(
                    move.toStandardNotation(*gameIt->second->getBoard()));
            } else {
                recObj["standardNotation"] = QString::fromStdString(move.toAlgebraic());
            }
            
            recommendationsArray.append(recObj);
        }
        
        recommendationsMessage["recommendations"] = recommendationsArray;
        sendMessage(player->getSocket(), recommendationsMessage);
        
        // Remove the task from the map
        recommendationTasks.remove(gameId);
    });
    
    // Store the task in the map
    recommendationTasks[gameId] = task;
    
    // Start the task
    threadPool->start(task);
}

void MPChessServer::handleNewConnection()
{
    QTcpSocket* socket = server->nextPendingConnection();
    
    if (!socket) {
        logger->error("Invalid socket from new connection");
        return;
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &MPChessServer::handleClientData);
    connect(socket, &QTcpSocket::disconnected, this, &MPChessServer::handleClientDisconnected);
    
    logger->log("New client connected: " + socket->peerAddress().toString().toStdString());
}

void MPChessServer::handleClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        logger->error("Invalid socket in disconnect handler");
        return;
    }
    
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (player) {
        logger->log("Player disconnected: " + player->getUsername());
        
        // Clean up resources
        cleanupDisconnectedPlayer(player);
    } else {
        logger->log("Unknown client disconnected: " + socket->peerAddress().toString().toStdString());
    }
    
    // Remove the socket from the map
    socketToPlayer.remove(socket);
    
    // Delete the socket
    socket->deleteLater();
}

void MPChessServer::handleClientData()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        logger->error("Invalid socket in data handler");
        return;
    }
    
    // Read all available data
    QByteArray data = socket->readAll();
    
    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        logger->error("Invalid JSON received from client: " + data.toStdString());
        return;
    }
    
    QJsonObject message = doc.object();
    
    // Log the message
    logger->logNetworkMessage("RECEIVED", message);
    
    // Process the message
    processClientMessage(socket, message);
}

void MPChessServer::handleMatchmakingTimer()
{
    logger->debug("Matchmaking timer triggered - checking for matches and timeouts");
    
    try {
        // Check for timed out players and match them with bots
        std::vector<ChessPlayer*> timedOutPlayers = matchmaker->checkTimeouts(60); // 60 seconds timeout
        for (ChessPlayer* player : timedOutPlayers) {
            if (!player) {
                logger->error("Null player in timed out players list");
                continue;
            }
            
            // Skip if player is already in a game
            if (isPlayerInGame(player)) {
                logger->debug("Player " + player->getUsername() + " is already in a game, skipping bot match");
                continue;
            }
            
            logger->log("Player timed out in matchmaking: " + player->getUsername());
            
            try {
                // Create a bot player
                ChessPlayer* botPlayer = createBotPlayer(player->getRating() / 200);  // Scale skill level based on rating
                
                if (!botPlayer) {
                    logger->error("Failed to create bot player for timed out player: " + player->getUsername());
                    continue;
                }
                
                // Create a game between the player and the bot
                try {
                    createGame(player, botPlayer, TimeControlType::RAPID);
                    
                    // Send matchmaking status to the player
                    QJsonObject message;
                    message["type"] = static_cast<int>(MessageType::MATCHMAKING_STATUS);
                    message["status"] = "matched_with_bot";
                    message["opponent"] = QString::fromStdString(botPlayer->getUsername());
                    
                    if (player->getSocket()) {
                        sendMessage(player->getSocket(), message);
                    } else {
                        logger->warning("Player has no socket: " + player->getUsername());
                    }
                } catch (const std::exception& e) {
                    logger->error("Failed to create game with bot: " + std::string(e.what()));
                }
            } catch (const std::exception& e) {
                logger->error("Exception in bot matching: " + std::string(e.what()));
            } catch (...) {
                logger->error("Unknown exception in bot matching");
            }
        }
        
        // Try to match players
        std::vector<std::pair<ChessPlayer*, ChessPlayer*>> matches;
        try {
            matches = matchmaker->matchPlayers();
            logger->debug("Matchmaker found " + std::to_string(matches.size()) + " potential matches");
        } catch (const std::exception& e) {
            logger->error("Exception in matchPlayers: " + std::string(e.what()));
            return;
        }
        
        for (const auto& match : matches) {
            ChessPlayer* player1 = match.first;
            ChessPlayer* player2 = match.second;
            
            if (!player1 || !player2) {
                logger->error("Null player in match");
                continue;
            }
            
            // Double-check that neither player is already in a game
            if (isPlayerInGame(player1) || isPlayerInGame(player2)) {
                logger->warning("Skipping match because at least one player is already in a game");
                continue;
            }
            
            logger->log("Matched players: " + player1->getUsername() + " vs " + player2->getUsername());
            
            try {
                // Create a game between the players
                std::string gameId = createGame(player1, player2, TimeControlType::RAPID);
                logger->debug("Created game with ID: " + gameId);
                
                // Send matchmaking status to both players
                QJsonObject message1;
                message1["type"] = static_cast<int>(MessageType::MATCHMAKING_STATUS);
                message1["status"] = "matched";
                message1["opponent"] = QString::fromStdString(player2->getUsername());
                message1["gameId"] = QString::fromStdString(gameId);
                
                QJsonObject message2;
                message2["type"] = static_cast<int>(MessageType::MATCHMAKING_STATUS);
                message2["status"] = "matched";
                message2["opponent"] = QString::fromStdString(player1->getUsername());
                message2["gameId"] = QString::fromStdString(gameId);
                
                if (player1->getSocket()) {
                    sendMessage(player1->getSocket(), message1);
                } else {
                    logger->warning("Player1 has no socket: " + player1->getUsername());
                }
                
                if (player2->getSocket()) {
                    sendMessage(player2->getSocket(), message2);
                } else {
                    logger->warning("Player2 has no socket: " + player2->getUsername());
                }
            } catch (const std::exception& e) {
                logger->error("Exception in creating game: " + std::string(e.what()));
            } catch (...) {
                logger->error("Unknown exception in creating game");
            }
        }
    } catch (const std::exception& e) {
        logger->error("Exception in handleMatchmakingTimer: " + std::string(e.what()));
    } catch (...) {
        logger->error("Unknown exception in handleMatchmakingTimer");
    }
}

void MPChessServer::handleGameTimerUpdate()
{
    // Update timers for all active games
    for (auto it = activeGames.begin(); it != activeGames.end(); ++it) {
        ChessGame* game = it->second.get();
        if (!game->isOver()) {
            game->updateTimers();
            
            // Check if a player has timed out
            ChessPlayer* whitePlayer = game->getWhitePlayer();
            ChessPlayer* blackPlayer = game->getBlackPlayer();
            
            if (game->hasPlayerTimedOut(whitePlayer)) {
                logger->log("White player timed out: " + whitePlayer->getUsername());
                game->end(GameResult::BLACK_WIN);
                
                // Send game over message to both players
                QJsonObject message;
                message["type"] = static_cast<int>(MessageType::GAME_OVER);
                message["result"] = "black_win";
                message["reason"] = "timeout";
                
                if (whitePlayer->getSocket()) {
                    sendMessage(whitePlayer->getSocket(), message);
                }
                
                if (blackPlayer->getSocket()) {
                    sendMessage(blackPlayer->getSocket(), message);
                }
                
                // Update ratings
                updatePlayerRatings(game);
                
                // Save game history
                saveGameHistory(*game);
            } else if (game->hasPlayerTimedOut(blackPlayer)) {
                logger->log("Black player timed out: " + blackPlayer->getUsername());
                game->end(GameResult::WHITE_WIN);
                
                // Send game over message to both players
                QJsonObject message;
                message["type"] = static_cast<int>(MessageType::GAME_OVER);
                message["result"] = "white_win";
                message["reason"] = "timeout";
                
                if (whitePlayer->getSocket()) {
                    sendMessage(whitePlayer->getSocket(), message);
                }
                
                if (blackPlayer->getSocket()) {
                    sendMessage(blackPlayer->getSocket(), message);
                }
                
                // Update ratings
                updatePlayerRatings(game);
                
                // Save game history
                saveGameHistory(*game);
            }
        }
    }
}

void MPChessServer::handleServerStatusUpdate() {
    // Log server status
    QJsonObject stats = getServerStats();
    
    std::stringstream ss;
    ss << "Server Status: "
       << "Uptime: " << stats["uptime"].toInt() << "s, "
       << "Connected Clients: " << stats["connectedClients"].toInt() << ", "
       << "Active Games: " << stats["activeGames"].toInt() << ", "
       << "Total Games: " << stats["totalGamesPlayed"].toInt();
    
    logger->log(ss.str());
}

void MPChessServer::processClientMessage(QTcpSocket* socket, const QJsonObject& message) {
    if (!message.contains("type")) {
        logger->error("Message missing type field");
        return;
    }
    
    MessageType type = static_cast<MessageType>(message["type"].toInt());
    
    switch (type) {
        case MessageType::AUTHENTICATION:
            processAuthRequest(socket, message);
            break;
            
        case MessageType::MOVE:
            processMoveRequest(socket, message);
            break;
            
        case MessageType::MATCHMAKING_REQUEST:
            processMatchmakingRequest(socket, message);
            break;
            
        case MessageType::GAME_HISTORY_REQUEST:
            processGameHistoryRequest(socket, message);
            break;
            
        case MessageType::GAME_ANALYSIS_REQUEST:
            processGameAnalysisRequest(socket, message);
            break;
            
        case MessageType::RESIGN:
            processResignRequest(socket, message);
            break;
            
        case MessageType::DRAW_OFFER:
            processDrawOfferRequest(socket, message);
            break;
            
        case MessageType::DRAW_RESPONSE:
            processDrawResponseRequest(socket, message);
            break;

        case MessageType::LEADERBOARD_REQUEST:
            processLeaderboardRequest(socket, message);
            break;

        case MessageType::PING:
            // Respond with a pong
            {
                QJsonObject response;
                response["type"] = static_cast<int>(MessageType::PONG);
                sendMessage(socket, response);
            }
            break;
            
        default:
            logger->warning("Unknown message type: " + std::to_string(static_cast<int>(type)));
            break;
    }
}

void MPChessServer::handleLeaderboardRefresh() {
    logger->log("Refreshing leaderboard");
    leaderboard->refreshLeaderboard();
}

void MPChessServer::sendMessage(QTcpSocket* socket, const QJsonObject& message) {
    if (!socket) {
        logger->error("Attempted to send message to null socket");
        return;
    }
    
    // Log the message
    logger->logNetworkMessage("SENT", message);
    
    // Convert to JSON
    QJsonDocument doc(message);
    QByteArray data = doc.toJson();
    
    // Send the data
    socket->write(data);
    socket->flush();
}

/*
std::string MPChessServer::createGame(ChessPlayer* player1, ChessPlayer* player2, TimeControlType timeControl)
{
    if (!player1 || !player2) {
        logger->error("createGame() - Attempted to create game with null player(s)");
        throw std::invalid_argument("Null player(s) provided to createGame");
    }

    // Check if either player is already in a game
    if (isPlayerInGame(player1)) {
        logger->error("createGame() - Player " + player1->getUsername() + " is already in a game");
        throw std::runtime_error("Player " + player1->getUsername() + " is already in a game");
    }
    
    if (isPlayerInGame(player2)) {
        logger->error("createGame() - Player " + player2->getUsername() + " is already in a game");
        throw std::runtime_error("Player " + player2->getUsername() + " is already in a game");
    }
    
    logger->debug("createGame() - Creating game between " + player1->getUsername() + " and " + player2->getUsername());
    
    try {
        // Generate a unique game ID
        std::string gameId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        logger->debug("createGame() - Generated game ID: " + gameId);
        
        // Randomly assign colors
        bool player1IsWhite = QRandomGenerator::global()->bounded(2) == 0;
        ChessPlayer* whitePlayer = player1IsWhite ? player1 : player2;
        ChessPlayer* blackPlayer = player1IsWhite ? player2 : player1;
        
        logger->debug("createGame() - Assigned colors: " + whitePlayer->getUsername() + " (White), " + 
                     blackPlayer->getUsername() + " (Black)");
        
        // Set player colors before creating the game
        whitePlayer->setColor(PieceColor::WHITE);
        blackPlayer->setColor(PieceColor::BLACK);
        
        // Create the game with try-catch
        std::unique_ptr<ChessGame> game;
        try {
            logger->debug("createGame() - Constructing ChessGame object for game " + gameId);
            game = std::make_unique<ChessGame>(whitePlayer, blackPlayer, gameId, timeControl);
            logger->debug("createGame() - ChessGame object created successfully for game " + gameId);
        } catch (const std::exception& e) {
            logger->error("createGame() - Exception creating ChessGame for game " + gameId + ": " + std::string(e.what()));
            throw;
        } catch (...) {
            logger->error("createGame() - Unknown exception creating ChessGame for game " + gameId);
            throw std::runtime_error("Unknown error creating game");
        }
        
        // Start the game with try-catch
        try {
            logger->debug("createGame() - Starting game " + gameId);
            game->start();
            logger->debug("createGame() - Game " + gameId + " started successfully");
        } catch (const std::exception& e) {
            logger->error("createGame() - Exception starting game " + gameId + ": " + std::string(e.what()));
            throw;
        } catch (...) {
            logger->error("createGame() - Unknown exception starting game " + gameId);
            throw std::runtime_error("Unknown error starting game");
        }
        
        // Store the game
        logger->debug("createGame() - Storing game " + gameId + " in active games map");
        activeGames[gameId] = std::move(game);
        playerToGameId[whitePlayer] = gameId;
        playerToGameId[blackPlayer] = gameId;
        
        // Send game start message to both players
        logger->debug("createGame() - Preparing game start messages for game " + gameId);
        QJsonObject message;
        message["type"] = static_cast<int>(MessageType::GAME_START);
        message["gameId"] = QString::fromStdString(gameId);
        message["whitePlayer"] = QString::fromStdString(whitePlayer->getUsername());
        message["blackPlayer"] = QString::fromStdString(blackPlayer->getUsername());
        message["timeControl"] = [timeControl]() -> QString {
            switch (timeControl) {
                case TimeControlType::RAPID: return "rapid";
                case TimeControlType::BLITZ: return "blitz";
                case TimeControlType::BULLET: return "bullet";
                case TimeControlType::CLASSICAL: return "classical";
                case TimeControlType::CASUAL: return "casual";
                default: return "rapid";
            }
        }();
        
        if (whitePlayer->getSocket()) {
            logger->debug("createGame() - Sending game start message to white player: " + whitePlayer->getUsername());
            message["yourColor"] = "white";
            sendMessage(whitePlayer->getSocket(), message);
        } else {
            logger->warning("createGame() - White player has no socket: " + whitePlayer->getUsername());
        }
        
        if (blackPlayer->getSocket()) {
            logger->debug("createGame() - Sending game start message to black player: " + blackPlayer->getUsername());
            message["yourColor"] = "black";
            sendMessage(blackPlayer->getSocket(), message);
        } else {
            logger->warning("createGame() - Black player has no socket: " + blackPlayer->getUsername());
        }
        
        // Send initial game state
        try {
            logger->debug("createGame() - Preparing initial game state message for game " + gameId);
            QJsonObject gameStateMessage;
            gameStateMessage["type"] = static_cast<int>(MessageType::GAME_STATE);
            
            logger->debug("createGame() - Getting game state JSON for game " + gameId);
            gameStateMessage["gameState"] = activeGames[gameId]->getGameStateJson();
            logger->debug("createGame() - Successfully got game state JSON for game " + gameId);
            
            if (whitePlayer->getSocket()) {
                logger->debug("createGame() - Sending game state to white player: " + whitePlayer->getUsername());
                sendMessage(whitePlayer->getSocket(), gameStateMessage);
            }
            
            if (blackPlayer->getSocket()) {
                logger->debug("createGame() - Sending game state to black player: " + blackPlayer->getUsername());
                sendMessage(blackPlayer->getSocket(), gameStateMessage);
            }
        } catch (const std::exception& e) {
            logger->error("createGame() - Exception sending game state for game " + gameId + ": " + std::string(e.what()));
            // Continue despite error in sending game state
        }
        
        // Send move recommendations to white player (first to move) asynchronously
        if (whitePlayer->getSocket()) {
            try {
                logger->debug("createGame() - Scheduling async move recommendations for white player in game " + gameId);
                generateMoveRecommendationsAsync(gameId, whitePlayer);
            } catch (const std::exception& e) {
                logger->error("createGame() - Exception scheduling move recommendations for game " + gameId + ": " + std::string(e.what()));
                // Continue despite error in recommendations
            }
        }
/ *
        // Send move recommendations to white player (first to move) - SIMPLIFIED VERSION
        if (whitePlayer->getSocket()) {
            try {
                logger->debug("createGame() - Generating simplified move recommendations for white player in game " + gameId);
                
                // Get valid moves directly from the board
                std::vector<ChessMove> validMoves = activeGames[gameId]->getBoard()->getAllValidMoves(PieceColor::WHITE);
                
                // Take just the first few moves without deep evaluation
                std::vector<std::pair<ChessMove, double>> recommendations;
                int count = 0;
                for (const auto& move : validMoves) {
                    recommendations.emplace_back(move, 0.0);
                    count++;
                    if (count >= 5) break;
                }
                
                QJsonObject recommendationsMessage;
                recommendationsMessage["type"] = static_cast<int>(MessageType::MOVE_RECOMMENDATIONS);
                
                QJsonArray recommendationsArray;
                for (const auto& [move, evaluation] : recommendations) {
                    QJsonObject recObj;
                    recObj["move"] = QString::fromStdString(move.toAlgebraic());
                    recObj["evaluation"] = evaluation;
                    recObj["standardNotation"] = QString::fromStdString(
                        move.toStandardNotation(*activeGames[gameId]->getBoard()));
                    recommendationsArray.append(recObj);
                }
                
                recommendationsMessage["recommendations"] = recommendationsArray;
                logger->debug("createGame() - Sending move recommendations to white player: " + whitePlayer->getUsername());
                sendMessage(whitePlayer->getSocket(), recommendationsMessage);
            } catch (const std::exception& e) {
                logger->error("createGame() - Exception generating move recommendations for game " + gameId + ": " + std::string(e.what()));
                // Continue despite error in recommendations
            }
        }
// * /
        
        // Log the game creation
        logger->log("Created game " + gameId + ": " + whitePlayer->getUsername() + 
                   " (White) vs " + blackPlayer->getUsername() + " (Black)");
        
        // Increment total games played
        totalGamesPlayed++;

        // Make sure to remove players from matchmaking queue if they're still there
        matchmaker->removePlayer(player1);
        matchmaker->removePlayer(player2);
        
        return gameId;
    } catch (const std::exception& e) {
        logger->error("createGame() - Exception in createGame: " + std::string(e.what()));
        throw;
    } catch (...) {
        logger->error("createGame() - Unknown exception in createGame");
        throw std::runtime_error("Unknown error creating game");
    }
}
*/

/*
std::string MPChessServer::createGame(ChessPlayer* player1, ChessPlayer* player2, TimeControlType timeControl)
{
    if (!player1 || !player2) {
        logger->error("createGame() - Attempted to create game with null player(s)");
        throw std::invalid_argument("Null player(s) provided to createGame");
    }

    // Check if either player is already in a game
    if (isPlayerInGame(player1)) {
        logger->error("createGame() - Player " + player1->getUsername() + " is already in a game");
        throw std::runtime_error("Player " + player1->getUsername() + " is already in a game");
    }
    
    if (isPlayerInGame(player2)) {
        logger->error("createGame() - Player " + player2->getUsername() + " is already in a game");
        throw std::runtime_error("Player " + player2->getUsername() + " is already in a game");
    }
    
    logger->debug("createGame() - Creating game between " + player1->getUsername() + " and " + player2->getUsername());
    
    try {
        // Generate a unique game ID
        std::string gameId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        logger->debug("createGame() - Generated game ID: " + gameId);
        
        // Randomly assign colors
        bool player1IsWhite = QRandomGenerator::global()->bounded(2) == 0;
        ChessPlayer* whitePlayer = player1IsWhite ? player1 : player2;
        ChessPlayer* blackPlayer = player1IsWhite ? player2 : player1;
        
        logger->debug("createGame() - Assigned colors: " + whitePlayer->getUsername() + " (White), " + 
                     blackPlayer->getUsername() + " (Black)");
        
        // Set player colors before creating the game
        whitePlayer->setColor(PieceColor::WHITE);
        blackPlayer->setColor(PieceColor::BLACK);
        
        // Create the game with try-catch
        std::unique_ptr<ChessGame> game;
        try {
            logger->debug("createGame() - Constructing ChessGame object for game " + gameId);
            game = std::make_unique<ChessGame>(whitePlayer, blackPlayer, gameId, timeControl);
            logger->debug("createGame() - ChessGame object created successfully for game " + gameId);
        } catch (const std::exception& e) {
            logger->error("createGame() - Exception creating ChessGame for game " + gameId + ": " + std::string(e.what()));
            throw;
        } catch (...) {
            logger->error("createGame() - Unknown exception creating ChessGame for game " + gameId);
            throw std::runtime_error("Unknown error creating game");
        }
        
        // Start the game with try-catch
        try {
            logger->debug("createGame() - Starting game " + gameId);
            game->start();
            logger->debug("createGame() - Game " + gameId + " started successfully");
        } catch (const std::exception& e) {
            logger->error("createGame() - Exception starting game " + gameId + ": " + std::string(e.what()));
            throw;
        } catch (...) {
            logger->error("createGame() - Unknown exception starting game " + gameId);
            throw std::runtime_error("Unknown error starting game");
        }
        
        // Store the game
        logger->debug("createGame() - Storing game " + gameId + " in active games map");
        activeGames[gameId] = std::move(game);
        playerToGameId[whitePlayer] = gameId;
        playerToGameId[blackPlayer] = gameId;
        
        // Send game start message to both players
        logger->debug("createGame() - Preparing game start messages for game " + gameId);
        
        // White player message
        QJsonObject whiteMessage;
        whiteMessage["type"] = static_cast<int>(MessageType::GAME_START);
        whiteMessage["gameId"] = QString::fromStdString(gameId);
        whiteMessage["whitePlayer"] = QString::fromStdString(whitePlayer->getUsername());
        whiteMessage["blackPlayer"] = QString::fromStdString(blackPlayer->getUsername());
        whiteMessage["yourColor"] = "white";
        whiteMessage["boardOrientation"] = "standard"; // White pieces at bottom
        whiteMessage["timeControl"] = [timeControl]() -> QString {
            switch (timeControl) {
                case TimeControlType::RAPID: return "rapid";
                case TimeControlType::BLITZ: return "blitz";
                case TimeControlType::BULLET: return "bullet";
                case TimeControlType::CLASSICAL: return "classical";
                case TimeControlType::CASUAL: return "casual";
                default: return "rapid";
            }
        }();
        
        // Black player message
        QJsonObject blackMessage;
        blackMessage["type"] = static_cast<int>(MessageType::GAME_START);
        blackMessage["gameId"] = QString::fromStdString(gameId);
        blackMessage["whitePlayer"] = QString::fromStdString(whitePlayer->getUsername());
        blackMessage["blackPlayer"] = QString::fromStdString(blackPlayer->getUsername());
        blackMessage["yourColor"] = "black";
        blackMessage["boardOrientation"] = "flipped"; // Black pieces at bottom
        blackMessage["timeControl"] = whiteMessage["timeControl"];
        
        if (whitePlayer->getSocket()) {
            logger->debug("createGame() - Sending game start message to white player: " + whitePlayer->getUsername());
            sendMessage(whitePlayer->getSocket(), whiteMessage);
        } else {
            logger->warning("createGame() - White player has no socket: " + whitePlayer->getUsername());
        }
        
        if (blackPlayer->getSocket()) {
            logger->debug("createGame() - Sending game start message to black player: " + blackPlayer->getUsername());
            sendMessage(blackPlayer->getSocket(), blackMessage);
        } else {
            logger->warning("createGame() - Black player has no socket: " + blackPlayer->getUsername());
        }
        
        // Send initial game state
        try {
            logger->debug("createGame() - Preparing initial game state message for game " + gameId);
            
            // Get the base game state
            QJsonObject baseGameState = activeGames[gameId]->getGameStateJson();
            
            // Create white player's game state
            QJsonObject whiteGameStateMessage;
            whiteGameStateMessage["type"] = static_cast<int>(MessageType::GAME_STATE);
            whiteGameStateMessage["gameState"] = baseGameState;
            whiteGameStateMessage["gameState"].toObject()["boardOrientation"] = "standard";
            
            // Create black player's game state
            QJsonObject blackGameStateMessage;
            blackGameStateMessage["type"] = static_cast<int>(MessageType::GAME_STATE);
            blackGameStateMessage["gameState"] = baseGameState;
            blackGameStateMessage["gameState"].toObject()["boardOrientation"] = "flipped";
            
            if (whitePlayer->getSocket()) {
                logger->debug("createGame() - Sending game state to white player: " + whitePlayer->getUsername());
                sendMessage(whitePlayer->getSocket(), whiteGameStateMessage);
            }
            
            if (blackPlayer->getSocket()) {
                logger->debug("createGame() - Sending game state to black player: " + blackPlayer->getUsername());
                sendMessage(blackPlayer->getSocket(), blackGameStateMessage);
            }
        } catch (const std::exception& e) {
            logger->error("createGame() - Exception sending game state for game " + gameId + ": " + std::string(e.what()));
            // Continue despite error in sending game state
        }
        
        // Send move recommendations to white player (first to move) asynchronously
        if (whitePlayer->getSocket()) {
            try {
                logger->debug("createGame() - Scheduling async move recommendations for white player in game " + gameId);
                generateMoveRecommendationsAsync(gameId, whitePlayer);
            } catch (const std::exception& e) {
                logger->error("createGame() - Exception scheduling move recommendations for game " + gameId + ": " + std::string(e.what()));
                // Continue despite error in recommendations
            }
        }
        
        // Log the game creation
        logger->log("Created game " + gameId + ": " + whitePlayer->getUsername() + 
                   " (White) vs " + blackPlayer->getUsername() + " (Black)");
        
        // Increment total games played
        totalGamesPlayed++;

        // Make sure to remove players from matchmaking queue if they're still there
        matchmaker->removePlayer(player1);
        matchmaker->removePlayer(player2);
        
        return gameId;
    } catch (const std::exception& e) {
        logger->error("createGame() - Exception in createGame: " + std::string(e.what()));
        throw;
    } catch (...) {
        logger->error("createGame() - Unknown exception in createGame");
        throw std::runtime_error("Unknown error creating game");
    }
}
*/

std::string MPChessServer::createGame(ChessPlayer* player1, ChessPlayer* player2, TimeControlType timeControl)
{
    if (!player1 || !player2) {
        logger->error("createGame() - Attempted to create game with null player(s)");
        throw std::invalid_argument("Null player(s) provided to createGame");
    }

    // Check if either player is already in a game
    if (isPlayerInGame(player1)) {
        logger->error("createGame() - Player " + player1->getUsername() + " is already in a game");
        throw std::runtime_error("Player " + player1->getUsername() + " is already in a game");
    }
    
    if (isPlayerInGame(player2)) {
        logger->error("createGame() - Player " + player2->getUsername() + " is already in a game");
        throw std::runtime_error("Player " + player2->getUsername() + " is already in a game");
    }
    
    logger->debug("createGame() - Creating game between " + player1->getUsername() + " and " + player2->getUsername());
    
    try {
        // Generate a unique game ID
        std::string gameId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        logger->debug("createGame() - Generated game ID: " + gameId);
        
        // Randomly assign colors
        bool player1IsWhite = QRandomGenerator::global()->bounded(2) == 0;
        ChessPlayer* whitePlayer = player1IsWhite ? player1 : player2;
        ChessPlayer* blackPlayer = player1IsWhite ? player2 : player1;
        
        logger->debug("createGame() - Assigned colors: " + whitePlayer->getUsername() + " (White), " + 
                     blackPlayer->getUsername() + " (Black)");
        
        // Set player colors before creating the game
        whitePlayer->setColor(PieceColor::WHITE);
        blackPlayer->setColor(PieceColor::BLACK);
        
        // Create the game with try-catch
        std::unique_ptr<ChessGame> game;
        try {
            logger->debug("createGame() - Constructing ChessGame object for game " + gameId);
            game = std::make_unique<ChessGame>(whitePlayer, blackPlayer, gameId, timeControl);
            logger->debug("createGame() - ChessGame object created successfully for game " + gameId);
        } catch (const std::exception& e) {
            logger->error("createGame() - Exception creating ChessGame for game " + gameId + ": " + std::string(e.what()));
            throw;
        } catch (...) {
            logger->error("createGame() - Unknown exception creating ChessGame for game " + gameId);
            throw std::runtime_error("Unknown error creating game");
        }
        
        // Start the game with try-catch
        try {
            logger->debug("createGame() - Starting game " + gameId);
            game->start();
            logger->debug("createGame() - Game " + gameId + " started successfully");
        } catch (const std::exception& e) {
            logger->error("createGame() - Exception starting game " + gameId + ": " + std::string(e.what()));
            throw;
        } catch (...) {
            logger->error("createGame() - Unknown exception starting game " + gameId);
            throw std::runtime_error("Unknown error starting game");
        }
        
        // Store the game
        logger->debug("createGame() - Storing game " + gameId + " in active games map");
        activeGames[gameId] = std::move(game);
        playerToGameId[whitePlayer] = gameId;
        playerToGameId[blackPlayer] = gameId;
        
        // Send game start message to both players
        logger->debug("createGame() - Preparing game start messages for game " + gameId);
        
        // White player message
        QJsonObject whiteMessage;
        whiteMessage["type"] = static_cast<int>(MessageType::GAME_START);
        whiteMessage["gameId"] = QString::fromStdString(gameId);
        whiteMessage["whitePlayer"] = QString::fromStdString(whitePlayer->getUsername());
        whiteMessage["blackPlayer"] = QString::fromStdString(blackPlayer->getUsername());
        whiteMessage["yourColor"] = "white";
        whiteMessage["boardOrientation"] = "standard"; // White pieces at bottom
        whiteMessage["timeControl"] = [timeControl]() -> QString {
            switch (timeControl) {
                case TimeControlType::RAPID: return "rapid";
                case TimeControlType::BLITZ: return "blitz";
                case TimeControlType::BULLET: return "bullet";
                case TimeControlType::CLASSICAL: return "classical";
                case TimeControlType::CASUAL: return "casual";
                default: return "rapid";
            }
        }();
        
        // Black player message
        QJsonObject blackMessage;
        blackMessage["type"] = static_cast<int>(MessageType::GAME_START);
        blackMessage["gameId"] = QString::fromStdString(gameId);
        blackMessage["whitePlayer"] = QString::fromStdString(whitePlayer->getUsername());
        blackMessage["blackPlayer"] = QString::fromStdString(blackPlayer->getUsername());
        blackMessage["yourColor"] = "black";
        blackMessage["boardOrientation"] = "flipped"; // Black pieces at bottom
        blackMessage["timeControl"] = whiteMessage["timeControl"];
        
        if (whitePlayer->getSocket()) {
            logger->debug("createGame() - Sending game start message to white player: " + whitePlayer->getUsername());
            sendMessage(whitePlayer->getSocket(), whiteMessage);
        } else {
            logger->warning("createGame() - White player has no socket: " + whitePlayer->getUsername());
        }
        
        if (blackPlayer->getSocket()) {
            logger->debug("createGame() - Sending game start message to black player: " + blackPlayer->getUsername());
            sendMessage(blackPlayer->getSocket(), blackMessage);
        } else {
            logger->warning("createGame() - Black player has no socket: " + blackPlayer->getUsername());
        }
        
        // Send initial game state to both players
        sendGameStateToPlayers(gameId);
        
        // Send move recommendations to white player (first to move) asynchronously
        if (whitePlayer->getSocket()) {
            try {
                logger->debug("createGame() - Scheduling async move recommendations for white player in game " + gameId);
                generateMoveRecommendationsAsync(gameId, whitePlayer);
            } catch (const std::exception& e) {
                logger->error("createGame() - Exception scheduling move recommendations for game " + gameId + ": " + std::string(e.what()));
                // Continue despite error in recommendations
            }
        }
        
        // Log the game creation
        logger->log("Created game " + gameId + ": " + whitePlayer->getUsername() + 
                   " (White) vs " + blackPlayer->getUsername() + " (Black)");
        
        // Increment total games played
        totalGamesPlayed++;

        // Make sure to remove players from matchmaking queue if they're still there
        matchmaker->removePlayer(player1);
        matchmaker->removePlayer(player2);
        
        return gameId;
    } catch (const std::exception& e) {
        logger->error("createGame() - Exception in createGame: " + std::string(e.what()));
        throw;
    } catch (...) {
        logger->error("createGame() - Unknown exception in createGame");
        throw std::runtime_error("Unknown error creating game");
    }
}

void MPChessServer::endGame(const std::string& gameId, GameResult result) {
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("Attempted to end non-existent game: " + gameId);
        return;
    }
    
    ChessGame* game = it->second.get();
    if (game->isOver()) {
        return;  // Game already over
    }
    
    // End the game
    game->end(result);
    
    // Get the players
    ChessPlayer* whitePlayer = game->getWhitePlayer();
    ChessPlayer* blackPlayer = game->getBlackPlayer();
    
    // Send game over message to both players
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::GAME_OVER);
    
    switch (result) {
        case GameResult::WHITE_WIN:
            message["result"] = "white_win";
            break;
        case GameResult::BLACK_WIN:
            message["result"] = "black_win";
            break;
        case GameResult::DRAW:
            message["result"] = "draw";
            break;
        default:
            message["result"] = "unknown";
            break;
    }
    
    if (whitePlayer->getSocket()) {
        sendMessage(whitePlayer->getSocket(), message);
    }
    
    if (blackPlayer->getSocket()) {
        sendMessage(blackPlayer->getSocket(), message);
    }
    
    // Update ratings
    updatePlayerRatings(game);
    
    // Save game history
    saveGameHistory(*game);
    
    // Clean up
    playerToGameId.remove(whitePlayer);
    playerToGameId.remove(blackPlayer);
    
    // Log the game end
    logger->log("Game " + gameId + " ended with result: " + message["result"].toString().toStdString());
}

void MPChessServer::processAuthRequest(QTcpSocket* socket, const QJsonObject& data) {
    std::string username = data["username"].toString().toStdString();
    std::string password = data["password"].toString().toStdString();
    bool isRegistration = data["register"].toBool();
    
    QJsonObject response;
    response["type"] = static_cast<int>(MessageType::AUTHENTICATION_RESULT);
    
    if (isRegistration) {
        // Registration
        if (authenticator->registerPlayer(username, password)) {
            response["success"] = true;
            response["message"] = "Registration successful";
            
            // Create a new player
            ChessPlayer* player = new ChessPlayer(username, socket);
            socketToPlayer[socket] = player;
            usernamesToPlayers[username] = player;
            
            // Increment total players registered
            totalPlayersRegistered++;
            
            // Update peak concurrent players
            peakConcurrentPlayers = std::max(peakConcurrentPlayers, getConnectedClientCount());
            
            logger->log("Player registered: " + username);
        } else {
            response["success"] = false;
            response["message"] = "Username already exists";
            logger->warning("Registration failed for username: " + username);
        }
    } else {
        // Authentication
        if (authenticator->authenticatePlayer(username, password)) {
            response["success"] = true;
            response["message"] = "Authentication successful";
            
            // Check if the player is already logged in
            if (usernamesToPlayers.contains(username)) {
                // Disconnect the old socket
                ChessPlayer* existingPlayer = usernamesToPlayers[username];
                QTcpSocket* oldSocket = existingPlayer->getSocket();
                
                if (oldSocket && oldSocket != socket) {
                    // Send a message to the old socket
                    QJsonObject disconnectMessage;
                    disconnectMessage["type"] = static_cast<int>(MessageType::ERROR);
                    disconnectMessage["message"] = "You have been logged in from another location";
                    sendMessage(oldSocket, disconnectMessage);
                    
                    // Disconnect the old socket
                    oldSocket->disconnectFromHost();
                }
                
                // Update the player's socket
                existingPlayer->setSocket(socket);
                socketToPlayer[socket] = existingPlayer;
                
                logger->log("Player reconnected: " + username);
            } else {
                // Load the player data
                std::unique_ptr<ChessPlayer> playerData = authenticator->getPlayer(username);
                if (playerData) {
                    ChessPlayer* player = new ChessPlayer(*playerData);
                    player->setSocket(socket);
                    socketToPlayer[socket] = player;
                    usernamesToPlayers[username] = player;
                } else {
                    // Create a new player if data not found
                    ChessPlayer* player = new ChessPlayer(username, socket);
                    socketToPlayer[socket] = player;
                    usernamesToPlayers[username] = player;
                }
                
                logger->log("Player authenticated: " + username);
            }
            
            // Update peak concurrent players
            peakConcurrentPlayers = std::max(peakConcurrentPlayers, getConnectedClientCount());
        } else {
            response["success"] = false;
            response["message"] = "Invalid username or password";
            logger->warning("Authentication failed for username: " + username);
        }
    }
    
    sendMessage(socket, response);
}

/*
void MPChessServer::processMoveRequest(QTcpSocket* socket, const QJsonObject& data)
{
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Move request from unauthenticated socket");
        return;
    }
    
    std::string gameId = data["gameId"].toString().toStdString();
    std::string moveStr = data["move"].toString().toStdString();
    
    // Find the game
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("Move request for non-existent game: " + gameId);
        
        QJsonObject response;
        response["type"] = static_cast<int>(MessageType::MOVE_RESULT);
        response["success"] = false;
        response["message"] = "Game not found";
        sendMessage(socket, response);
        return;
    }
    
    ChessGame* game = it->second.get();
    
    // Parse the move
    ChessMove move = ChessMove::fromAlgebraic(moveStr);
    
    // Process the move
    MoveValidationStatus status = game->processMove(player, move);
    
    // Prepare response
    QJsonObject response;
    response["type"] = static_cast<int>(MessageType::MOVE_RESULT);
    
    switch (status) {
        case MoveValidationStatus::VALID:
            response["success"] = true;
            
            // Increment total moves played
            totalMovesPlayed++;
            
            // Send updated game state to both players
            {
                QJsonObject gameStateMessage;
                gameStateMessage["type"] = static_cast<int>(MessageType::GAME_STATE);
                gameStateMessage["gameState"] = game->getGameStateJson();
                
                ChessPlayer* whitePlayer = game->getWhitePlayer();
                ChessPlayer* blackPlayer = game->getBlackPlayer();
                
                if (whitePlayer->getSocket()) {
                    sendMessage(whitePlayer->getSocket(), gameStateMessage);
                }
                
                if (blackPlayer->getSocket()) {
                    sendMessage(blackPlayer->getSocket(), gameStateMessage);
                }
                
/ *
                // Send move recommendations to the next player
                ChessPlayer* nextPlayer = game->getCurrentPlayer();
                if (nextPlayer && nextPlayer->getSocket() && !game->isOver()) {
                    std::vector<std::pair<ChessMove, double>> recommendations = 
                        game->getMoveRecommendations(nextPlayer);
                    
                    QJsonObject recommendationsMessage;
                    recommendationsMessage["type"] = static_cast<int>(MessageType::MOVE_RECOMMENDATIONS);
                    
                    QJsonArray recommendationsArray;
                    for (const auto& [recMove, evaluation] : recommendations) {
                        QJsonObject recObj;
                        recObj["move"] = QString::fromStdString(recMove.toAlgebraic());
                        recObj["evaluation"] = evaluation;
                        recObj["standardNotation"] = QString::fromStdString(
                            recMove.toStandardNotation(*game->getBoard()));
                        recommendationsArray.append(recObj);
                    }
                    
                    recommendationsMessage["recommendations"] = recommendationsArray;
                    sendMessage(nextPlayer->getSocket(), recommendationsMessage);
                }
* /

                // Send move recommendations to the next player asynchronously
                ChessPlayer* nextPlayer = game->getCurrentPlayer();
                if (nextPlayer && nextPlayer->getSocket() && !game->isOver()) {
                    try {
                        generateMoveRecommendationsAsync(gameId, nextPlayer);
                    } catch (const std::exception& e) {
                        logger->error("processMoveRequest() - Exception scheduling move recommendations: " + std::string(e.what()));
                    }
                }

                // If the game is over, update ratings and save history
                if (game->isOver()) {
                    updatePlayerRatings(game);
                    saveGameHistory(*game);
                }
            }
            break;
            
        case MoveValidationStatus::INVALID_PIECE:
            response["success"] = false;
            response["message"] = "No piece at the source position";
            break;
            
        case MoveValidationStatus::INVALID_DESTINATION:
            response["success"] = false;
            response["message"] = "Invalid destination";
            break;
            
        case MoveValidationStatus::INVALID_PATH:
            response["success"] = false;
            response["message"] = "Invalid move for this piece";
            break;
            
        case MoveValidationStatus::KING_IN_CHECK:
            response["success"] = false;
            response["message"] = "Move would leave your king in check";
            break;
            
        case MoveValidationStatus::WRONG_TURN:
            response["success"] = false;
            response["message"] = "It's not your turn";
            break;
            
        case MoveValidationStatus::GAME_OVER:
            response["success"] = false;
            response["message"] = "The game is already over";
            break;
            
        default:
            response["success"] = false;
            response["message"] = "Unknown error";
            break;
    }
    
    sendMessage(socket, response);
    
    // Log the move
    if (status == MoveValidationStatus::VALID) {
        logger->log("Player " + player->getUsername() + " made move " + moveStr + 
                   " in game " + gameId);
    } else {
        logger->warning("Player " + player->getUsername() + " attempted invalid move " + 
                       moveStr + " in game " + gameId + ": " + response["message"].toString().toStdString());
    }
}
*/

/*
void MPChessServer::processMoveRequest(QTcpSocket* socket, const QJsonObject& data)
{
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Move request from unauthenticated socket");
        return;
    }
    
    std::string gameId = data["gameId"].toString().toStdString();
    std::string moveStr = data["move"].toString().toStdString();
    
    // Find the game
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("Move request for non-existent game: " + gameId);
        
        QJsonObject response;
        response["type"] = static_cast<int>(MessageType::MOVE_RESULT);
        response["success"] = false;
        response["message"] = "Game not found";
        sendMessage(socket, response);
        return;
    }
    
    ChessGame* game = it->second.get();
    
    // Parse the move
    ChessMove move = ChessMove::fromAlgebraic(moveStr);
    
    // Process the move
    MoveValidationStatus status = game->processMove(player, move);
    
    // Prepare response
    QJsonObject response;
    response["type"] = static_cast<int>(MessageType::MOVE_RESULT);
    
    switch (status) {
        case MoveValidationStatus::VALID:
            response["success"] = true;
            
            // Increment total moves played
            totalMovesPlayed++;
            
            // Send updated game state to both players with proper orientation
            {
                ChessPlayer* whitePlayer = game->getWhitePlayer();
                ChessPlayer* blackPlayer = game->getBlackPlayer();
                
                // Get the base game state
                QJsonObject baseGameState = game->getGameStateJson();
                
                // Create white player's game state message
                QJsonObject whiteGameStateMessage;
                whiteGameStateMessage["type"] = static_cast<int>(MessageType::GAME_STATE);
                whiteGameStateMessage["gameState"] = baseGameState;
                whiteGameStateMessage["gameState"].toObject()["boardOrientation"] = "standard";
                
                // Create black player's game state message
                QJsonObject blackGameStateMessage;
                blackGameStateMessage["type"] = static_cast<int>(MessageType::GAME_STATE);
                blackGameStateMessage["gameState"] = baseGameState;
                blackGameStateMessage["gameState"].toObject()["boardOrientation"] = "flipped";
                
                if (whitePlayer->getSocket()) {
                    sendMessage(whitePlayer->getSocket(), whiteGameStateMessage);
                }
                
                if (blackPlayer->getSocket()) {
                    sendMessage(blackPlayer->getSocket(), blackGameStateMessage);
                }
                
                // Send move recommendations to the next player asynchronously
                ChessPlayer* nextPlayer = game->getCurrentPlayer();
                if (nextPlayer && nextPlayer->getSocket() && !game->isOver()) {
                    try {
                        generateMoveRecommendationsAsync(gameId, nextPlayer);
                    } catch (const std::exception& e) {
                        logger->error("processMoveRequest() - Exception scheduling move recommendations: " + std::string(e.what()));
                    }
                }

                // If the game is over, update ratings and save history
                if (game->isOver()) {
                    updatePlayerRatings(game);
                    saveGameHistory(*game);
                }
            }
            break;
            
        case MoveValidationStatus::INVALID_PIECE:
            response["success"] = false;
            response["message"] = "No piece at the source position";
            break;
            
        case MoveValidationStatus::INVALID_DESTINATION:
            response["success"] = false;
            response["message"] = "Invalid destination";
            break;
            
        case MoveValidationStatus::INVALID_PATH:
            response["success"] = false;
            response["message"] = "Invalid move for this piece";
            break;
            
        case MoveValidationStatus::KING_IN_CHECK:
            response["success"] = false;
            response["message"] = "Move would leave your king in check";
            break;
            
        case MoveValidationStatus::WRONG_TURN:
            response["success"] = false;
            response["message"] = "It's not your turn";
            break;
            
        case MoveValidationStatus::GAME_OVER:
            response["success"] = false;
            response["message"] = "The game is already over";
            break;
            
        default:
            response["success"] = false;
            response["message"] = "Unknown error";
            break;
    }
    
    sendMessage(socket, response);
    
    // Log the move
    if (status == MoveValidationStatus::VALID) {
        logger->log("Player " + player->getUsername() + " made move " + moveStr + 
                   " in game " + gameId);
    } else {
        logger->warning("Player " + player->getUsername() + " attempted invalid move " + 
                       moveStr + " in game " + gameId + ": " + response["message"].toString().toStdString());
    }
}
*/

void MPChessServer::processMoveRequest(QTcpSocket* socket, const QJsonObject& data)
{
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Move request from unauthenticated socket");
        return;
    }
    
    std::string gameId = data["gameId"].toString().toStdString();
    std::string moveStr = data["move"].toString().toStdString();
    
    // Find the game
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("Move request for non-existent game: " + gameId);
        
        QJsonObject response;
        response["type"] = static_cast<int>(MessageType::MOVE_RESULT);
        response["success"] = false;
        response["message"] = "Game not found";
        sendMessage(socket, response);
        return;
    }
    
    ChessGame* game = it->second.get();
    
    // Parse the move
    ChessMove move = ChessMove::fromAlgebraic(moveStr);
    
    // Process the move
    MoveValidationStatus status = game->processMove(player, move);
    
    // Prepare response
    QJsonObject response;
    response["type"] = static_cast<int>(MessageType::MOVE_RESULT);
    
    switch (status) {
        case MoveValidationStatus::VALID:
            response["success"] = true;
            
            // Increment total moves played
            totalMovesPlayed++;
            
            // Send updated game state to both players
            sendGameStateToPlayers(gameId);
            
            // Send move recommendations to the next player asynchronously
            {
                ChessPlayer* nextPlayer = game->getCurrentPlayer();
                if (nextPlayer && nextPlayer->getSocket() && !game->isOver()) {
                    try {
                        generateMoveRecommendationsAsync(gameId, nextPlayer);
                    } catch (const std::exception& e) {
                        logger->error("processMoveRequest() - Exception scheduling move recommendations: " + std::string(e.what()));
                    }
                }

                // If the game is over, update ratings and save history
                if (game->isOver()) {
                    updatePlayerRatings(game);
                    saveGameHistory(*game);
                }
            }
            break;
            
        case MoveValidationStatus::INVALID_PIECE:
            response["success"] = false;
            response["message"] = "No piece at the source position";
            break;
            
        case MoveValidationStatus::INVALID_DESTINATION:
            response["success"] = false;
            response["message"] = "Invalid destination";
            break;
            
        case MoveValidationStatus::INVALID_PATH:
            response["success"] = false;
            response["message"] = "Invalid move for this piece";
            break;
            
        case MoveValidationStatus::KING_IN_CHECK:
            response["success"] = false;
            response["message"] = "Move would leave your king in check";
            break;
            
        case MoveValidationStatus::WRONG_TURN:
            response["success"] = false;
            response["message"] = "It's not your turn";
            break;
            
        case MoveValidationStatus::GAME_OVER:
            response["success"] = false;
            response["message"] = "The game is already over";
            break;
            
        default:
            response["success"] = false;
            response["message"] = "Unknown error";
            break;
    }
    
    sendMessage(socket, response);
    
    // Log the move
    if (status == MoveValidationStatus::VALID) {
        logger->log("Player " + player->getUsername() + " made move " + moveStr + 
                   " in game " + gameId);
    } else {
        logger->warning("Player " + player->getUsername() + " attempted invalid move " + 
                       moveStr + " in game " + gameId + ": " + response["message"].toString().toStdString());
    }
}

void MPChessServer::processMatchmakingRequest(QTcpSocket* socket, const QJsonObject& data)
{
    logger->debug("Processing matchmaking request");
    
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Matchmaking request from unauthenticated socket");
        
        QJsonObject errorResponse;
        errorResponse["type"] = static_cast<int>(MessageType::ERROR);
        errorResponse["message"] = "You must be authenticated to use matchmaking";
        sendMessage(socket, errorResponse);
        return;
    }
    
    bool join = data["join"].toBool();
    logger->debug("Player " + player->getUsername() + (join ? " joining " : " leaving ") + "matchmaking queue");
    
    QJsonObject response;
    response["type"] = static_cast<int>(MessageType::MATCHMAKING_STATUS);
    
    if (join) {
        // Check if the player is already in a game
        if (isPlayerInGame(player)) {
            std::string gameId = playerToGameId[player];
            logger->warning("Player " + player->getUsername() + " tried to join matchmaking while in game " + gameId);
            
            response["status"] = "already_in_game";
            response["message"] = "You are already in a game";
            response["gameId"] = QString::fromStdString(gameId);
            sendMessage(socket, response);
            return;
        }
        
        try {
            // Add the player to the matchmaking queue
            matchmaker->addPlayer(player);
            
            response["status"] = "queued";
            response["message"] = "You have been added to the matchmaking queue";
            response["queueSize"] = matchmaker->getQueueSize();
            
            logger->log("Player " + player->getUsername() + " joined matchmaking queue");
        } catch (const std::exception& e) {
            logger->error("Exception adding player to matchmaking: " + std::string(e.what()));
            
            response["status"] = "error";
            response["message"] = "An error occurred while joining the matchmaking queue";
        }
    } else {
        try {
            // Remove the player from the matchmaking queue
            matchmaker->removePlayer(player);
            
            response["status"] = "left";
            response["message"] = "You have left the matchmaking queue";
            
            logger->log("Player " + player->getUsername() + " left matchmaking queue");
        } catch (const std::exception& e) {
            logger->error("Exception removing player from matchmaking: " + std::string(e.what()));
            
            response["status"] = "error";
            response["message"] = "An error occurred while leaving the matchmaking queue";
        }
    }
    
    sendMessage(socket, response);
}

void MPChessServer::processGameHistoryRequest(QTcpSocket* socket, const QJsonObject& data) {
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Game history request from unauthenticated socket");
        return;
    }
    
    QJsonObject response;
    response["type"] = static_cast<int>(MessageType::GAME_HISTORY_RESPONSE);
    
    if (data.contains("gameId")) {
        // Request for a specific game
        std::string gameId = data["gameId"].toString().toStdString();
        
        // Check if the game is active
        auto it = activeGames.find(gameId);
        if (it != activeGames.end()) {
            // Game is active
            response["success"] = true;
            response["gameHistory"] = it->second->getGameHistoryJson();
        } else {
            // Try to load the game from disk
            std::string filePath = getGameHistoryPath() + "/" + gameId + ".json";
            QFile file(QString::fromStdString(filePath));
            
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray fileData = file.readAll();
                QJsonDocument doc = QJsonDocument::fromJson(fileData);
                
                if (!doc.isNull() && doc.isObject()) {
                    response["success"] = true;
                    response["gameHistory"] = doc.object();
                } else {
                    response["success"] = false;
                    response["message"] = "Failed to parse game history";
                }
            } else {
                response["success"] = false;
                response["message"] = "Game not found";
            }
        }
    } else {
        // Request for all games
        QJsonArray gameHistories;
        
        // Add active games
        for (auto it = activeGames.begin(); it != activeGames.end(); ++it) {
            ChessGame* game = it->second.get();
            ChessPlayer* whitePlayer = game->getWhitePlayer();
            ChessPlayer* blackPlayer = game->getBlackPlayer();
            
            // Only include games that the player is part of
            if (whitePlayer == player || blackPlayer == player) {
                QJsonObject gameObj;
                gameObj["gameId"] = QString::fromStdString(game->getGameId());
                gameObj["whitePlayer"] = QString::fromStdString(whitePlayer->getUsername());
                gameObj["blackPlayer"] = QString::fromStdString(blackPlayer->getUsername());
                gameObj["result"] = [game]() -> QString {
                    switch (game->getResult()) {
                        case GameResult::WHITE_WIN: return "white_win";
                        case GameResult::BLACK_WIN: return "black_win";
                        case GameResult::DRAW: return "draw";
                        default: return "in_progress";
                    }
                }();
                gameObj["active"] = true;
                
                gameHistories.append(gameObj);
            }
        }
        
        // Add past games from the player's history
        for (const std::string& gameId : player->getGameHistory()) {
            // Skip active games
            if (activeGames.find(gameId) != activeGames.end()) {
                continue;
            }
            
            std::string filePath = getGameHistoryPath() + "/" + gameId + ".json";
            QFile file(QString::fromStdString(filePath));
            
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray fileData = file.readAll();
                QJsonDocument doc = QJsonDocument::fromJson(fileData);
                
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject gameObj = doc.object();
                    QJsonObject summaryObj;
                    
                    summaryObj["gameId"] = gameObj["gameId"];
                    summaryObj["whitePlayer"] = gameObj["whitePlayer"];
                    summaryObj["blackPlayer"] = gameObj["blackPlayer"];
                    summaryObj["result"] = gameObj["result"];
                    summaryObj["active"] = false;
                    summaryObj["startTime"] = gameObj["startTime"];
                    summaryObj["endTime"] = gameObj["endTime"];
                    
                    gameHistories.append(summaryObj);
                }
            }
        }
        
        response["success"] = true;
        response["gameHistories"] = gameHistories;
    }
    
    sendMessage(socket, response);
}

void MPChessServer::processGameAnalysisRequest(QTcpSocket* socket, const QJsonObject& data) {
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Game analysis request from unauthenticated socket");
        return;
    }
    
    QJsonObject response;
    response["type"] = static_cast<int>(MessageType::GAME_ANALYSIS_RESPONSE);
    
    std::string gameId = data["gameId"].toString().toStdString();
    
    // Check if the game is active
    auto it = activeGames.find(gameId);
    if (it != activeGames.end()) {
        // Game is active
        ChessGame* game = it->second.get();
        
        // Only allow analysis if the game is over or the player is part of the game
        ChessPlayer* whitePlayer = game->getWhitePlayer();
        ChessPlayer* blackPlayer = game->getBlackPlayer();
        
        if (game->isOver() || whitePlayer == player || blackPlayer == player) {
            response["success"] = true;
            response["analysis"] = analysisEngine->analyzeGame(*game);
        } else {
            response["success"] = false;
            response["message"] = "You are not allowed to analyze this game";
        }
    } else {
        // Try to load the game from disk
        std::string filePath = getGameHistoryPath() + "/" + gameId + ".json";
        QFile file(QString::fromStdString(filePath));
        
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(fileData);
            
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject gameObj = doc.object();
                
                // Check if the player was part of the game
                QString whitePlayer = gameObj["whitePlayer"].toString();
                QString blackPlayer = gameObj["blackPlayer"].toString();
                
                if (whitePlayer == QString::fromStdString(player->getUsername()) ||
                    blackPlayer == QString::fromStdString(player->getUsername()) ||
                    gameObj["result"].toString() != "in_progress") {
                    
                    // Load the game and analyze it
                    ChessPlayer dummyWhite(whitePlayer.toStdString());
                    ChessPlayer dummyBlack(blackPlayer.toStdString());
                    
                    std::unique_ptr<ChessGame> game = 
                        ChessGame::deserialize(gameObj, &dummyWhite, &dummyBlack);
                    
                    if (game) {
                        response["success"] = true;
                        response["analysis"] = analysisEngine->analyzeGame(*game);
                    } else {
                        response["success"] = false;
                        response["message"] = "Failed to load game for analysis";
                    }
                } else {
                    response["success"] = false;
                    response["message"] = "You are not allowed to analyze this game";
                }
            } else {
                response["success"] = false;
                response["message"] = "Failed to parse game data";
            }
        } else {
            response["success"] = false;
            response["message"] = "Game not found";
        }
    }
    
    sendMessage(socket, response);
}

void MPChessServer::processResignRequest(QTcpSocket* socket, const QJsonObject& data) {
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Resign request from unauthenticated socket");
        return;
    }
    
    std::string gameId = data["gameId"].toString().toStdString();
    
    // Find the game
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("Resign request for non-existent game: " + gameId);
        
        QJsonObject response;
        response["type"] = static_cast<int>(MessageType::ERROR);
        response["message"] = "Game not found";
        sendMessage(socket, response);
        return;
    }
    
    ChessGame* game = it->second.get();
    
    // Handle resignation
    game->handleResignation(player);
    
    // Send game over message to both players
    QJsonObject gameOverMessage;
    gameOverMessage["type"] = static_cast<int>(MessageType::GAME_OVER);
    gameOverMessage["result"] = (player == game->getWhitePlayer()) ? "black_win" : "white_win";
    gameOverMessage["reason"] = "resignation";
    
    ChessPlayer* whitePlayer = game->getWhitePlayer();
    ChessPlayer* blackPlayer = game->getBlackPlayer();
    
    if (whitePlayer->getSocket()) {
        sendMessage(whitePlayer->getSocket(), gameOverMessage);
    }
    
    if (blackPlayer->getSocket()) {
        sendMessage(blackPlayer->getSocket(), gameOverMessage);
    }
    
    // Update ratings
    updatePlayerRatings(game);
    
    // Save game history
    saveGameHistory(*game);
    
    // Log the resignation
    logger->log("Player " + player->getUsername() + " resigned in game " + gameId);
}

void MPChessServer::processDrawOfferRequest(QTcpSocket* socket, const QJsonObject& data) {
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Draw offer request from unauthenticated socket");
        return;
    }
    
    std::string gameId = data["gameId"].toString().toStdString();
    
    // Find the game
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("Draw offer request for non-existent game: " + gameId);
        
        QJsonObject response;
        response["type"] = static_cast<int>(MessageType::ERROR);
        response["message"] = "Game not found";
        sendMessage(socket, response);
        return;
    }
    
    ChessGame* game = it->second.get();
    
    // Handle draw offer
    if (game->handleDrawOffer(player)) {
        // Send draw offer message to the opponent
        QJsonObject drawOfferMessage;
        drawOfferMessage["type"] = static_cast<int>(MessageType::DRAW_OFFER);
        drawOfferMessage["offeredBy"] = QString::fromStdString(player->getUsername());
        
        ChessPlayer* opponent = game->getOpponentPlayer(player);
        if (opponent->getSocket()) {
            sendMessage(opponent->getSocket(), drawOfferMessage);
        }
        
        // Send confirmation to the offering player
        QJsonObject confirmationMessage;
        confirmationMessage["type"] = static_cast<int>(MessageType::DRAW_OFFER);
        confirmationMessage["status"] = "sent";
        sendMessage(socket, confirmationMessage);
        
        // Log the draw offer
        logger->log("Player " + player->getUsername() + " offered a draw in game " + gameId);
    } else {
        // Send error message
        QJsonObject errorMessage;
        errorMessage["type"] = static_cast<int>(MessageType::ERROR);
        errorMessage["message"] = "Cannot offer draw at this time";
        sendMessage(socket, errorMessage);
    }
}

void MPChessServer::processDrawResponseRequest(QTcpSocket* socket, const QJsonObject& data) {
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Draw response request from unauthenticated socket");
        return;
    }
    
    std::string gameId = data["gameId"].toString().toStdString();
    bool accepted = data["accepted"].toBool();
    
    // Find the game
    auto it = activeGames.find(gameId);
    if (it == activeGames.end()) {
        logger->error("Draw response request for non-existent game: " + gameId);
        
        QJsonObject response;
        response["type"] = static_cast<int>(MessageType::ERROR);
        response["message"] = "Game not found";
        sendMessage(socket, response);
        return;
    }
    
    ChessGame* game = it->second.get();
    
    // Handle draw response
    game->handleDrawResponse(player, accepted);
    
    if (accepted) {
        // Send game over message to both players
        QJsonObject gameOverMessage;
        gameOverMessage["type"] = static_cast<int>(MessageType::GAME_OVER);
        gameOverMessage["result"] = "draw";
        gameOverMessage["reason"] = "agreement";
        
        ChessPlayer* whitePlayer = game->getWhitePlayer();
        ChessPlayer* blackPlayer = game->getBlackPlayer();
        
        if (whitePlayer->getSocket()) {
            sendMessage(whitePlayer->getSocket(), gameOverMessage);
        }
        
        if (blackPlayer->getSocket()) {
            sendMessage(blackPlayer->getSocket(), gameOverMessage);
        }
        
        // Update ratings
        updatePlayerRatings(game);
        
        // Save game history
        saveGameHistory(*game);
        
        // Log the draw agreement
        logger->log("Draw agreed in game " + gameId);
    } else {
        // Send draw declined message to the offering player
        QJsonObject declinedMessage;
        declinedMessage["type"] = static_cast<int>(MessageType::DRAW_RESPONSE);
        declinedMessage["accepted"] = false;
        
        ChessPlayer* opponent = game->getOpponentPlayer(player);
        if (opponent->getSocket()) {
            sendMessage(opponent->getSocket(), declinedMessage);
        }
        
        // Log the draw decline
        logger->log("Player " + player->getUsername() + " declined draw offer in game " + gameId);
    }
}

ChessPlayer* MPChessServer::createBotPlayer(int skillLevel) {
    // Ensure skill level is in valid range
    skillLevel = std::min(std::max(skillLevel, 1), 10);
    
    // Generate a bot username
    std::string botUsername = "Bot_" + std::to_string(skillLevel) + "_" + 
                             std::to_string(QRandomGenerator::global()->bounded(1000));
    
    // Create the bot player
    ChessPlayer* botPlayer = new ChessPlayer(botUsername);
    botPlayer->setBot(true);
    botPlayer->setRating(1000 + skillLevel * 100);  // Simple rating based on skill level
    
    // Store the bot player
    usernamesToPlayers[botUsername] = botPlayer;
    
    logger->log("Created bot player: " + botUsername + " with skill level " + std::to_string(skillLevel));
    
    return botPlayer;
}

void MPChessServer::saveGameHistory(const ChessGame& game) {
    std::string gameId = game.getGameId();
    std::string filePath = getGameHistoryPath() + "/" + gameId + ".json";
    
    // Serialize the game
    QJsonObject gameJson = game.getGameHistoryJson();
    QJsonDocument doc(gameJson);
    
    // Save to file
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::WriteOnly)) {
        logger->error("Failed to save game history: " + filePath);
        return;
    }
    
    file.write(doc.toJson());
    
    // Add the game to the players' history
    ChessPlayer* whitePlayer = game.getWhitePlayer();
    ChessPlayer* blackPlayer = game.getBlackPlayer();
    
    whitePlayer->addGameToHistory(gameId);
    blackPlayer->addGameToHistory(gameId);
    
    // Save the players' data
    authenticator->savePlayer(*whitePlayer);
    authenticator->savePlayer(*blackPlayer);
    
    logger->log("Saved game history: " + gameId);
}

QJsonArray MPChessServer::loadAllGameHistories() {
    QJsonArray histories;
    
    QDir dir(QString::fromStdString(getGameHistoryPath()));
    QStringList filters;
    filters << "*.json";
    QStringList files = dir.entryList(filters, QDir::Files);
    
    for (const QString& file : files) {
        QFile jsonFile(dir.filePath(file));
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QByteArray data = jsonFile.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (!doc.isNull() && doc.isObject()) {
                histories.append(doc.object());
            }
        }
    }
    
    return histories;
}

void MPChessServer::updatePlayerRatings(ChessGame* game) {
    if (!game->isOver()) {
        return;
    }
    
    ChessPlayer* whitePlayer = game->getWhitePlayer();
    ChessPlayer* blackPlayer = game->getBlackPlayer();
    
    // Get current ratings
    int whiteRating = whitePlayer->getRating();
    int blackRating = blackPlayer->getRating();
    
    // Calculate new ratings
    auto [newWhiteRating, newBlackRating] = 
        ratingSystem->calculateNewRatings(whiteRating, blackRating, game->getResult());
    
    // Update player ratings
    whitePlayer->setRating(newWhiteRating);
    blackPlayer->setRating(newBlackRating);
    
    // Save player data
    authenticator->savePlayer(*whitePlayer);
    authenticator->savePlayer(*blackPlayer);
    
    // Update leaderboard
    leaderboard->updatePlayer(*whitePlayer);
    leaderboard->updatePlayer(*blackPlayer);
    
    logger->log("Updated ratings: " + whitePlayer->getUsername() + " " + 
               std::to_string(whiteRating) + " -> " + std::to_string(newWhiteRating) + ", " +
               blackPlayer->getUsername() + " " + std::to_string(blackRating) + 
               " -> " + std::to_string(newBlackRating));
}

void MPChessServer::processLeaderboardRequest(QTcpSocket* socket, const QJsonObject& data) {
    ChessPlayer* player = socketToPlayer.value(socket, nullptr);
    if (!player) {
        logger->error("Leaderboard request from unauthenticated socket");
        return;
    }
    
    // Check if all players are requested
    bool allPlayers = data.contains("all") && data["all"].toBool();
    
    // Get count parameter (default to 100 if not requesting all)
    int count = allPlayers ? -1 : (data.contains("count") ? data["count"].toInt() : 100);
    
    // If not requesting all, ensure count is between 1 and 100
    if (!allPlayers) {
        count = std::min(std::max(count, 1), 100);
    }
    
    QJsonObject response;
    response["type"] = static_cast<int>(MessageType::LEADERBOARD_RESPONSE);
    
    // Generate the leaderboard
    response["leaderboard"] = leaderboard->generateLeaderboardJson(count);
    
    // Add the requesting player's ranks
    std::string username = player->getUsername();
    response["yourRanks"] = QJsonObject({
        {"byRating", leaderboard->getPlayerRatingRank(username)},
        {"byWins", leaderboard->getPlayerWinsRank(username)},
        {"byWinPercentage", leaderboard->getPlayerWinPercentageRank(username)}
    });
    
    sendMessage(socket, response);
    
    logger->log("Sent leaderboard to player: " + username + (allPlayers ? " (all players)" : 
                " (top " + std::to_string(count) + " players)"));
}

void MPChessServer::cleanupDisconnectedPlayer(ChessPlayer* player)
{
    if (!player) {
        logger->error("Attempted to cleanup null player");
        return;
    }
    
    std::string username;
    try {
        username = player->getUsername();
        logger->debug("Cleaning up disconnected player: " + username);
    } catch (...) {
        logger->error("Exception getting username from disconnected player");
        username = "unknown";
    }

    // Remove from matchmaking queue safely
    try {
        matchmaker->removePlayer(player);
    } catch (...) {
        logger->error("Exception removing player from matchmaking queue");
    }
    
    // Check if the player is in a game
    std::string gameId;
    bool playerInGame = false;
    
    try {
        // Make a copy of the game ID to avoid accessing the map after removal
        if (playerToGameId.contains(player)) {
            gameId = playerToGameId[player];
            playerInGame = true;
            
            // Remove from the map first to prevent recursive access
            playerToGameId.remove(player);
        }
    } catch (...) {
        logger->error("Exception checking if player is in a game");
    }
    
    // Handle the game if the player was in one
    if (playerInGame && !gameId.empty()) {
        try {
            auto it = activeGames.find(gameId);
            if (it != activeGames.end()) {
                ChessGame* game = it->second.get();
                
                // Get opponent before potentially modifying the game
                ChessPlayer* opponent = nullptr;
                try {
                    opponent = game->getOpponentPlayer(player);
                } catch (...) {
                    logger->error("Exception getting opponent player");
                }
                
                // If the game is not over, handle as a resignation
                if (!game->isOver()) {
                    try {
                        game->handleResignation(player);
                        logger->log("Player " + username + " disconnected during game " + gameId);
                    } catch (...) {
                        logger->error("Exception handling resignation for disconnected player");
                    }
                    
                    // Send game over message to the opponent
                    if (opponent && opponent->getSocket()) {
                        try {
                            QJsonObject gameOverMessage;
                            gameOverMessage["type"] = static_cast<int>(MessageType::GAME_OVER);
                            gameOverMessage["result"] = (player == game->getWhitePlayer()) ? "black_win" : "white_win";
                            gameOverMessage["reason"] = "disconnection";
                            
                            sendMessage(opponent->getSocket(), gameOverMessage);
                        } catch (...) {
                            logger->error("Exception sending game over message to opponent");
                        }
                    }
                    
                    // Update ratings
                    try {
                        updatePlayerRatings(game);
                    } catch (...) {
                        logger->error("Exception updating ratings after player disconnection");
                    }
                    
                    // Save game history
                    try {
                        saveGameHistory(*game);
                    } catch (...) {
                        logger->error("Exception saving game history after player disconnection");
                    }
                }
            }
        } catch (...) {
            logger->error("Exception handling game for disconnected player");
        }
    }
    
    // Remove from username map - do this before deleting the player
    try {
        usernamesToPlayers.remove(username);
    } catch (...) {
        logger->error("Exception removing player from username map");
    }
    
    // Save player data before deleting
    try {
        authenticator->savePlayer(*player);
    } catch (...) {
        logger->error("Exception saving player data before deletion");
    }
    
    // Delete the player
    try {
        delete player;
        logger->debug("Successfully deleted player object for: " + username);
    } catch (...) {
        logger->error("Exception deleting player object");
    }
}

void MPChessServer::initializeServerDirectories() {
    // Create necessary directories
    QDir().mkpath(QString::fromStdString(getGameHistoryPath()));
    QDir().mkpath(QString::fromStdString(getPlayerDataPath()));
    QDir().mkpath(QString::fromStdString(getLogsPath()));
}

std::string MPChessServer::getGameHistoryPath() const {
    return "data/game_history";
}

std::string MPChessServer::getPlayerDataPath() const {
    return "data/players";
}

std::string MPChessServer::getLogsPath() const {
    return "data/logs";
}

// Implementation of StockfishConnector class
StockfishConnector::StockfishConnector(const std::string& enginePath, int skillLevel, int depth)
    : enginePath(enginePath), skillLevel(skillLevel), depth(depth), process(nullptr), initialized(false) {
}

StockfishConnector::~StockfishConnector() {
    if (process) {
        if (process->state() == QProcess::Running) {
            sendCommand("quit");
            process->waitForFinished(1000);
        }
        delete process;
    }
}

bool StockfishConnector::initialize() {
    if (initialized) return true;
    
    process = new QProcess();
    process->setProcessChannelMode(QProcess::MergedChannels);
    
    process->start(QString::fromStdString(enginePath), QStringList());
    
    if (!process->waitForStarted(3000)) {
        delete process;
        process = nullptr;
        return false;
    }
    
    // Initialize the engine
    sendCommand("uci");
    sendCommand("isready");
    
    // Set options
    sendCommand("setoption name Skill Level value " + std::to_string(skillLevel));
    sendCommand("setoption name Threads value 4");  // Use 4 threads by default
    sendCommand("setoption name Hash value 128");   // Use 128MB hash by default
    sendCommand("isready");
    
    initialized = true;
    return true;
}

bool StockfishConnector::isInitialized() const {
    return initialized;
}

void StockfishConnector::setSkillLevel(int level) {
    if (!initialized) return;
    
    skillLevel = std::min(std::max(level, 0), 20);
    sendCommand("setoption name Skill Level value " + std::to_string(skillLevel));
    sendCommand("isready");
}

void StockfishConnector::setDepth(int d) {
    depth = std::max(d, 1);
}

void StockfishConnector::setPosition(const ChessBoard& board) {
    if (!initialized) return;
    
    std::string fen = boardToFen(board);
    sendCommand("position fen " + fen);
    sendCommand("isready");
}

ChessMove StockfishConnector::getBestMove() {
    if (!initialized) return ChessMove();
    
    std::string output = sendCommandAndGetOutput("go depth " + std::to_string(depth), "bestmove");
    
    // Parse the best move
    size_t pos = output.find("bestmove");
    if (pos == std::string::npos) return ChessMove();
    
    std::string moveStr = output.substr(pos + 9, 4);  // Extract the move (e.g., "e2e4")
    
    // Convert to our move format
    Position from(moveStr[1] - '1', moveStr[0] - 'a');
    Position to(moveStr[3] - '1', moveStr[2] - 'a');
    
    // Check for promotion
    PieceType promotionType = PieceType::EMPTY;
    if (moveStr.length() > 4 && moveStr[4] != ' ') {
        switch (moveStr[4]) {
            case 'q': promotionType = PieceType::QUEEN; break;
            case 'r': promotionType = PieceType::ROOK; break;
            case 'b': promotionType = PieceType::BISHOP; break;
            case 'n': promotionType = PieceType::KNIGHT; break;
            default: break;
        }
    }
    
    return ChessMove(from, to, promotionType);
}

std::vector<std::pair<ChessMove, double>> StockfishConnector::getMoveRecommendations(int maxRecommendations) {
    if (!initialized) return {};
    
    std::vector<std::pair<ChessMove, double>> recommendations;
    
    // Use the multipv option to get multiple best moves
    sendCommand("setoption name MultiPV value " + std::to_string(maxRecommendations));
    sendCommand("isready");
    
    std::string output = sendCommandAndGetOutput("go depth " + std::to_string(depth), "bestmove");
    
    // Parse the output to extract moves and scores
    std::istringstream iss(output);
    std::string line;
    std::map<int, std::pair<std::string, double>> pvMoves;  // Map from multipv index to (move, score)
    
    while (std::getline(iss, line)) {
        if (line.find("info depth") != std::string::npos && line.find("multipv") != std::string::npos) {
            // Extract multipv index
            size_t multipvPos = line.find("multipv");
            if (multipvPos == std::string::npos) continue;
            
            size_t multipvEndPos = line.find(" ", multipvPos + 8);
            if (multipvEndPos == std::string::npos) continue;
            
            int multipvIndex = std::stoi(line.substr(multipvPos + 8, multipvEndPos - multipvPos - 8));
            
            // Extract score
            size_t scorePos = line.find("score cp");
            size_t matePos = line.find("score mate");
            double score = 0.0;
            
            if (scorePos != std::string::npos) {
                size_t scoreEndPos = line.find(" ", scorePos + 9);
                if (scoreEndPos != std::string::npos) {
                    score = std::stoi(line.substr(scorePos + 9, scoreEndPos - scorePos - 9)) / 100.0;
                }
            } else if (matePos != std::string::npos) {
                size_t mateEndPos = line.find(" ", matePos + 11);
                if (mateEndPos != std::string::npos) {
                    int mateIn = std::stoi(line.substr(matePos + 11, mateEndPos - matePos - 11));
                    score = mateIn > 0 ? 100.0 : -100.0;  // Use a large value for mate
                }
            }
            
            // Extract move
            size_t pvPos = line.find(" pv ");
            if (pvPos != std::string::npos) {
                std::string moveStr = line.substr(pvPos + 4, 4);  // Extract the first move
                pvMoves[multipvIndex] = std::make_pair(moveStr, score);
            }
        }
    }
    
    // Convert to our format
    for (int i = 1; i <= maxRecommendations; ++i) {
        if (pvMoves.find(i) != pvMoves.end()) {
            const auto& [moveStr, score] = pvMoves[i];
            
            Position from(moveStr[1] - '1', moveStr[0] - 'a');
            Position to(moveStr[3] - '1', moveStr[2] - 'a');
            
            // Check for promotion
            PieceType promotionType = PieceType::EMPTY;
            if (moveStr.length() > 4 && moveStr[4] != ' ') {
                switch (moveStr[4]) {
                    case 'q': promotionType = PieceType::QUEEN; break;
                    case 'r': promotionType = PieceType::ROOK; break;
                    case 'b': promotionType = PieceType::BISHOP; break;
                    case 'n': promotionType = PieceType::KNIGHT; break;
                    default: break;
                }
            }
            
            recommendations.emplace_back(ChessMove(from, to, promotionType), score);
        }
    }
    
    // Reset MultiPV to 1
    sendCommand("setoption name MultiPV value 1");
    sendCommand("isready");
    
    return recommendations;
}

double StockfishConnector::evaluatePosition() {
    if (!initialized) return 0.0;
    
    std::string output = sendCommandAndGetOutput("go depth " + std::to_string(depth / 2), "bestmove");
    
    // Parse the evaluation
    size_t scorePos = output.rfind("score cp");
    if (scorePos != std::string::npos) {
        size_t scoreEndPos = output.find(" ", scorePos + 9);
        if (scoreEndPos != std::string::npos) {
            return std::stoi(output.substr(scorePos + 9, scoreEndPos - scorePos - 9)) / 100.0;
        }
    }
    
    size_t matePos = output.rfind("score mate");
    if (matePos != std::string::npos) {
        size_t mateEndPos = output.find(" ", matePos + 11);
        if (mateEndPos != std::string::npos) {
            int mateIn = std::stoi(output.substr(matePos + 11, mateEndPos - matePos - 11));
            return mateIn > 0 ? 100.0 : -100.0;  // Use a large value for mate
        }
    }
    
    return 0.0;
}

QJsonObject StockfishConnector::analyzePosition(const ChessBoard& board) {
    QJsonObject analysis;
    
    if (!initialized) {
        analysis["error"] = "Stockfish not initialized";
        return analysis;
    }
    
    setPosition(board);
    
    // Get evaluation
    double eval = evaluatePosition();
    analysis["evaluation"] = eval;
    
    // Get best moves
    std::vector<std::pair<ChessMove, double>> recommendations = getMoveRecommendations(5);
    
    QJsonArray movesArray;
    for (const auto& [move, score] : recommendations) {
        QJsonObject moveObj;
        moveObj["move"] = QString::fromStdString(move.toAlgebraic());
        moveObj["score"] = score;
        moveObj["standardNotation"] = QString::fromStdString(move.toStandardNotation(board));
        movesArray.append(moveObj);
    }
    
    analysis["bestMoves"] = movesArray;
    
    return analysis;
}

QJsonObject StockfishConnector::analyzeGame(const ChessGame& game) {
    QJsonObject analysis;
    
    if (!initialized) {
        analysis["error"] = "Stockfish not initialized";
        return analysis;
    }
    
    // Game overview
    analysis["gameId"] = QString::fromStdString(game.getGameId());
    analysis["whitePlayer"] = QString::fromStdString(game.getWhitePlayer()->getUsername());
    analysis["blackPlayer"] = QString::fromStdString(game.getBlackPlayer()->getUsername());
    analysis["result"] = [&game]() -> QString {
        switch (game.getResult()) {
            case GameResult::WHITE_WIN: return "white_win";
            case GameResult::BLACK_WIN: return "black_win";
            case GameResult::DRAW: return "draw";
            default: return "in_progress";
        }
    }();
    
    // Analyze each move
    QJsonArray moveAnalysis;
    const ChessBoard* board = game.getBoard();
    auto moveHistory = board->getMoveHistory();
    
    // Create a temporary board to replay the game
    auto tempBoard = std::make_unique<ChessBoard>();
    
    for (size_t i = 0; i < moveHistory.size(); ++i) {
        const ChessMove& move = moveHistory[i];
        
        // Analyze the position before the move
        setPosition(*tempBoard);
        double evalBefore = evaluatePosition();
        
        // Make the move
        tempBoard->movePiece(move);
        
        // Analyze the position after the move
        setPosition(*tempBoard);
        double evalAfter = evaluatePosition();
        
        // Calculate evaluation change
        double evalChange = evalAfter - evalBefore;
        
        // Classify the move
        std::string classification;
        if (evalChange > 2.0) classification = "Brilliant";
        else if (evalChange > 1.0) classification = "Good";
        else if (evalChange > 0.3) classification = "Accurate";
        else if (evalChange > -0.3) classification = "Normal";
        else if (evalChange > -1.0) classification = "Inaccuracy";
        else if (evalChange > -2.0) classification = "Mistake";
        else classification = "Blunder";
        
        // Create move analysis object
        QJsonObject moveObj;
        moveObj["moveNumber"] = static_cast<int>(i / 2) + 1;
        moveObj["color"] = (i % 2 == 0) ? "white" : "black";
        moveObj["move"] = QString::fromStdString(move.toAlgebraic());
        moveObj["standardNotation"] = QString::fromStdString(move.toStandardNotation(*tempBoard));
        moveObj["evaluationBefore"] = evalBefore;
        moveObj["evaluationAfter"] = evalAfter;
        moveObj["evaluationChange"] = evalChange;
        moveObj["classification"] = QString::fromStdString(classification);
        
        // Get alternative moves
        setPosition(*tempBoard);
        std::vector<std::pair<ChessMove, double>> alternatives = getMoveRecommendations(3);
        
        QJsonArray alternativesArray;
        for (const auto& [altMove, altScore] : alternatives) {
            QJsonObject altObj;
            altObj["move"] = QString::fromStdString(altMove.toAlgebraic());
            altObj["score"] = altScore;
            altObj["standardNotation"] = QString::fromStdString(altMove.toStandardNotation(*tempBoard));
            alternativesArray.append(altObj);
        }
        
        moveObj["alternatives"] = alternativesArray;
        moveAnalysis.append(moveObj);
    }
    
    analysis["moveAnalysis"] = moveAnalysis;
    
    return analysis;
}

void StockfishConnector::sendCommand(const std::string& command) {
    if (!process || process->state() != QProcess::Running) return;
    
    process->write((command + "\n").c_str());
    process->waitForBytesWritten();
}

std::string StockfishConnector::sendCommandAndGetOutput(const std::string& command, const std::string& terminator) {
    if (!process || process->state() != QProcess::Running) return "";
    
    process->write((command + "\n").c_str());
    process->waitForBytesWritten();
    
    std::string output;
    QByteArray line;
    
    // Read until we find the terminator
    while (true) {
        if (process->waitForReadyRead(5000)) {
            line = process->readLine();
            output += line.constData();
            
            if (line.contains(terminator.c_str())) {
                break;
            }
        } else {
            break;  // Timeout
        }
    }
    
    return output;
}

double StockfishConnector::parseEvaluation(const std::string& evalStr) {
    if (evalStr.find("mate") != std::string::npos) {
        // Handle mate scores
        size_t pos = evalStr.find("mate");
        std::string mateStr = evalStr.substr(pos + 5);
        int mateIn = std::stoi(mateStr);
        
        return mateIn > 0 ? 100.0 : -100.0;  // Use a large value for mate
    } else {
        // Handle centipawn scores
        size_t pos = evalStr.find("cp");
        std::string cpStr = evalStr.substr(pos + 3);
        int cp = std::stoi(cpStr);
        
        return cp / 100.0;  // Convert centipawns to pawns
    }
}

std::string StockfishConnector::boardToFen(const ChessBoard& board) {
    std::stringstream ss;
    
    // Piece placement
    for (int r = 7; r >= 0; --r) {
        int emptyCount = 0;
        
        for (int c = 0; c < 8; ++c) {
            const ChessPiece* piece = board.getPiece(Position(r, c));
            
            if (piece) {
                if (emptyCount > 0) {
                    ss << emptyCount;
                    emptyCount = 0;
                }
                
                char pieceChar = piece->getAsciiChar();
                ss << pieceChar;
            } else {
                emptyCount++;
            }
        }
        
        if (emptyCount > 0) {
            ss << emptyCount;
        }
        
        if (r > 0) {
            ss << "/";
        }
    }
    
    // Active color
    ss << " " << (board.getCurrentTurn() == PieceColor::WHITE ? "w" : "b") << " ";
    
    // Castling availability
    bool hasWhiteKingside = false;
    bool hasWhiteQueenside = false;
    bool hasBlackKingside = false;
    bool hasBlackQueenside = false;
    
    // Check white king and rooks
    const ChessPiece* whiteKing = board.getPiece(Position(0, 4));
    const ChessPiece* whiteKingsideRook = board.getPiece(Position(0, 7));
    const ChessPiece* whiteQueensideRook = board.getPiece(Position(0, 0));
    
    if (whiteKing && whiteKing->getType() == PieceType::KING && !whiteKing->hasMoved()) {
        if (whiteKingsideRook && whiteKingsideRook->getType() == PieceType::ROOK && !whiteKingsideRook->hasMoved()) {
            hasWhiteKingside = true;
        }
        
        if (whiteQueensideRook && whiteQueensideRook->getType() == PieceType::ROOK && !whiteQueensideRook->hasMoved()) {
            hasWhiteQueenside = true;
        }
    }
    
    // Check black king and rooks
    const ChessPiece* blackKing = board.getPiece(Position(7, 4));
    const ChessPiece* blackKingsideRook = board.getPiece(Position(7, 7));
    const ChessPiece* blackQueensideRook = board.getPiece(Position(7, 0));
    
    if (blackKing && blackKing->getType() == PieceType::KING && !blackKing->hasMoved()) {
        if (blackKingsideRook && blackKingsideRook->getType() == PieceType::ROOK && !blackKingsideRook->hasMoved()) {
            hasBlackKingside = true;
        }
        
        if (blackQueensideRook && blackQueensideRook->getType() == PieceType::ROOK && !blackQueensideRook->hasMoved()) {
            hasBlackQueenside = true;
        }
    }
    
    if (hasWhiteKingside) ss << "K";
    if (hasWhiteQueenside) ss << "Q";
    if (hasBlackKingside) ss << "k";
    if (hasBlackQueenside) ss << "q";
    
    if (!hasWhiteKingside && !hasWhiteQueenside && !hasBlackKingside && !hasBlackQueenside) {
        ss << "-";
    }
    
    // En passant target square
    Position enPassantTarget = board.getEnPassantTarget();
    if (enPassantTarget.isValid()) {
        ss << " " << enPassantTarget.toAlgebraic();
    } else {
        ss << " -";
    }
    
    // Halfmove clock and fullmove number
    // For simplicity, we'll use placeholder values
    ss << " 0 1";
    
    return ss.str();
}

ChessMove StockfishConnector::parseStockfishMove(const std::string& moveStr, const ChessBoard& board) {
    if (moveStr.length() < 4) return ChessMove();
    
    Position from(moveStr[1] - '1', moveStr[0] - 'a');
    Position to(moveStr[3] - '1', moveStr[2] - 'a');
    
    // Check for promotion
    PieceType promotionType = PieceType::EMPTY;
    if (moveStr.length() > 4) {
        switch (moveStr[4]) {
            case 'q': promotionType = PieceType::QUEEN; break;
            case 'r': promotionType = PieceType::ROOK; break;
            case 'b': promotionType = PieceType::BISHOP; break;
            case 'n': promotionType = PieceType::KNIGHT; break;
            default: break;
        }
    }
    
    return ChessMove(from, to, promotionType);
}

// Implementation of ChessLeaderboard class
ChessLeaderboard::ChessLeaderboard(const std::string& dataPath) : dataPath(dataPath) {
    refreshLeaderboard();
}

void ChessLeaderboard::updatePlayer(const ChessPlayer& player) {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    std::string username = player.getUsername();
    int rating = player.getRating();
    int wins = player.getWins();
    int losses = player.getLosses();
    int draws = player.getDraws();
    double winPercentage = player.getGamesPlayed() > 0 ? 
        (static_cast<double>(wins) / player.getGamesPlayed()) * 100.0 : 0.0;
    
    // Check if player already exists in leaderboard
    auto it = std::find_if(leaderboardData.begin(), leaderboardData.end(),
                          [&username](const auto& entry) {
                              return std::get<0>(entry) == username;
                          });
    
    if (it != leaderboardData.end()) {
        // Update existing player
        *it = std::make_tuple(username, rating, wins, losses, draws, winPercentage);
    } else {
        // Add new player
        leaderboardData.emplace_back(username, rating, wins, losses, draws, winPercentage);
    }
    
    // Re-sort the leaderboard
    sortByRating();
}

std::vector<std::pair<std::string, int>> ChessLeaderboard::getTopPlayersByRating(int count) {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    // Sort by rating
    sortByRating();
    
    // Extract top players
    std::vector<std::pair<std::string, int>> topPlayers;
    int numPlayers = (count == -1) ? leaderboardData.size() : 
                     std::min(static_cast<int>(leaderboardData.size()), count);
    
    for (int i = 0; i < numPlayers; ++i) {
        const auto& [username, rating, wins, losses, draws, winPercentage] = leaderboardData[i];
        topPlayers.emplace_back(username, rating);
    }
    
    return topPlayers;
}

std::vector<std::pair<std::string, int>> ChessLeaderboard::getTopPlayersByWins(int count) {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    // Sort by wins
    sortByWins();
    
    // Extract top players
    std::vector<std::pair<std::string, int>> topPlayers;
    int numPlayers = (count == -1) ? leaderboardData.size() : 
                     std::min(static_cast<int>(leaderboardData.size()), count);
    
    for (int i = 0; i < numPlayers; ++i) {
        const auto& [username, rating, wins, losses, draws, winPercentage] = leaderboardData[i];
        topPlayers.emplace_back(username, wins);
    }
    
    return topPlayers;
}

std::vector<std::pair<std::string, double>> ChessLeaderboard::getTopPlayersByWinPercentage(int count) {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    // Sort by win percentage
    sortByWinPercentage();
    
    // Extract top players (minimum 10 games)
    std::vector<std::pair<std::string, double>> topPlayers;
    int added = 0;
    
    for (const auto& entry : leaderboardData) {
        const auto& [username, rating, wins, losses, draws, winPercentage] = entry;
        
        // Only include players with at least 10 games
        if (wins + losses + draws >= 10) {
            topPlayers.emplace_back(username, winPercentage);
            added++;
            
            if (count != -1 && added >= count) {
                break;
            }
        }
    }
    
    return topPlayers;
}

int ChessLeaderboard::getPlayerRatingRank(const std::string& username) {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    // Sort by rating
    sortByRating();
    
    // Find player's rank
    for (size_t i = 0; i < leaderboardData.size(); ++i) {
        if (std::get<0>(leaderboardData[i]) == username) {
            return static_cast<int>(i) + 1;  // Ranks are 1-based
        }
    }
    
    return -1;  // Player not found
}

int ChessLeaderboard::getPlayerWinsRank(const std::string& username) {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    // Sort by wins
    sortByWins();
    
    // Find player's rank
    for (size_t i = 0; i < leaderboardData.size(); ++i) {
        if (std::get<0>(leaderboardData[i]) == username) {
            return static_cast<int>(i) + 1;  // Ranks are 1-based
        }
    }
    
    return -1;  // Player not found
}

int ChessLeaderboard::getPlayerWinPercentageRank(const std::string& username) {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    // Sort by win percentage
    sortByWinPercentage();
    
    // Find player's rank (only counting players with at least 10 games)
    int rank = 1;
    
    for (const auto& entry : leaderboardData) {
        const auto& [entryUsername, rating, wins, losses, draws, winPercentage] = entry;
        
        // Only count players with at least 10 games
        if (wins + losses + draws >= 10) {
            if (entryUsername == username) {
                return rank;
            }
            rank++;
        }
    }
    
    return -1;  // Player not found or doesn't have enough games
}

QJsonObject ChessLeaderboard::generateLeaderboardJson(int count) {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    QJsonObject leaderboardJson;
    
    // Top players by rating
    sortByRating();
    QJsonArray ratingLeaderboard;
    int numPlayers = (count == -1) ? leaderboardData.size() : 
                     std::min(static_cast<int>(leaderboardData.size()), count);
    
    for (int i = 0; i < numPlayers; ++i) {
        const auto& [username, rating, wins, losses, draws, winPercentage] = leaderboardData[i];
        
        QJsonObject playerObj;
        playerObj["rank"] = i + 1;
        playerObj["username"] = QString::fromStdString(username);
        playerObj["rating"] = rating;
        playerObj["wins"] = wins;
        playerObj["losses"] = losses;
        playerObj["draws"] = draws;
        playerObj["gamesPlayed"] = wins + losses + draws;
        playerObj["winPercentage"] = winPercentage;
        
        ratingLeaderboard.append(playerObj);
    }
    
    leaderboardJson["byRating"] = ratingLeaderboard;
    
    // Top players by wins
    sortByWins();
    QJsonArray winsLeaderboard;
    
    for (int i = 0; i < numPlayers; ++i) {
        const auto& [username, rating, wins, losses, draws, winPercentage] = leaderboardData[i];
        
        QJsonObject playerObj;
        playerObj["rank"] = i + 1;
        playerObj["username"] = QString::fromStdString(username);
        playerObj["rating"] = rating;
        playerObj["wins"] = wins;
        playerObj["losses"] = losses;
        playerObj["draws"] = draws;
        playerObj["gamesPlayed"] = wins + losses + draws;
        playerObj["winPercentage"] = winPercentage;
        
        winsLeaderboard.append(playerObj);
    }
    
    leaderboardJson["byWins"] = winsLeaderboard;
    
    // Top players by win percentage (minimum 10 games)
    sortByWinPercentage();
    QJsonArray winPercentageLeaderboard;
    int added = 0;
    
    for (const auto& entry : leaderboardData) {
        const auto& [username, rating, wins, losses, draws, winPercentage] = entry;
        
        // Only include players with at least 10 games
        if (wins + losses + draws >= 10) {
            QJsonObject playerObj;
            playerObj["rank"] = added + 1;
            playerObj["username"] = QString::fromStdString(username);
            playerObj["rating"] = rating;
            playerObj["wins"] = wins;
            playerObj["losses"] = losses;
            playerObj["draws"] = draws;
            playerObj["gamesPlayed"] = wins + losses + draws;
            playerObj["winPercentage"] = winPercentage;
            
            winPercentageLeaderboard.append(playerObj);
            added++;
            
            if (count != -1 && added >= count) {
                break;
            }
        }
    }
    
    leaderboardJson["byWinPercentage"] = winPercentageLeaderboard;
    
    // Add total player count
    leaderboardJson["totalPlayers"] = static_cast<int>(leaderboardData.size());
    
    // Add timestamp
    leaderboardJson["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    return leaderboardJson;
}

void ChessLeaderboard::refreshLeaderboard() {
    std::lock_guard<std::mutex> lock(leaderboardMutex);
    
    // Clear existing data
    leaderboardData.clear();
    
    // Load player data
    loadPlayerData();
    
    // Sort by rating
    sortByRating();
}

void ChessLeaderboard::loadPlayerData() {
    QDir dir(QString::fromStdString(dataPath));
    QStringList filters;
    filters << "player_*.json";
    QStringList files = dir.entryList(filters, QDir::Files);
    
    for (const QString& file : files) {
        QFile jsonFile(dir.filePath(file));
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QByteArray data = jsonFile.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject playerJson = doc.object();
                
                std::string username = playerJson["username"].toString().toStdString();
                int rating = playerJson["rating"].toInt();
                int wins = playerJson["wins"].toInt();
                int losses = playerJson["losses"].toInt();
                int draws = playerJson["draws"].toInt();
                double winPercentage = (wins + losses + draws > 0) ? 
                    (static_cast<double>(wins) / (wins + losses + draws)) * 100.0 : 0.0;
                
                leaderboardData.emplace_back(username, rating, wins, losses, draws, winPercentage);
            }
        }
    }
}

void ChessLeaderboard::sortByRating() {
    std::sort(leaderboardData.begin(), leaderboardData.end(),
             [](const auto& a, const auto& b) {
                 return std::get<1>(a) > std::get<1>(b);  // Sort by rating (descending)
             });
}

void ChessLeaderboard::sortByWins() {
    std::sort(leaderboardData.begin(), leaderboardData.end(),
             [](const auto& a, const auto& b) {
                 return std::get<2>(a) > std::get<2>(b);  // Sort by wins (descending)
             });
}

void ChessLeaderboard::sortByWinPercentage() {
    std::sort(leaderboardData.begin(), leaderboardData.end(),
             [](const auto& a, const auto& b) {
                 return std::get<5>(a) > std::get<5>(b);  // Sort by win percentage (descending)
             });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main function to control the server
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Multiplayer Chess Server");
    parser.addHelpOption();
    
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                 "Port to listen on (default: 5000)",
                                 "port", "5000");
    parser.addOption(portOption);
    
    QCommandLineOption stockfishOption(QStringList() << "s" << "stockfish",
                                     "Path to Stockfish chess engine",
                                     "path");
    parser.addOption(stockfishOption);
    
    QCommandLineOption stockfishDepthOption(QStringList() << "d" << "depth",
                                          "Stockfish analysis depth (default: 15)",
                                          "depth", "15");
    parser.addOption(stockfishDepthOption);
    
    QCommandLineOption stockfishSkillOption(QStringList() << "skill",
                                          "Stockfish skill level 0-20 (default: 20)",
                                          "skill", "20");
    parser.addOption(stockfishSkillOption);
    
    QCommandLineOption logLevelOption(QStringList() << "l" << "log-level",
                                    "Log level (0-3, default: 2)",
                                    "level", "2");
    parser.addOption(logLevelOption);
    
    parser.process(app);
    
    int port = parser.value(portOption).toInt();
    std::string stockfishPath = parser.value(stockfishOption).toStdString();
    int logLevel = parser.value(logLevelOption).toInt();
    
    try {
        // Create and start the server
        MPChessServer server(nullptr, stockfishPath);
        
        // Set log level
        if (server.getLogger()) {
            server.getLogger()->setLogLevel(logLevel);
            server.getLogger()->log("Log level set to " + std::to_string(logLevel), true);
        }
        
        // Configure Stockfish if it was initialized
        if (server.stockfishConnector && server.stockfishConnector->isInitialized()) {
            if (parser.isSet(stockfishDepthOption)) {
                int depth = parser.value(stockfishDepthOption).toInt();
                server.stockfishConnector->setDepth(depth);
                server.getLogger()->log("Stockfish depth set to " + std::to_string(depth), true);
            }
            
            if (parser.isSet(stockfishSkillOption)) {
                int skill = parser.value(stockfishSkillOption).toInt();
                server.stockfishConnector->setSkillLevel(skill);
                server.getLogger()->log("Stockfish skill level set to " + std::to_string(skill), true);
            }
        }
        
        if (!server.start(port)) {
            std::cerr << "Failed to start server on port " << port << std::endl;
            return 1;
        }
        
        std::cout << "Server started on port " << port << std::endl;
        std::cout << "Press Ctrl+C to quit" << std::endl;
        
        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        return 1;
    }
}