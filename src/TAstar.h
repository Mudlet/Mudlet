#ifndef MUDLET_TASTAR_H
#define MUDLET_TASTAR_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2020, 2022 by Stephen Lyons                       *
 *                                               - slysven@virginmedia.com *
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

#include "TRoom.h"

#ifndef Q_MOC_RUN
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/astar_search.hpp>
#include <boost/graph/graphviz.hpp>
#endif

#include <QDebug>
#include <QString>

#include <algorithm>
#include <cmath>

class TRoom;

using namespace boost;


// auxiliary types
struct location
{
    int id;
    // Copied rather than read through pR: TMap::initGraph() scales the heuristic to these, and a
    // room moved after it would otherwise let the heuristic overestimate.
    int x;
    int y;
    int z;
    TRoom* pR;
};

// double, not float: weights go up to INT_MAX and a route sums several of them, all exactly.
typedef double cost;

// The least an edge costs per coordinate unit it spans, over every edge, so distance times it never
// overestimates a route. Zero (Dijkstra) when an edge spans distance for free or none spans any.
struct heuristicScale
{
    cost chebyshev = 0;
    cost euclidean = 0;
};

// Shared by the heuristic and the scale it is held to: measured differently, it could overestimate.
struct coordinateSpan
{
    cost chebyshev;
    cost euclidean;
};

inline coordinateSpan spanBetween(const location& a, const location& b)
{
    // Subtracted as cost: two coordinates anywhere in the int range can differ by more than an int holds.
    const cost dx = std::abs(static_cast<cost>(a.x) - b.x);
    const cost dy = std::abs(static_cast<cost>(a.y) - b.y);
    const cost dz = std::abs(static_cast<cost>(a.z) - b.z);
    return {std::max({dx, dy, dz}), std::sqrt(dx * dx + dy * dy + dz * dz)};
}

// Used to record edge details and to deduplicate parallel ones:
struct route
{
    int cost;                // Needed during establishing the best parallel edge
    quint8 direction;        // Use DIR_xxx values to code exit direction
    QString specialExitName; // If direction is DIR_OTHER then this is needed
};

// Admissible and consistent: each distance is scaled by its heuristicScale, and the larger kept.
// Chebyshev is exact on a grid with diagonal exits, euclidean on one without.
template <class Graph, class CostType, class LocMap>
class distance_heuristic : public boost::astar_heuristic<Graph, CostType>
{
public:
    typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
    // Held by reference: a per-search copy (55MB on a 2.3M-room map) costs more than the search.
    // Callers pass TMap::locations, which initGraph() only clear()s: it can't dangle, but goes stale
    // across initGraph() (rooms renumbered), so never store a heuristic.
    distance_heuristic(const LocMap& l, Vertex goal, const heuristicScale& scale)
    : m_location(l)
    , m_goal(goal)
    , m_scale(scale)
    {
    }

    // A temporary would leave m_location dangling.
    distance_heuristic(const LocMap&&, Vertex, const heuristicScale&) = delete;

    CostType operator()(Vertex u)
    {
        const coordinateSpan toGoal = spanBetween(m_location[m_goal], m_location[u]);
        return std::max(toGoal.chebyshev * m_scale.chebyshev, toGoal.euclidean * m_scale.euclidean);
    }

private:
    const LocMap& m_location;
    Vertex m_goal;
    heuristicScale m_scale;
};


// exception for termination
struct found_goal
{
};

// visitor that terminates when we find the goal
template <class Vertex>
class astar_goal_visitor : public boost::default_astar_visitor
{
public:
    explicit astar_goal_visitor(Vertex goal)
    : m_goal(goal)
    {
    }

    template <class Graph>
    void examine_vertex(Vertex u, Graph& g)
    {
        Q_UNUSED(g)
        if (u == m_goal) {
            throw found_goal();
        }
    }

private:
    Vertex m_goal;
};

#endif // MUDLET_TASTAR_H
