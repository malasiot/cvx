#ifndef POIWRITER_HPP
#define POIWRITER_HPP

#include "osm_parser.hpp"
#include "poi_config.hpp"

#include <cvx/db/connection.hpp>
#include <cvx/util/misc/variant.hpp>

class POIWriter {
public:
    POIWriter(const OSMDocument &doc): doc_(doc) {}

    void write(const std::string &dbPath, const POIConfig &cfg) ;

private:

    void writeNodes(cvx::db::Connection &con, const POIConfig &cfg) ;
    void makeNames(const std::string &lang, cvx::db::Connection &con, const POIConfig &cfg);
    std::string getPOIName(const OSMNode &node, const std::string &lang) ;
    std::string getPOItype(const OSMNode &node, const POIConfig &cfg);

private:
    const OSMDocument &doc_ ;

};

#endif // POIWRITER_HPP
