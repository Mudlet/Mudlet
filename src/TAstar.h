#ifndef MUDLET_TASTAR_H
#define MUDLET_TASTAR_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "pre_guard.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/astar_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/random.hpp>
#include "post_guard.h"
#endif

#include "pre_guard.h"
#include <QDebug>
#include <QString>
#include "post_guard.h"

#include <math.h> // for sqrt

class TRoom;

using namespace boost;
using namespace std;


// auxiliary types
struct location
{
    int id;    // Typically 4 bytes
    TRoom* pR; // 4 or 8 bytes? - so may have reduced size from 20 to 8 or 12 plus padding...?
};

typedef float cost;

// Used to record edge details and to deduplicate parallel ones:
struct route
{
    float cost;              // Needed during establishing the best parallel edge
    quint8 direction;        // Use DIR_xxx values to code exit direction
    QString specialExitName; // If direction is DIR_OTHER then this is needed
};

// euclidean distance heuristic
template <class Graph, class CostType, class LocMap>
class distance_heuristic : public boost::astar_heuristic<Graph, CostType>
{
public:
    typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
    distance_heuristic(LocMap l, Vertex goal) : m_location(l), m_goal(goal) {}

    CostType operator()(Vertex u)
    {
        if (m_location[m_goal].pR->getArea() != m_location[u].pR->getArea()) {
            return 1;
        }
        CostType dx = m_location[m_goal].pR->x - m_location[u].pR->x;
        CostType dy = m_location[m_goal].pR->y - m_location[u].pR->y;
        CostType dz = m_location[m_goal].pR->z - m_location[u].pR->z;

        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

private:
    LocMap m_location;
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
    astar_goal_visitor(Vertex goal) : m_goal(goal) {}

    template <class Graph>
    void examine_vertex(Vertex u, Graph& g)
    {
        if (u == m_goal) {
            throw found_goal();
        }
    }

private:
    Vertex m_goal;
};

#endif // MUDLET_TASTAR_H
