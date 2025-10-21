#pragma once

#include <optional>

#include "pre_guard.h"
#include <QList>
#include <QPointF>
#include <QString>
#include "post_guard.h"

class T2DMap;
class TRoom;

class CustomLineSession
{
public:
    explicit CustomLineSession(T2DMap& mapWidget);

    bool isSnapToGridEnabled() const;
    void setSnapToGridEnabled(bool enabled);

    QPointF snapPointToGrid(const QPointF& point) const;

    bool canMoveSelectedCustomLineLastPointToTargetRoom() const;
    bool canMoveCustomLineLastPointToTargetRoom(const TRoom& room, const QString& exitKey) const;
    void moveCustomLineLastPointToTargetRoom();

    void clearOriginalPoints();

private:
    struct LineKey
    {
        int roomId = 0;
        QString exitKey;

        bool isValid() const { return roomId > 0 && !exitKey.isEmpty(); }
        bool operator==(const LineKey& other) const { return roomId == other.roomId && exitKey == other.exitKey; }
    };

    struct OriginalLine
    {
        LineKey key;
        QList<QPointF> points;
    };

    std::optional<LineKey> currentLineKey() const;
    QList<QPointF>* resolveLinePoints(const LineKey& key, TRoom*& room) const;

    void rememberOriginalLine(const LineKey& key, const QList<QPointF>& points);
    void snapLineToGrid(LineKey key, QList<QPointF>& points, TRoom& room);
    void restoreOriginalLineIfNeeded();

    std::optional<int> resolveCustomLineTargetRoomId(const TRoom& room, const QString& exitKey) const;

    T2DMap& mMapWidget;
    bool mSnapToGridEnabled = false;
    std::optional<OriginalLine> mOriginalLine;
};

