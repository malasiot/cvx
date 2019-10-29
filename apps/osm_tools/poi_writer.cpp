#include "poi_writer.hpp"

#include <sqlite3.h>
#include <spatialite.h>

#include <cvx/util/misc/path.hpp>
#include <cvx/util/misc/strings.hpp>

using namespace cvx::db ;
using namespace cvx::util ;
using namespace std ;

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

static const std::string DATABASE_VERSION = "1" ;

void POIWriter::write(const std::string &db_path, const POIConfig &cfg) {


    Path p(db_path) ;
    Path::remove(p) ;

    Connection con("sqlite:db=" + db_path + ";mode=rc") ;

    con.execute("SELECT InitSpatialMetaData('WGS84');");

    string greek_grid_srs_sql = R"(INSERT INTO spatial_ref_sys (srid, auth_name, auth_srid, ref_sys_name, proj4text, srtext) VALUES (2100, 'epsg', 2100, 'GGRS 87 / Greek Grid',
            '+proj=tmerc +lat_0=0 +lon_0=24 +k=0.9996 +x_0=500000 +y_0=0 +ellps=GRS80 +towgs84=-199.87,74.79,246.62,0,0,0,0 +units=m +no_defs',
            'PROJCS[\"GGRS87 / Greek Grid\",GEOGCS["GGRS87",DATUM["Greek_Geodetic_Reference_System_1987",SPHEROID["GRS 1980",6378137,298.257222101,AUTHORITY["EPSG",
            "7019"]],TOWGS84[-199.87,74.79,246.62,0,0,0,0],AUTHORITY["EPSG","6121"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",
            0.0174532925199433,AUTHORITY["EPSG","9122"]],AXIS["Latitude",NORTH],AXIS["Longitude",EAST],AUTHORITY["EPSG","4121"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",
            0],PARAMETER["central_meridian",24],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",0],UNIT["metre",
            1,AUTHORITY["EPSG","9001"]],AXIS["Easting",EAST],AXIS["Northing",NORTH],AUTHORITY["EPSG","2100"]]')
            )";

    con.execute(greek_grid_srs_sql) ;

    con.execute(R"(CREATE TABLE IF NOT EXISTS `metadata` ( `key` TEXT NOT NULL,
                `value` TEXT NULL);)");

    con.execute("INSERT INTO `metadata` (key, value) VALUES ('version', ?)", DATABASE_VERSION);

    writeNodes(con, cfg) ;

    for ( const string &lang: cfg.languages_ )  {
        Path localized_db_path(p.parentPath(), "poi_names_" + lang + ".sqlite");
        Path::remove(localized_db_path) ;

        Connection ldb_con("sqlite:db=" + localized_db_path.absolute() + ";mode=rc") ;

        makeNames(lang, ldb_con, cfg) ;
    }
}

void POIWriter::makeNames(const string &lang, Connection &con, const POIConfig &config) {
    con.execute("DROP TABLE IF EXISTS `names`") ;
    con.execute(R"(  CREATE TABLE IF NOT EXISTS `names` (
                    `id` INTEGER NOT NULL,
                    `name` TEXT NOT NULL )
                )");

    con.execute("CREATE INDEX `names_osm_id` ON `names` (`id`)") ;

    Transaction trans(con) ;
    Statement stmt = con.prepareStatement("INSERT INTO `names` (`id`, `name`) VALUES ( ?, ? )") ;

    for ( const auto &np: doc_.nodes_ ) {
        const OSMNode &node = np.second ;
        string name = getPOIName(node, lang) ;
        stmt.clear() ;
        stmt(node.id_, name) ;
    }
    trans.commit() ;

    con.execute("CREATE VIRTUAL TABLE `names_idx` USING FTS3 (id, content)");
    con.execute("INSERT INTO `names_idx` SELECT id, upper(name, '" + lang + "') as content FROM `names`") ;

    con.execute("DROP TABLE IF EXISTS `categories`") ;
    con.execute(R"(  CREATE TABLE IF NOT EXISTS `categories` (
                    `id` TEXT NOT NULL,
                    `name` TEXT NOT NULL,
                    PRIMARY KEY(id) )
                )");
    trans = con.transaction() ;
    stmt = con.prepareStatement("INSERT INTO `categories` (`id`, `name`) VALUES ( ?, ? )") ;

    for ( const auto &cat: config.categories_ ) {
        stmt.clear() ;
        auto it = cat.lnames_.find(lang) ;
        if ( it != cat.lnames_.end() )
            stmt(cat.id_, (*it).second) ;
    }
    trans.commit() ;
}

string POIWriter::getPOIName(const OSMNode &node, const string &lang)
{
    for ( const auto &np: node.tags_ ) {
        const string &tag_key = np.first ;
        const string &tag_value = np.second ;
        if ( startsWith(tag_key, "name") ) {
            string tag_lang ;
            if ( tag_key.size() > 5 ) tag_lang = tag_key.substr(5) ;

            if ( tag_lang.empty() || lang == tag_lang ) return tag_value ;
        }
    }

    return string() ;
}


static string makeWKTPointString(const OSMNode &coords) {
    ostringstream strm ;
    strm << "POINTM(" << coords.lon_ << ' ' << coords.lat_ << ' ' << coords.ele_ << ")";
    return strm.str();
}

string POIWriter::getPOItype(const OSMNode &node, const POIConfig &config) {
    for( const auto &category: config.categories_) {
        string id = category.id_ ;
        for ( const auto &rule: category.rules_ ) {
            string k = rule.key_ ;
            string v = rule.val_ ;
            auto tokens = split(v, "|") ;
            for( const auto &tp: node.tags_ ) {
                string tag_key = tp.first ;
                string tag_value = tp.second ;

                if ( tag_key == k ) {
                    for( const string &token: tokens ) {
                        if ( tag_value == token )
                            return id ;
                    }
                }
            }
        }
    }
    return string() ;
}


void POIWriter::writeNodes(cvx::db::Connection &con, const POIConfig &config)
{
    con.execute("DROP TABLE IF EXISTS `poi_categories`") ;
    con.execute(R"(  CREATE TABLE IF NOT EXISTS `poi_categories` (
                `id` INTEGER NOT NULL,
                `key` TEXT NOT NULL,
                 PRIMARY KEY (`id`)
                )
                )");

    Transaction trans =con.transaction() ;
    Statement stmt = con.prepareStatement("INSERT INTO `poi_categories` (`id`, `key`) VALUES ( ?, ? )") ;

    int count = 0 ;
    std::map<string, int> categoryIndex ;

    for ( const POIConfig::Category &cat: config.categories_ ) {
        categoryIndex.emplace(cat.id_, count) ;
        stmt.clear() ;
        stmt(count++, cat.id_) ;
    }

    trans.commit() ;

    con.execute("DROP TABLE IF EXISTS `pois`") ;
    con.execute(R"(  CREATE TABLE IF NOT EXISTS `pois` (
                `id` INTEGER NOT NULL,
                `category` INTEGER NOT NULL,
                 PRIMARY KEY (`id`)
                )
                )");

    con.execute("SELECT AddGeometryColumn('pois', 'geom', 4326, 'POINT', 'XYM')");
    con.execute("SELECT CreateSpatialIndex('pois', 'geom')");

    trans = con.transaction() ;
    stmt = con.prepareStatement("INSERT INTO `pois` (`id`, `category`, `geom`) VALUES ( ?, ?, GeomFromText(?, 4326) )") ;

    for ( const auto &np: doc_.nodes_ ) {
        const OSMNode &node = np.second ;

        stmt.clear() ;
        string poiType = getPOItype(node, config) ;
        if ( !poiType.empty() )
            stmt(node.id_, categoryIndex[poiType], makeWKTPointString(node)) ;
    }
    trans.commit() ;
}
