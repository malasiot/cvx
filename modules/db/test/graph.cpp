#include "graph.hpp"

#include <cmath>
#include <fstream>

#include <cvx/util/misc/xml_writer.hpp>
#include <cvx/util/misc/strings.hpp>
#include <cvx/util/misc/path.hpp>


#include <sqlite3.h>
#include <spatialite.h>

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

    writer.startElement("osm")
            .attribute("version", "0.6");

    for( const auto &np: nodes_ ) {
        const Node &node = np.second ;
        writer.startElement("node")
                .attribute("id", node.id_)
                .attribute("lat", format("%.7f", node.lat_))
                .attribute("lon", format("%.7f", node.lon_))
                .attribute("version", "1") ;

        writer.startElement("tag")
                .attribute("k", "place")
                .attribute("v", "locality")
                .endElement() ;

        writer.endElement() ;
    }

    for( const auto &edge: edges_ ) {
        for ( int i=0 ; i<edge.nodes_.size() ; i++ ) {
            const string &node_id = edge.nodes_[i] ;
            const Coord &coord = edge.coords_[i] ;

            auto it = nodes_.find(node_id) ;
                if ( it == nodes_.end() ) {

                    writer.startElement("node")
                        .attribute("id", node_id)
                        .attribute("lat", format("%.7f", coord.lat_))
                        .attribute("lon", format("%.7f", coord.lon_))
                        .attribute("version", "1")
                        .endElement();
                }
        }
    }

    int counter = 1000000 ;

    for( const auto &edge: edges_ ) {
        writer.startElement("way")
                .attribute("id", format("%d", counter++))
                .attribute("version", "1");

        writer.startElement("tag")
                .attribute("k", "gain")
                .attribute("v", format("%.1f", edge.gain_))
                .endElement() ;

        writer.startElement("tag")
                .attribute("k", "loss")
                .attribute("v", format("%.1f", edge.loss_))
                .endElement() ;

        for( const auto &node_id: edge.nodes_) {
            writer.startElement("nd")
                 .attribute("ref", node_id)
                 .endElement() ;
        }

        writer.endElement() ;
    }

    writer.endElement() ;

    writer.endDocument() ;


}

class SpatialLiteSingleton
{
public:

    static SpatialLiteSingleton instance_;

    static SpatialLiteSingleton& instance() {
        return instance_;
    }

private:

    SpatialLiteSingleton () {
        spatialite_init(false);
    }

    ~SpatialLiteSingleton () {
        spatialite_cleanup();
    }

    SpatialLiteSingleton( SpatialLiteSingleton const & );

    void operator = ( SpatialLiteSingleton const & );
};

SpatialLiteSingleton SpatialLiteSingleton::instance_ ;

void Graph::write(const string &path) {
    using namespace cvx::db ;

    Path p(path) ;
    Path::remove(p) ;

    Connection con("sqlite:db=" + path + ";mode=rc") ;

  //  con.execute("SELECT load_extension(\"libSqliteIcu\")") ;
    con.execute("SELECT InitSpatialMetaData(1);");

    writeNodes(con) ;
    writeEdges(con) ;
}

void Graph::writeNodes(cvx::db::Connection &con)
{
    using namespace cvx::db ;

    con.execute("DROP TABLE IF EXISTS `nodes`");

    con.execute(R"(
                CREATE TABLE IF NOT EXISTS `nodes` (
            `id` INTEGER NOT NULL,
            `lat` DOUBLE NOT NULL,
            `lon` DOUBLE NOT NULL,
            `ele` INTEGER NOT NULL,
             PRIMARY KEY (`id`)
            )
    )") ;

    Transaction trans = con.transaction() ;

    Statement stmt = con.prepareStatement("INSERT INTO `nodes` (`id`, `lat`, `lon`, `ele`) VALUES ( ?, ?, ?, ? )");

    for( const auto &np: nodes_ ) {
        const Node &node = np.second ;
        stmt.clear() ;
        stmt(node.id_, node.lat_, node.lon_, node.ele_) ;

    }

    trans.commit() ;
}

static string makeWKTMultiPointString(const vector<Coord> &coords) {
    string ptstring ;

    for( const Coord &c: coords ) {
        if ( ptstring.empty() )
            ptstring = "MULTIPOINTM(" ;
        else
            ptstring += ',' ;
        ptstring += format("%f %f %f", c.lon_, c.lat_, c.ele_) ;
    }

    ptstring += ')';

    return ptstring;
}

void Graph::writeEdges(cvx::db::Connection &con)
{
    using namespace cvx::db ;

    con.execute("DROP TABLE IF EXISTS `edges`");

    con.execute(R"(
                CREATE TABLE IF NOT EXISTS `edges` (
                    id INTEGER NOT NULL,
                    start INTEGER NOT NULL,
                    stop INTEGER NOT NULL,
                    dist DOUBLE NOT NULL,
                    gain DOUBLE NOT NULL,
                    loss DOUBLE NOT NULL,
                    type INTEGER NOT NULL
            )
    )") ;

    con.execute("SELECT AddGeometryColumn('edges', 'geom', 4326, 'MULTIPOINT', 'XYM')");
    con.execute("SELECT CreateSpatialIndex('edges', 'geom')");

    Statement stmt = con.prepareStatement(R"(INSERT INTO `edges` (`id`, `start`, `stop`, `dist`, `gain`, `loss`, `type`, `geom`) VALUES
                     ( ?, ?, ?, ?, ?, ?, ?, ST_GeomFromText(?, 4326)) )") ;

    int count = 0 ;

    Transaction trans = con.transaction() ;

    for ( Edge &e: edges_ ) {
        string highway = e.tags_.get("highway") ;
        string track_type = e.tags_.get("tracktype", "grade1") ;
        int edge_type ;

        if ( highway == "track" && ( track_type == "grade1" || track_type == "grade2" || track_type == "grade3"))
            edge_type = 1 ;
        else if ( highway == "track" && ( track_type == "grade4" || track_type == "grade5" ))
            edge_type = 0 ;
        else if ( highway == "path" || highway == "footway" || highway == "bridleway" || highway == "steps" || highway == "cycleway")
            edge_type = 0 ;
        else
            edge_type = 2 ;

        stmt.clear() ;
        stmt(count++, e.start_, e.stop_, e.length_, e.gain_, e.loss_, edge_type, makeWKTMultiPointString(e.coords_)) ;
    }

    trans.commit() ;
}
