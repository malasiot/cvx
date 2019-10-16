#include "osm_parser.hpp"
#include "dem.hpp"
#include "graph.hpp"

#include <fstream>
#include <cmath>

using namespace std ;


int main(int argc, char *argv[]) {

    ifstream strm("/home/malasiot/Downloads/athos.osm") ;

    OSMParser parser ;
    OSMDocument doc ;

    parser.parse(strm, doc, nullptr,
                 [](const Dictionary &tags)->bool {
        if ( tags.contains("highway")) return true ;
        return false ;
    }) ;

    DEM dem("/home/malasiot/source/Android/AthosGuide/app/scripts/dems/") ;

    doc.fillInElevationsFromDEM(dem) ;

    Graph graph(doc) ;
    graph.computeEdgeStats(dem) ;
    graph.exportToOSM("/home/malasiot/tmp/graph.osm") ;
    graph.write("/home/malasiot/tmp/athos.sqlite");


}
