#ifndef GRAPH_H
#define GRAPH_H

#include "osm_parser.hpp"
#include <cvx/db/connection.hpp>

class Graph
{
public:
    Graph(const OSMDocument &doc);
    void computeEdgeStats(DEM &dem);
    void exportToOSM(const std::string &path) ;
    void write(const std::string &path) ;
private:

    void writeNodes(cvx::db::Connection &con) ;
    void writeEdges(cvx::db::Connection &con) ;

    struct Node {
        Node(const OSMNode &node): id_(node.id_), lat_(node.lat_), lon_(node.lon_), ele_(node.ele_), tags_(node.tags_) {}

        double lat_, lon_, ele_ ;
        std::string id_ ;
        cvx::util::Dictionary tags_ ;
    } ;

    struct Edge {
        std::string start_, stop_ ;
        double length_, gain_, loss_ ;
        std::vector<Coord> coords_ ;
        std::vector<std::string> nodes_ ;
        cvx::util::Dictionary tags_ ;

    public:
        void computeElevationGainAndLoss(const std::vector<double> &prof);
    };

    std::map<std::string, Node> nodes_ ;
    std::vector<Edge> edges_ ;
};

#endif // GRAPH_H
