#pragma once

#include <cvx/util/misc/xml_pull_parser.hpp>

using cvx::util::Dictionary ;

class OSMFeature {
public:
    std::string id_ ;
    Dictionary tags_ ;

    OSMFeature(const std::string &id): id_(id) {}

    virtual ~OSMFeature() {}
};

class OSMNode: public OSMFeature {
public:
    OSMNode(const std::string &id, double lat, double lon):
        OSMFeature(id), lat_(lat), lon_(lon) {}

    double lat_, lon_ ;
};

class OSMWay: public OSMFeature {
public:
    OSMWay(const std::string &id):
        OSMFeature(id) {}

    std::vector<std::string> nodes_ ;
};

class OSMDocument {
public:
    std::map<std::string, OSMNode> nodes_ ;
    std::map<std::string, OSMWay> ways_ ;
};

class OSMParser {

public:

    using Filter = std::function<bool(const Dictionary &)> ;

    bool parse(std::istream &strm, OSMDocument &doc, Filter nodeFilter = nullptr, Filter wayFilter = nullptr) ;
};
