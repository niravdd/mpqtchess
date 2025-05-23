// ChessBoard.cpp
#include "ChessBoard.h"
#include <QPainter>
#include <QMouseEvent>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QDebug>

ChessBoard::ChessBoard(QWidget* parent)
    : QWidget(parent),
      selectedSquare(-1, -1),
      draggedPiece(-1, -1),
      isRotated(false)
{
    setMinimumSize(400, 400);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    
    // Initialize empty board
    resetBoard();
    
    // Load piece images
    loadPieceImages();
}

void ChessBoard::resetBoard() {
    // Initialize with standard chess position
    setPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void ChessBoard::setPosition(const QString& fen) {
    // Parse FEN string to set up the board
    QStringList parts = fen.split(" ");
    QString boardPart = parts[0];
    
    // Clear current board state
    board.clear();
    board.resize(8);
    for (int i = 0; i < 8; i++) {
        board[i].resize(8);
        for (int j = 0; j < 8; j++) {
            board[i][j] = '.';
        }
    }
    
    pieces.clear();
    
    // Parse board part of FEN
    int row = 7, col = 0;
    for (QChar c : boardPart) {
        if (c == '/') {
            row--;
            col = 0;
        } else if (c.isDigit()) {
            col += c.digitValue();
        } else {
            if (row >= 0 && row < 8 && col >= 0 && col < 8) {
                board[row][col] = c.toLatin1();
                
                Piece p;
                p.type = c.toLatin1();
                p.x = col;
                p.y = row;
                pieces.append(p);
            }
            col++;
        }
    }
    
    update();
}

void ChessBoard::setPossibleMoves(const QStringList& moves) {
    possibleMoves = moves;
    update();
}

void ChessBoard::setRecommendedMoves(const QStringList& moves) {
    recommendedMoves = moves;
    update();
}

void ChessBoard::clearRecommendedMoves() {
    recommendedMoves.clear();
    update();
}

void ChessBoard::setRotated(bool rotated) {
    isRotated = rotated;
    update();
}

void ChessBoard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Draw board background
    painter.fillRect(rect(), QColor(240, 217, 181));
    
    // Draw squares
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            QRect square = squareRect(x, y);
            
            // Determine square color
            if ((x + y) % 2 == 1) {
                painter.fillRect(square, QColor(181, 136, 99));
            }
            
            // Highlight selected square
            if (selectedSquare.x() == x && selectedSquare.y() == y) {
                painter.fillRect(square, QColor(106, 168, 79, 180));
            }
            
            // Highlight last move
            QString from = squareToAlgebraic(selectedSquare.x(), selectedSquare.y());
            if (!from.isEmpty()) {
                for (const QString& move : possibleMoves) {
                    if (move.startsWith(from) && move.length() >= 4) {
                        QString to = move.mid(2, 2);
                        QPair<int, int> toSquare = algebraicToSquare(to);
                        if (toSquare.first == x && toSquare.second == y) {
                            painter.fillRect(square, QColor(106, 168, 79, 120));
                        }
                    }
                }
            }
            
            // Highlight recommended moves
            for (const QString& move : recommendedMoves) {
                if (move.length() >= 4) {
                    QString to = move.mid(2, 2);
                    QPair<int, int> toSquare = algebraicToSquare(to);
                    if (toSquare.first == x && toSquare.second == y) {
                        painter.fillRect(square, QColor(65, 105, 225, 120));
                    }
                }
            }
        }
    }
    
    // Draw coordinates
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            QRect square = squareRect(x, y);
            
            // Draw file coordinates (a-h) on the bottom rank
            if (y == 0) {
                QChar file = 'a' + x;
                if (isRotated) {
                    file = 'h' - x;
                }
                painter.drawText(square.adjusted(2, square.height() - 16, 0, 0), Qt::AlignLeft, QString(file));
            }
            
            // Draw rank coordinates (1-8) on the left file
            if (x == 0) {
                QChar rank = '1' + y;
                if (isRotated) {
                    rank = '8' - y;
                }
                painter.drawText(square.adjusted(2, 2, 0, 0), Qt::AlignLeft, QString(rank));
            }
        }
    }
    
    // Draw pieces
    for (const Piece& p : pieces) {
        // Skip the dragged piece
        if (draggedPiece.x() == p.x && draggedPiece.y() == p.y) {
            continue;
        }
        
        QRect square = squareRect(p.x, p.y);
        QPixmap pixmap = pieceImages.value(p.type);
        
        if (!pixmap.isNull()) {
            painter.drawPixmap(square, pixmap);
        }
    }
    
    // Draw dragged piece
    if (draggedPiece.x() >= 0 && draggedPiece.y() >= 0) {
        int squareSize = qMin(width(), height()) / 8;
        QRect pieceRect(dragPosition.x() - squareSize / 2, 
                       dragPosition.y() - squareSize / 2,
                       squareSize, squareSize);
        
        for (const Piece& p : pieces) {
            if (p.x == draggedPiece.x() && p.y == draggedPiece.y()) {
                QPixmap pixmap = pieceImages.value(p.type);
                if (!pixmap.isNull()) {
                    painter.drawPixmap(pieceRect, pixmap);
                }
                break;
            }
        }
    }
}

void ChessBoard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();
        QPair<int, int> square = squareAtPosition(pos);
        
        if (square.first >= 0 && square.first < 8 && square.second >= 0 && square.second < 8) {
            // Check if there's a piece on the square
            char piece = board[square.second][square.first];
            
            if (piece != '.') {
                // Start dragging the piece
                draggedPiece = QPoint(square.first, square.second);
                dragPosition = pos;
                selectedSquare = QPoint(square.first, square.second);
                update();
            } else {
                // Click on empty square - deselect
                if (selectedSquare.x() >= 0) {
                    // If a piece was already selected, try to move it here
                    QString from = squareToAlgebraic(selectedSquare.x(), selectedSquare.y());
                    QString to = squareToAlgebraic(square.first, square.second);
                    
                    if (isValidMove(from, to)) {
                        emit moveMade(from, to);
                    }
                    
                    selectedSquare = QPoint(-1, -1);
                    update();
                }
            }
        }
    }
}

void ChessBoard::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && draggedPiece.x() >= 0) {
        QPoint pos = event->pos();
        QPair<int, int> targetSquare = squareAtPosition(pos);
        
        if (targetSquare.first >= 0 && targetSquare.first < 8 && 
            targetSquare.second >= 0 && targetSquare.second < 8) {
            
            // Check if this is a valid move
            QString from = squareToAlgebraic(draggedPiece.x(), draggedPiece.y());
            QString to = squareToAlgebraic(targetSquare.first, targetSquare.second);
            
            if (isValidMove(from, to)) {
                emit moveMade(from, to);
            }
        }
        
        // Stop dragging
        draggedPiece = QPoint(-1, -1);
        selectedSquare = QPoint(-1, -1);
        update();
    }
}

void ChessBoard::mouseMoveEvent(QMouseEvent* event) {
    if (draggedPiece.x() >= 0) {
        dragPosition = event->pos();
        update();
    }
}

void ChessBoard::loadPieceImages() {
    // Load SVG images for all pieces
    pieceImages['P'] = QPixmap(":/Resources/pieces/white_pawn.svg");
    pieceImages['N'] = QPixmap(":/Resources/pieces/white_knight.svg");
    pieceImages['B'] = QPixmap(":/Resources/pieces/white_bishop.svg");
    pieceImages['R'] = QPixmap(":/Resources/pieces/white_rook.svg");
    pieceImages['Q'] = QPixmap(":/Resources/pieces/white_queen.svg");
    pieceImages['K'] = QPixmap(":/Resources/pieces/white_king.svg");
    
    pieceImages['p'] = QPixmap(":/Resources/pieces/black_pawn.svg");
    pieceImages['n'] = QPixmap(":/Resources/pieces/black_knight.svg");
    pieceImages['b'] = QPixmap(":/Resources/pieces/black_bishop.svg");
    pieceImages['r'] = QPixmap(":/Resources/pieces/black_rook.svg");
    pieceImages['q'] = QPixmap(":/Resources/pieces/black_queen.svg");
    pieceImages['k'] = QPixmap(":/Resources/pieces/black_king.svg");
}

QRect ChessBoard::squareRect(int x, int y) const {
    int boardSize = qMin(width(), height());
    int squareSize = boardSize / 8;
    
    // Adjust for rotation if needed
    int displayX = isRotated ? (7 - x) : x;
    int displayY = isRotated ? (7 - y) : y;
    
    return QRect(displayX * squareSize, (7 - displayY) * squareSize, squareSize, squareSize);
}

QPair<int, int> ChessBoard::squareAtPosition(const QPoint& pos) const {
    int boardSize = qMin(width(), height());
    int squareSize = boardSize / 8;
    
    int x = pos.x() / squareSize;
    int y = 7 - (pos.y() / squareSize);
    
    // Adjust for rotation
    if (isRotated) {
        x = 7 - x;
        y = 7 - y;
    }
    
    return qMakePair(x, y);
}

QString ChessBoard::squareToAlgebraic(int x, int y) const {
    if (x < 0 || x > 7 || y < 0 || y > 7) {
        return QString();
    }
    
    QChar file = 'a' + x;
    QChar rank = '1' + y;
    
    return QString(file) + rank;
}

QPair<int, int> ChessBoard::algebraicToSquare(const QString& algebraic) const {
    if (algebraic.length() != 2) {
        return qMakePair(-1, -1);
    }
    
    int x = algebraic[0].toLatin1() - 'a';
    int y = algebraic[1].toLatin1() - '1';
    
    if (x < 0 || x > 7 || y < 0 || y > 7) {
        return qMakePair(-1, -1);
    }
    
    return qMakePair(x, y);
}

bool ChessBoard::isValidMove(const QString& from, const QString& to) const {
    QString move = from + to;
    return possibleMoves.contains(move);
}