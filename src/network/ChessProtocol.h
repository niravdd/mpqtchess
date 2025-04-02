// src/network/ChessProtocol.h
#pragma once
#include "../QtCompat.h"
#include <QtCore/QDataStream>
#include "../core/ChessGame.h"
#include "../core/Position.h"

enum class MessageType {
    CONNECT_REQUEST,
    CONNECT_RESPONSE,
    GAME_STATE,
    MOVE,
    MOVE_RESPONSE,
    DRAW_OFFER,
    DRAW_RESPONSE,
    RESIGN,
    CHAT,
    ERROR,
    GAME_END,
    KEEPALIVE
};

struct MoveData {
    Position from;
    Position to;
    PieceType promotionPiece;
};

struct NetworkMessage {
    MessageType type;
    bool success{false};
    QString data;
    QString extraData;
    MoveData moveData;
};

/*
// Serialization operators
inline QDataStream& operator<<(QDataStream& stream, const Position& pos) {
    return stream << pos.row << pos.col;
}

inline QDataStream& operator>>(QDataStream& stream, Position& pos) {
    return stream >> pos.row >> pos.col;
}
*/

inline QDataStream& operator<<(QDataStream& stream, const MoveData& moveData) {
    return stream << moveData.from << moveData.to 
                 << static_cast<qint32>(moveData.promotionPiece);
}

inline QDataStream& operator>>(QDataStream& stream, MoveData& moveData) {
    qint32 piece;
    stream >> moveData.from >> moveData.to >> piece;
    moveData.promotionPiece = static_cast<PieceType>(piece);
    return stream;
}

inline QDataStream& operator<<(QDataStream& stream, const NetworkMessage& msg) {
    return stream << static_cast<qint32>(msg.type)
                 << msg.success
                 << msg.data
                 << msg.extraData
                 << msg.moveData;
}

inline QDataStream& operator>>(QDataStream& stream, NetworkMessage& msg) {
    qint32 type;
    stream >> type >> msg.success >> msg.data >> msg.extraData >> msg.moveData;
    msg.type = static_cast<MessageType>(type);
    return stream;
}