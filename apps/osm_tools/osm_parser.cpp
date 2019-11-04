#include "osm_parser.hpp"
#include "dem.hpp"

#include <cvx/util/misc/xml_pull_parser.hpp>
#include <cmath>

using namespace cvx::util ;
using namespace std ;

bool OSMParser::parse(std::istream &strm, OSMDocument &doc, Filter nodeFilter, Filter wayFilter)
{
    XmlPullParser parser(strm) ;
    std::unique_ptr<OSMFeature> current ;

    try {
          while ( parser.next() != XmlPullParser::END_DOCUMENT ) {
              auto et = parser.getEventType() ;
              string tagName = parser.getName() ;

              if ( et == XmlPullParser::START_TAG ) {
                  auto attributes = parser.getAttributes() ;
                  string id = attributes.get("id") ;
                  string action = attributes.get("action") ;

                  if ( tagName == "node" ) {
                      double lat = attributes.value<double>("lat") ;
                      double lon = attributes.value<double>("lon") ;
                      current.reset(new OSMNode(id, lat, lon)) ;
                      current->setDeleted(action == "delete") ;
                  } else if ( tagName == "way" ) {
                      current.reset(new OSMWay(id)) ;
                      current->setDeleted(action == "delete") ;
                  } else if ( tagName == "tag" ) {
                      current->tags_.add(attributes.get("k"), attributes.get("v")) ;
                  } else if ( tagName == "nd" ) {
                      OSMWay *way = dynamic_cast<OSMWay *>(current.get())   ;
                      if ( way != nullptr )
                          way->nodes_.push_back(attributes.get("ref")) ;
                  }
              } else if ( et == XmlPullParser::END_TAG ) {
                  if ( tagName == "node" ) {
                      if ( !current->isDeleted() ) {
                        OSMNode *node = dynamic_cast<OSMNode *>(current.get())   ;
                        if ( nodeFilter == nullptr || nodeFilter(node->tags_) )
                             doc.nodes_.emplace(node->id_, std::move(*node)) ;
                      }
                  } else if ( tagName == "way" ) {

                      if ( !current->isDeleted() ) {
                      OSMWay *way = dynamic_cast<OSMWay *>(current.get())   ;
                      if ( way->id_ == "82514026" ) {
                          cout << "ok" << endl ;
                      }
                      if ( wayFilter == nullptr || wayFilter(way->tags_) )
                        doc.ways_.emplace_back(*way) ;
                      }
                  }

              } else if ( et == XmlPullParser::TEXT ) {
    //              current_->value_ = trimCopy(parser.getText()) ;
              }
          }
          return true ;
      } catch ( XmlPullParserException & ) {
          return false ;
      }

}


void OSMDocument::fillInElevationsFromDEM(DEM &dem) {
    for ( auto &lp: nodes_ ) {
        OSMNode &node = lp.second ;
        node.ele_ = dem.getElevation(node.lat_, node.lon_) ;
    }


}

vector<Coord> OSMDocument::getLineString(const OSMWay &way) const
{
    vector<Coord> coords ;

    for ( const auto &ref: way.nodes_ ) {
        auto it = nodes_.find(ref) ;
        if ( it != nodes_.end() ) {
            const OSMNode &node = (*it).second ;
            coords.emplace_back(node.lat_, node.lon_, node.ele_) ;
        }
    }

    return coords ;
}
