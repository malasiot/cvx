#pragma once

#include <cvx/util/misc/xml_pull_parser.hpp>
#include "dem.hpp"

using cvx::util::Dictionary ;

class OSMFeature {
public:
    std::string id_ ;
    Dictionary tags_ ;

    OSMFeature() = default ;
    OSMFeature(const std::string &id): id_(id) {}

    virtual ~OSMFeature() {}
};

class OSMNode: public OSMFeature {
public:
    OSMNode() = default ;

    OSMNode(const std::string &id, double lat, double lon):
        OSMFeature(id), lat_(lat), lon_(lon) {}

    double lat_, lon_, ele_ ;
};

struct Coord {
    Coord(double lat, double lon, double ele): lat_(lat), lon_(lon), ele_(ele) {}

    double lat_, lon_, ele_ ;
};

class OSMWay: public OSMFeature {
public:
    OSMWay(const std::string &id):  OSMFeature(id) {}

    void computeElevationGainAndLoss(const std::vector<double> &prof);

    std::vector<std::string> nodes_ ;
    double length_, alt_gain_, alt_loss_ ;

};

class OSMDocument {
public:

    void fillInElevationsFromDEM(DEM &dem) ;

    std::vector<Coord> getLineString(const OSMWay &way) const;

    std::map<std::string, OSMNode> nodes_ ;
    std::vector<OSMWay> ways_ ;

};

class OSMParser {

public:

    using Filter = std::function<bool(const Dictionary &)> ;

    bool parse(std::istream &strm, OSMDocument &doc, Filter nodeFilter = nullptr, Filter wayFilter = nullptr) ;
};
