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
 * are kept in the map user data under cLayoutUserDataKey, so they travel with the map file. Areas
 * that already have one keep it when others are added, moving only to make way for an area that
 * grew into them; setPosition() pins an area so that it never moves.
 *
 * Rebuilt lazily: refresh() compares the map's revision and generation with the ones it last saw.
 */
class TAreaGraph
{
public:
    static inline const QString cLayoutUserDataKey = QStringLiteral("system.areaGraphLayout");
    // Positions further out than this are refused, which keeps the layout's grid cell arithmetic in range;
    // computed positions are clamped to it.
    static constexpr double cMaxCoordinate = 1.0e15;

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
        // Half the diagonal of the bounding box, at least cMinRadius: how much room the area needs.
        double radius = 0.0;
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

    // Cheap when nothing changed since the last call; does nothing while a map is being loaded.
    void refresh();
    const QMap<int, Node>& nodes() const { return mNodes; }
    // Sorted by (fromArea, toArea).
    const QList<Link>& links() const { return mLinks; }
    // Pins the area there. False if there is no such area, a coordinate is not finite or beyond
    // cMaxCoordinate, or a map is being loaded.
    bool setPosition(int areaId, const QPointF& position);
    // Discards the stored positions (but the pinned ones, if asked) and lays everything out again.
    void resetLayout(bool keepPinned);
    // Drops a deleted area's stored position, which a later area given the same id would otherwise take.
    void forgetArea(int areaId);

private:
    static constexpr int cMobilityPinned = 0;
    static constexpr int cMobilitySettled = 1;
    static constexpr int cMobilityNew = 2;

    struct StoredPosition
    {
        QPointF position;
        bool pinned = false;
    };

    // True if an area was added or removed, changed size, or the links changed.
    bool rebuildGraph();
    void rebuildLinks();
    void readStoredLayout();
    void writeStoredLayout();
    // Places areas with no position, then pushes apart any that overlap.
    void layoutAreas();
    void simulate(const QList<int>& component, const QHash<int, bool>& fixed, const QHash<int, QList<int>>& adjacency);
    // Pushes apart overlapping areas. Of a pair, the one with the higher mobility moves, both move
    // if equal, and neither if both are 0; an area missing from mobility has cMobilityNew.
    bool removeOverlaps(const QList<int>& ids, const QHash<int, int>& mobility);
    void placeNewAreas(const QHash<int, bool>& fixed);
    // Shelf-packs components laid out on their own into rows below what is already placed.
    void packComponents(QList<QList<int>> freshComponents);
    double radiusOf(int areaId) const;

    TMap* mpMap = nullptr;
    QMap<int, Node> mNodes;
    QList<Link> mLinks;
    QHash<int, StoredPosition> mStored;
    // What the user data held when this last read or wrote it, to notice a load or a script changing it.
    QString mStoredLayoutText;
    // What the links were built from: the exits revision and every area's rooms version.
    quint64 mLinksSignature = 0;
    bool mLinksBuilt = false;
    bool mStoredDirty = true;
    bool mStoredLayoutRead = false;
    quint64 mSeenRevision = 0;
    unsigned int mSeenGeneration = 0;
    bool mBuilt = false;
};

#endif // MUDLET_TAREAGRAPH_H
