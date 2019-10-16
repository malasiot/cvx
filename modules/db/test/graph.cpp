#include "graph.hpp"

#include <cmath>
#include <fstream>

#include <cvx/util/misc/xml_writer.hpp>
#include <cvx/util/misc/strings.hpp>

using namespace std ;
using namespace cvx::util ;

Graph::Graph(const OSMDocument &doc) {
    // make nodes

    map<string, vector<const OSMWay *>> nodeMap ;

    for (const auto &way: doc.ways_ ) {
        string first = way.nodes_.front() ;
        string last = way.nodes_.back() ;

        auto it = nodes_.find(first) ;
        if ( it == nodes_.end() ) {
            auto oit = doc.nodes_.find(first) ;
            if ( oit != doc.nodes_.end() )
                nodes_.emplace(first, Node((*oit).second)) ;
        }

        it = nodes_.find(last) ;
        if ( it == nodes_.end() ) {
            auto oit = doc.nodes_.find(last) ;
            if ( oit != doc.nodes_.end() )
                nodes_.emplace(last, Node((*oit).second)) ;
        }

        for( const auto &ref: way.nodes_ ) {
            nodeMap[ref].push_back(&way) ;
        }
    }

    // check intermediate nodes that require breaking ways

    for( const auto &lp: nodeMap ) {
        string node_id = lp.first ;
        const auto &ways = lp.second ;
        if ( ways.size() > 1 ) {
            auto it = doc.nodes_.find(node_id) ;
            if ( it != doc.nodes_.end() )
                nodes_.emplace(node_id, Node((*it).second)) ;
        }
    }

    for (const auto &way: doc.ways_ ) {
        string first = way.nodes_.front() ;
        string last = way.nodes_.back() ;

        auto coords = doc.getLineString(way) ;

        string start = first ;
        Edge edge ;
        edge.start_ = start ;
        edge.coords_.push_back(coords[0]) ;
        edge.nodes_.push_back(first) ;
        edge.tags_ = way.tags_ ;

        for( int i=1 ; i<way.nodes_.size() ; i++ ) {
            const string &current = way.nodes_[i] ;
            edge.coords_.push_back(coords[i]) ;
            edge.nodes_.push_back(way.nodes_[i]) ;
            auto it = nodes_.find(current) ;
            if ( it != nodes_.end() ) { // we need to break
                edge.stop_ = current ;
                edges_.emplace_back(std::move(edge)) ;
                edge.start_ = current ;
                edge.coords_.push_back(coords[i]) ;
                edge.nodes_.push_back(way.nodes_[i]) ;
                edge.tags_ = way.tags_ ;
            }
        }
    }

}



#define R 6378100
#define TO_RAD (3.1415926536 / 180)

static double haversine(double th1, double ph1, double th2, double ph2)
{
    double dx, dy, dz;
    ph1 -= ph2;
    ph1 *= TO_RAD ;
    th1 *= TO_RAD ;
    th2 *= TO_RAD;

    dz = sin(th1) - sin(th2);
    dx = cos(ph1) * cos(th1) - cos(th2);
    dy = sin(ph1) * cos(th1);
    return asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R;
}

static double compute_length(const vector<Coord> &coords) {
    double len = 0.0 ;

    for( int i=1 ; i<coords.size() ; i++ ) {
        const auto &cur = coords[i] ;
        const auto &prev = coords[i-1] ;
        double dist = haversine(prev.lon_, prev.lat_, cur.lon_, cur.lat_) ;
        len += dist ;
    }

    return len ;
}

const double ELEVATION_SAMPLING_STEP = 20.0 ;
const double MAX_ELEVATION_GAIN_THRESHOLD = 10.0 ;

static vector<double> compute_altitude_profile(DEM &dem, vector<Coord> &coords, double len) {
    vector<double> profile ;

    double dist = 0.0, step = ELEVATION_SAMPLING_STEP ;
    int i = 1 ;
    double seg_length = haversine(coords[0].lon_, coords[0].lat_, coords[1].lon_, coords[1].lat_) ;
    double dist_seg = 0 ;

    while ( dist < len ) {
        double h = (dist - dist_seg)/seg_length ;
        double lat = h * coords[i-1].lat_  + (1.0 - h) * coords[i].lat_ ;
        double lon = h * coords[i-1].lon_  + (1.0 - h) * coords[i].lon_ ;
        int16_t ele = dem.getElevation(lat, lon) ;

        profile.emplace_back(ele) ;

        dist += step ;
        while ( dist > dist_seg + seg_length ) {
            i++ ;
            dist_seg += seg_length ;
            seg_length = haversine(coords[i-1].lon_, coords[i-1].lat_, coords[i].lon_, coords[i].lat_) ;
        }
    }

    return profile ;

}

static vector<double> filter_profile(const vector<double> &prof, int fw) {
    vector<double> filtered ;

    int fw2 = fw/2 ;
    if ( fw > int(prof.size()) ) return prof ;

    for( size_t i=fw2 ; i<prof.size() - fw2 ; i++ ) {
        double v = 0.0 ;
        for (int k=-fw2 ; k<=fw2 ; k++)
             v += prof[i + k] ;

        filtered.push_back(v/fw) ;
    }

    return filtered ;
}

void Graph::Edge::computeElevationGainAndLoss(const vector<double> &prof) {
    double d = ELEVATION_SAMPLING_STEP ;
    double currentAlt, prevAlt = prof[0] ;

    gain_ = loss_ = 0.0 ;

    for( int i=1 ; i<prof.size() ; i++ ) {
        currentAlt = prof[i] ;

        if ( abs(currentAlt - prevAlt) > MAX_ELEVATION_GAIN_THRESHOLD ) {
            if ( currentAlt > prevAlt ) {
                gain_ += currentAlt - prevAlt ;
            } else {
                loss_ += prevAlt - currentAlt ;
            }

            prevAlt = currentAlt ;
        }

        d += ELEVATION_SAMPLING_STEP ;
    }
}

void Graph::computeEdgeStats(DEM &dem) {
    for ( auto &edge: edges_ ) {
        edge.length_ = compute_length(edge.coords_) ;
        auto profile = compute_altitude_profile(dem, edge.coords_, edge.length_) ;
        auto filtered = filter_profile(profile, 5) ;
        edge.computeElevationGainAndLoss(filtered) ;
    }
}

void Graph::exportToOSM(const string &path) {
    ofstream strm(path) ;

    XmlWriter writer(strm) ;

    writer.startDocument() ;

    writer.startTag("osm")
            .attribute("version", "0.6");

    for( const auto &np: nodes_ ) {
        const Node &node = np.second ;
        writer.startTag("node")
                .attribute("id", node.id_)
                .attribute("lat", format("%.7f", node.lat_))
                .attribute("lon", format("%.7f", node.lon_))
                .attribute("version", "1") ;

        writer.startTag("tag")
                .attribute("k", "place")
                .attribute("v", "locality")
                .endTag("tag") ;

        writer.endTag("node") ;
    }

    for( const auto &edge: edges_ ) {
        for( const auto &node_id: edge.nodes_) {
            auto it = nodes_.find(node_id) ;
            if ( it != nodes_.end() ) {
                const Node &node = (*it).second ;
                writer.startTag("node")
                        .attribute("id", node.id_)
                        .attribute("lat", format("%.7f", node.lat_))
                        .attribute("lon", format("%.7f", node.lon_))
                        .attribute("version", "1")
                        .endTag("node");
            }
        }
    }

    int counter = 1000000 ;

    for( const auto &edge: edges_ ) {
        writer.startTag("way")
                .attribute("id", format("%d", counter++))
                .attribute("version", "1");

        writer.startTag("tag")
                .attribute("k", "gain")
                .attribute("v", format("%.1f", edge.gain_))
                .endTag("tag") ;

        writer.startTag("tag")
                .attribute("k", "loss")
                .attribute("v", format("%.1f", edge.loss_))
                .endTag("tag") ;

        for( const auto &node_id: edge.nodes_) {
            writer.startTag("nd")
                 .attribute("ref", node_id)
                 .endTag("nd") ;
        }

        writer.endTag("way") ;
    }

    writer.endTag("osm") ;

    writer.endDocument() ;


}
