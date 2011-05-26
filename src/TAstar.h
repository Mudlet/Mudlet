#ifndef TASTAR_H
#define TASTAR_H


#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <boost/random.hpp>
#include <boost/graph/graphviz.hpp>
//#include <sys/time.h>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <math.h>    // for sqrt

using namespace boost;
using namespace std;


// auxiliary types
struct location
{
  float y, x, z; // lat, long
};
typedef float cost;

template <class Name, class LocMap>
class city_writer {
public:
  city_writer(Name n, LocMap l, float _minx, float _maxx, float _minz,
              float _miny, float _maxy, float _maxz,
              unsigned int _ptx, unsigned int _pty, unsigned int _ptz )
    : name(n), loc(l), minx(_minx), maxx(_maxx), miny(_miny),
      maxy(_maxy), minz(_minz), maxz(_maxz), ptx(_ptx), pty(_pty), ptz(_ptz) {}
  template <class Vertex>
  void operator()(ostream& out, const Vertex& v) const {
    float px = 1 - (loc[v].x - minx) / (maxx - minx);
    float py = (loc[v].y - miny) / (maxy - miny);
    float pz = (loc[v].z - minz) / (maxz - minz);
    out << "[label=\"" << name[v] << "\", pos=\""
        << static_cast<unsigned int>(ptx * px) << ","
        << static_cast<unsigned int>(pty * py) << ","
        << static_cast<unsigned int>(ptz * pz)
        << "\", fontsize=\"11\"]";
  }
private:
  Name name;
  LocMap loc;
  float minx, maxx, miny, maxy, minz, maxz;
  unsigned int ptx, pty, ptz;
};

template <class WeightMap>
class time_writer {
public:
  time_writer(WeightMap w) : wm(w) {}
  template <class Edge>
  void operator()(ostream &out, const Edge& e) const {
    out << "[label=\"" << wm[e] << "\", fontsize=\"11\"]";
  }
private:
  WeightMap wm;
};


// euclidean distance heuristic
template <class Graph, class CostType, class LocMap>
class distance_heuristic : public astar_heuristic<Graph, CostType>
{
public:
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  distance_heuristic(LocMap l, Vertex goal)
    : m_location(l), m_goal(goal) {}
  CostType operator()(Vertex u)
  {
    //    CostType dx = m_location[m_goal].x - m_location[u].x;
    //    CostType dy = m_location[m_goal].y - m_location[u].y;
    //    CostType dz = m_location[m_goal].z - m_location[u].z;
    return 1;
    //return ::sqrt(dx * dx + dy * dy + dz * dz);
  }
private:
  LocMap m_location;
  Vertex m_goal;
};


struct found_goal {}; // exception for termination

// visitor that terminates when we find the goal
template <class Vertex>
class astar_goal_visitor : public boost::default_astar_visitor
{
public:
  astar_goal_visitor(Vertex goal) : m_goal(goal) {}
  template <class Graph>
  void examine_vertex(Vertex u, Graph& g) {
    if(u == m_goal)
      throw found_goal();
  }
private:
  Vertex m_goal;
};

#endif // TASTAR_H
