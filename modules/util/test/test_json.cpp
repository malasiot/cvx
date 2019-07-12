#include <cvx/util/misc/json_reader.hpp>
#include <cvx/util/misc/json_writer.hpp>
#include <iostream>
#include "mhx2_importer.hpp"

using namespace cvx::util ;
using namespace std ;

static string json_src = R"(
                         {
                      "glossary": {
                          "numEntries": 100.4,
                           "values": ["GML", "XML",
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
    writer.name("field1").value(std::string("string")) ; cout.flush() ;
    writer.name("field2") ;
    writer.beginArray() ;
    writer.value((int64_t)10) ;
    writer.value((int64_t)20) ;
    writer.value((int64_t)30) ;
    writer.endArray();
    writer.name("field3").value((double)1.0) ;
    writer.endObject() ;
}
int main(int argc, char *argv[]) {

    test3() ;

}
