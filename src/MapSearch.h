#ifndef MUDLET_MAPSEARCH_H
#define MUDLET_MAPSEARCH_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QList>
#include <QString>

#include <optional>

class TMap;

// The names of every room, area and map label of the map, case folded once so that each search only compares
// them, made again only once the map has changed since
class MapSearch
{
public:
    enum class Kind { Area, Room, Label };
    // Best first
    enum class Match { Exact, Prefix, Substring };

    struct Result
    {
        Kind kind = Kind::Room;
        Match match = Match::Substring;
        // A room's or a label's ID, or the area's
        int id = 0;
        int areaId = 0;
        QString name;
    };

    static constexpr qsizetype scmMaxResults = 50;

    // Makes the index again if the map changed since it was made; true if it did
    bool refresh(const TMap& map);
    bool stale(const TMap& map) const;
    void clear();
    qsizetype size() const { return mEntries.size(); }

    // The best matches for the text, ignoring case, at most limit of them: exact names, and a room by its ID, then
    // names starting with the text, then names containing it; among those as good, the current area's first.
    // total, if given, is set to how many there were in all
    QList<Result> find(const QString& text, int currentAreaId, bool labels, qsizetype limit = scmMaxResults, qsizetype* total = nullptr) const;

private:
    struct Entry
    {
        Kind kind = Kind::Room;
        int id = 0;
        int areaId = 0;
        QString folded;
        QString name;
    };
    struct Inputs
    {
        unsigned int mapGeneration = 0;
        quint64 mapRevision = 0;
        size_t roomsVersions = 0;
        size_t labels = 0;
        bool operator==(const Inputs&) const = default;
    };
    static Inputs inputsOf(const TMap& map);

    std::optional<Inputs> mInputs;
    QList<Entry> mEntries;
};

#endif // MUDLET_MAPSEARCH_H
