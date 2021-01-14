#include <cvx/util/misc/json_reader.hpp>
#include <cvx/util/misc/json_writer.hpp>

#include <cvx/util/misc/variant.hpp>
#include <iostream>
#include "mhx2_importer.hpp"

using namespace cvx::util ;
using namespace std ;

static string json_src = R"(
                         {
                      "glossary": {
                          "numEntries": 100.4,
                           "values": ["GML", "XML"],
                          "title": "example glossary",
                          "GlossDiv": {
                              "title": "S",
                              "GlossList": {
                                  "GlossEntry": {
                                      "ID": "SGML",
                                      "SortAs": "SGML",
                                      "GlossTerm": "Standard Generalized Markup Language",
                                      "Acronym": "SGML",
                                      "Abbrev": "ISO 8879:1986",
                                      "GlossDef": {
                                          "para": "A meta-markup language, used to create markup languages such as DocBook.",
                                          "GlossSeeAlso": ["GML", "XML"]
                                      },
                                      "GlossSee": "markup"
                                  }
                              }
                          }
                      }
                  }





                  )" ;

void test1() {
    stringstream strm(json_src) ;
    JSONReader reader(strm) ;

    try {
        reader.beginObject();
        while (reader.hasNext()) {
            string name = reader.nextName();
            if (name == "glossary" ) {
                reader.beginObject() ;
                while (reader.hasNext()) {
                    string name = reader.nextName();
                    if ( name == "values" ) {
                        reader.beginArray() ;
                        while (reader.hasNext()) {
                            std::cout << reader.nextString() << endl ;
                        }
                        reader.endArray() ;
                    }
                    else if ( name == "title" )
                        string t = reader.nextString() ;
                    else if ( name == "numEntries" ) {
                        double v = reader.nextDouble() ;
                    } else {
                        reader.skipValue() ;
                    }
                }

                reader.endObject() ;
            } else {
                reader.skipValue();
            }
        }
        reader.endObject();
    } catch ( JSONParseException &e ) {
        cout << e.what() << endl ;
    }
}

void test2() {
    Mhx2Importer importer ;
    importer.load("/home/malasiot/Downloads/human.mhx2", "");
}

void test3() {
    JSONWriter writer(cout) ;
    writer.setIndent("   ") ;
    writer.beginObject() ;
    writer.name("field1").stringValue("string") ; cout.flush() ;
    writer.name("field2") ;
    writer.beginArray() ;
    writer.integerValue((int)10) ;
    writer.integerValue(20) ;
    writer.stringValue("30hello") ;
    writer.endArray();
    writer.name("field3").floatValue(1.0) ;
    writer.endObject() ;
}

void test_variant() {
    Variant v = {
    {"pi", 3.141},
    {"happy", true},
    {"name", "Niels"},
    {"nothing", nullptr},
    {"answer", {
      {"everything", 42}
    }},
    {"list", {1, 0, 2}},
    {"object", {
      {"currency", "USD"},
      {"value", 42.99}
    }}
  };

    Variant array_not_object = Variant::array({ {"currency", "USD"}, {"value", 42.99} });

     cout << array_not_object.toJSON() << endl ;
    cout << v.value("answer.everything", nullptr).toJSON() << endl ;

    cout << v.toJSON() << endl ;

    istringstream strm(json_src) ;
    auto r = Variant::fromJSON(strm) ;
    cout << r.toJSON() << endl ;

}
int main(int argc, char *argv[]) {
    test1() ;
    test_variant() ;

}
