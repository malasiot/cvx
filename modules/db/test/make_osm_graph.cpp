#include "osm_parser.hpp"

#include <fstream>

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
}
