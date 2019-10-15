#include "osm_parser.hpp"

#include <cvx/util/misc/xml_pull_parser.hpp>

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

                  if ( tagName == "node" ) {
                      double lat = attributes.value<double>("lat") ;
                      double lon = attributes.value<double>("lon") ;
                      current.reset(new OSMNode(id, lat, lon)) ;
                  } else if ( tagName == "way" ) {
                      current.reset(new OSMWay(id)) ;
                  } else if ( tagName == "tag" ) {
                      current->tags_.add(attributes.get("k"), attributes.get("v")) ;
                  } else if ( tagName == "nd" ) {
                      OSMWay *way = dynamic_cast<OSMWay *>(current.get())   ;
                      if ( way != nullptr )
                          way->nodes_.push_back(attributes.get("ref")) ;
                  }
              } else if ( et == XmlPullParser::END_TAG ) {
                  if ( tagName == "node" ) {
                      OSMNode *node = dynamic_cast<OSMNode *>(current.get())   ;
                      if ( nodeFilter == nullptr || nodeFilter(node->tags_) )
                         doc.nodes_.emplace(node->id_, std::move(*node)) ;
                  } else if ( tagName == "way" ) {
                      OSMWay *way = dynamic_cast<OSMWay *>(current.get())   ;
                      if ( wayFilter == nullptr || wayFilter(way->tags_) )
                        doc.ways_.emplace(way->id_, std::move(*way)) ;
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
