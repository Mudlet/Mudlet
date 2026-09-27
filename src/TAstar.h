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

#include <math.h> // for sqrt

class TRoom;

using namespace boost;


// auxiliary types
struct location
{
    int id;    // Typically 4 bytes
    TRoom* pR; // 4 or 8 bytes? - so may have reduced size from 20 to 8 or 12 plus padding...?
    float areaChebyshevScale = 0; // EXPERIMENT
};

#ifdef MUDLET_EXPERIMENT_DOUBLE_COST
typedef double cost;
#else
typedef float cost;
#endif

// EXPERIMENT (#3334): 0 = euclidean (current), 1 = zero (Dijkstra), 2 = euclidean scaled by gHeuristicScale, 0 across areas
inline int gHeuristicMode = 0;
inline float gHeuristicScale = 1.0f;
// 3 = chebyshev scaled by gChebyshevScale (global), 4 = chebyshev scaled per area; both 0 across areas
inline float gChebyshevScale = 1.0f;
inline bool gRecordF = false;
inline std::vector<float> gExpandedF;
inline double gScalePassMs = 0;
inline double gChebPassMs = 0;
// 6 = ALT (landmarks + triangle inequality) with tie-break on h, 7 = ALT without tie-break.
// Landmark tables are vertex-major: [v * gAltK + k].
inline int gAltK = 0;
inline bool gAltUseTo = true;
inline const cost* gAltFrom = nullptr; // d(L_k, v)
inline const cost* gAltTo = nullptr;   // d(v, L_k)
inline cost gAltGoalFrom[64];
inline cost gAltGoalTo[64];
inline double gAltPassMs = 0;
inline std::size_t gAltBytes = 0;
inline int gAltLandmarksBuilt = 0;
inline qint64 gAltLargestScc = 0;
inline qint64 gAltComponents = 0;
inline qint64 gAltComponentsWithLandmarks = 0;
inline const qint32* gAltScc = nullptr;
inline qint32 gAltGoalScc = -1;
inline std::vector<int> gAltLandmarkRooms;

// Used to record edge details and to deduplicate parallel ones:
struct route
{
#ifdef MUDLET_EXPERIMENT_DOUBLE_COST
    double cost;
#else
    float cost;              // Needed during establishing the best parallel edge
#endif
    quint8 direction;        // Use DIR_xxx values to code exit direction
    QString specialExitName; // If direction is DIR_OTHER then this is needed
};

// euclidean distance heuristic
template <class Graph, class CostType, class LocMap>
class distance_heuristic : public boost::astar_heuristic<Graph, CostType>
{
public:
    typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
    // Held by reference: a per-search copy (35MB on a 2.3M-room map) costs more than the search.
    // Callers pass TMap::locations, which initGraph() only clear()s: it can't dangle, but goes stale
    // across initGraph() (rooms renumbered), so never store a heuristic.
    distance_heuristic(const LocMap& l, Vertex goal)
    : m_location(l)
    , m_goal(goal)
    {}

    // A temporary would leave m_location dangling.
    distance_heuristic(const LocMap&&, Vertex) = delete;

    CostType operator()(Vertex u)
    {
        if (gHeuristicMode == 1) {
            return 0;
        }
        if (gHeuristicMode == 6 || gHeuristicMode == 7) {
            // slots name the room's own component's landmarks, so they only compare with the goal's
            if (gAltK == 0 || !gAltFrom || !gAltScc || gAltScc[u] != gAltGoalScc) {
                return 0;
            }
            constexpr cost inf = std::numeric_limits<cost>::infinity();
            const cost* fromLandmark = gAltFrom + static_cast<std::size_t>(u) * gAltK;
            const cost* toLandmark = gAltUseTo ? gAltTo + static_cast<std::size_t>(u) * gAltK : nullptr;
            CostType best = 0;
            for (int k = 0; k < gAltK; ++k) {
                // d(u, g) >= d(L, g) - d(L, u), valid only when L reaches u
                if (fromLandmark[k] < inf && gAltGoalFrom[k] < inf) {
                    best = std::max(best, gAltGoalFrom[k] - fromLandmark[k]);
                }
                // d(u, g) >= d(u, L) - d(g, L), valid only when g reaches L
                if (toLandmark && toLandmark[k] < inf && gAltGoalTo[k] < inf) {
                    best = std::max(best, toLandmark[k] - gAltGoalTo[k]);
                }
            }
            return best;
        }
        if (m_location[m_goal].pR->getArea() != m_location[u].pR->getArea()) {
            return gHeuristicMode >= 2 ? 0 : 1;
        }
        CostType dx = m_location[m_goal].pR->x() - m_location[u].pR->x();
        CostType dy = m_location[m_goal].pR->y() - m_location[u].pR->y();
        CostType dz = m_location[m_goal].pR->z() - m_location[u].pR->z();

        if (gHeuristicMode == 3 || gHeuristicMode == 4) {
            const CostType cheb = std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
            return cheb * (gHeuristicMode == 3 ? gChebyshevScale : m_location[u].areaChebyshevScale);
        }
        if (gHeuristicMode == 5) {
            const CostType cheb = std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
            return cheb * m_location[u].areaChebyshevScale;
        }
        const CostType d = std::sqrt(dx * dx + dy * dy + dz * dz);
        return gHeuristicMode == 2 ? d * gHeuristicScale : d;
    }

private:
    const LocMap& m_location;
    Vertex m_goal;
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
    {}

    template <class Graph>
    void examine_vertex(Vertex u, Graph& g) {
        Q_UNUSED(g)
        if (u == m_goal) {
            throw found_goal();
        }
    }

private:
    Vertex m_goal;
};

#endif // MUDLET_TASTAR_H
