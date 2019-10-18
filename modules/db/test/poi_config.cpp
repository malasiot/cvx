#include "poi_config.hpp"

#include <fstream>
#include <iostream>

using namespace cvx::util ;
using namespace std ;

POIConfig::POIConfig()
{

}

void POIConfig::parseRule(JSONReader &reader, Category &category) {
    string key, val ;

    reader.beginObject() ;
    while (reader.hasNext()) {
        string tag = reader.nextName() ;

        if ( tag == "k" ) {
            key = reader.nextString() ;
        } else if ( tag == "v" ) {
            val = reader.nextString() ;
        }
    }
    reader.endObject() ;

    if ( !key.empty() && !val.empty() ) {
        Rule rule ;
        rule.key_ = key ;
        rule.val_ = val ;
        category.rules_.emplace_back(std::move(rule)) ;
    }

}

void POIConfig::parseCategory(JSONReader &reader) {
    Category category ;
    reader.beginObject() ;
    while (reader.hasNext()) {
        string key = reader.nextName() ;

        if ( key == "category" )
            category.id_ = reader.nextString() ;
        else if ( key == "names" ) {
            reader.beginObject() ;
            while ( reader.hasNext() ) {
                string key = reader.nextName() ;
                string value = reader.nextString() ;
                category.lnames_.emplace(std::move(key), std::move(value)) ;
            }
            reader.endObject() ;
        }
        else if ( key == "rules" ) {
            reader.beginArray() ;
            while ( reader.hasNext() ) {
                parseRule(reader, category) ;
            }
            reader.endArray() ;
        }

    }


    reader.endObject() ;

    categories_.emplace_back(std::move(category)) ;
}

bool POIConfig::parse(const std::string &path) {
    ifstream strm(path) ;

    JSONReader reader(strm) ;

    try {
        reader.beginObject();
        while (reader.hasNext()) {
            string name = reader.nextName();
            if (name == "languages" ) {
                reader.beginArray() ;
                while (reader.hasNext()) {
                    string lang = reader.nextString();
                    languages_.emplace_back(std::move(lang)) ;
                }
                reader.endArray() ;
            } else if ( name == "categories" ) {
                reader.beginArray() ;
                while (reader.hasNext()) {
                    parseCategory(reader) ;
                }
                reader.endArray() ;
            } else {
                reader.skipValue();
            }
        }
        reader.endObject();
    } catch ( JSONParseException &e ) {
        cerr << e.what() << endl ;
        return false ;
    }

    return true ;
}
