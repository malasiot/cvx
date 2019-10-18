#include "osm_parser.hpp"
#include "poi_config.hpp"
#include "poi_writer.hpp"

#include <fstream>
#include <cmath>

using namespace std ;


int main(int argc, char *argv[]) {
    ifstream strm("/home/malasiot/Downloads/athos.osm") ;

    OSMParser parser ;
    OSMDocument doc ;

    parser.parse(strm, doc, [](const Dictionary &tags)->bool {
        if ( tags.empty() ) return false ;
        return true ;
    },
                 [](const Dictionary &)->bool {
          return false ;
    }) ;

    DEM dem("/home/malasiot/source/Android/AthosGuide/app/scripts/dems/") ;

    doc.fillInElevationsFromDEM(dem) ;

    POIConfig cfg ;
    cfg.parse("/home/malasiot/source/Android/AthosGuide/app/scripts/poi_categories.json") ;

    POIWriter writer(doc) ;
    writer.write("/home/malasiot/tmp/pois.sqlite", cfg) ;
}
