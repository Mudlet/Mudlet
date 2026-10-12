#ifndef MUDLET_TAREAGRAPH_H
#define MUDLET_TAREAGRAPH_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QHash>
#include <QList>
#include <QMap>
#include <QPair>
#include <QPointF>
#include <QString>

class TMap;

/*
 * The areas of a map as a graph: a node per area, a link per pair of areas joined by at least one
 * normal or special exit, and a 2D world position per area from a deterministic force-directed
 * layout. Areas have no shared coordinates of their own, so this is what places them relative to
 * each other in a world view.
 *
 * A position is where the centre of the area's room bounding box goes, in room units. Positions
 * are kept in the map user data under cLayoutUserDataKey, so they travel with the map file; areas
 * that already have one keep it when others are added, and only setPosition() pins an area.
 *
 * Rebuilt lazily: refresh() compares the map's revision and generation with the ones it last saw.
 */
class TAreaGraph
{
public:
    static inline const QString cLayoutUserDataKey = QStringLiteral("system.areaGraphLayout");

    struct Node
    {
        int areaId = 0;
        QString name;
        int roomCount = 0;
        // Room coordinate extremes; all zero for an area with no rooms.
        int minX = 0;
        int maxX = 0;
        int minY = 0;
        int maxY = 0;
        int minZ = 0;
        int maxZ = 0;
        QPointF position;
        bool pinned = false;
    };

    struct Link
    {
        // fromArea < toArea; an exit counts once in whichever direction it runs.
        int fromArea = 0;
        int toArea = 0;
        int exitCount = 0;
        // (room the exit leaves, room it reaches), sorted; one entry per exit.
        QList<QPair<int, int>> roomPairs;
    };

    explicit TAreaGraph(TMap*);

    // Cheap when nothing changed since the last call.
    void refresh();
    const QMap<int, Node>& nodes() const { return mNodes; }
    // Sorted by (fromArea, toArea).
    const QList<Link>& links() const { return mLinks; }
    // Pins the area there. False if there is no such area or the point is not finite.
    bool setPosition(int areaId, const QPointF& position);
    // Discards the stored positions (but the pinned ones, if asked) and lays everything out again.
    void resetLayout(bool keepPinned);
    // For tests: how often the graph and the layout were recomputed.
    quint32 rebuildCount() const { return mRebuildCount; }
    quint32 layoutRunCount() const { return mLayoutRunCount; }

private:
    struct StoredPosition
    {
        QPointF position;
        bool pinned = false;
    };

    void rebuildGraph();
    void readStoredLayout();
    void writeStoredLayout();
    void layoutUnplacedAreas();
    void simulate(const QList<int>& component, const QHash<int, bool>& fixed, const QHash<int, QList<int>>& adjacency);
    double radiusOf(int areaId) const;

    TMap* mpMap = nullptr;
    QMap<int, Node> mNodes;
    QList<Link> mLinks;
    QHash<int, StoredPosition> mStored;
    // What the user data held when this last read or wrote it, to notice a load or a script changing it.
    QString mStoredLayoutText;
    bool mStoredLayoutRead = false;
    quint64 mSeenRevision = 0;
    unsigned int mSeenGeneration = 0;
    bool mBuilt = false;
    quint32 mRebuildCount = 0;
    quint32 mLayoutRunCount = 0;
};

#endif // MUDLET_TAREAGRAPH_H
