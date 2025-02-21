// src/core/Position.h
#pragma once
#include <string>
#include <QString>
#include <QtCore/QPoint>
#include <QDataStream>

class Position {
public:
    int row;
    int col;
    
    // Constructors
    Position() : row(0), col(0) {}
    Position(int r, int c) : row(r), col(c) {}
    
    // Copy constructor and assignment operator
    Position(const Position& other) = default;
    Position& operator=(const Position& other) = default;

    // Add conversion constructor from QPoint
    Position(const QPoint& point) : row(point.y()), col(point.x()) {}

    // Add conversion operator to QPoint
    operator QPoint() const { return QPoint(col, row); }
    
    // Comparison operators
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
    
    bool operator<(const Position& other) const {
        return row < other.row || (row == other.row && col < other.col);
    }
    
    // Validation
    bool isValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
    
    // Chess notation conversion
    QString toAlgebraic() const {
        if (!isValid()) return QString();
        return QString(QChar('a' + col)) + QString::number(8 - row);
    }
    
    static Position fromAlgebraic(const QString& algebraic) {
        if (algebraic.length() != 2) return Position(-1, -1);
        int col = algebraic[0].toLatin1() - 'a';
        int row = 8 - (algebraic[1].digitValue());
        return Position(row, col);
    }
    
    // Helper methods for chess moves
    int fileDistance(const Position& other) const {
        return std::abs(col - other.col);
    }
    
    int rankDistance(const Position& other) const {
        return std::abs(row - other.row);
    }
    
    bool isDiagonal(const Position& other) const {
        return fileDistance(other) == rankDistance(other);
    }
    
    bool isKnightMove(const Position& other) const {
        int fileDist = fileDistance(other);
        int rankDist = rankDistance(other);
        return (fileDist == 2 && rankDist == 1) || 
               (fileDist == 1 && rankDist == 2);
    }
    
    // Get positions between this and other (exclusive)
    std::vector<Position> getPositionsBetween(const Position& other) const {
        std::vector<Position> positions;
        int rowStep = (other.row > row) ? 1 : (other.row < row) ? -1 : 0;
        int colStep = (other.col > col) ? 1 : (other.col < col) ? -1 : 0;
        
        int currentRow = row + rowStep;
        int currentCol = col + colStep;
        
        while (currentRow != other.row || currentCol != other.col) {
            positions.emplace_back(currentRow, currentCol);
            currentRow += rowStep;
            currentCol += colStep;
        }
        
        return positions;
    }
    
    // Serialization support for Qt
    friend QDataStream& operator<<(QDataStream& stream, const Position& pos) {
        return stream << pos.row << pos.col;
    }
    
    friend QDataStream& operator>>(QDataStream& stream, Position& pos) {
        return stream >> pos.row >> pos.col;
    }
    
    // String representation
    QString toString() const {
        return QString("(%1, %2)").arg(row).arg(col);
    }
};

// Hash function for Position (useful for std::unordered_map/set)
namespace std {
    template<>
    struct hash<Position> {
        size_t operator()(const Position& pos) const {
            return (pos.row << 3) | pos.col;
        }
    };
}