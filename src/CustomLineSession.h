/***************************************************************************
*   Copyright (C) 2025 by Piotr Wilczynski - delwing@gmail.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include <optional>

#include <QList>
#include <QPointF>
#include <QString>

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

