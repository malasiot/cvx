#ifndef CVX_UTIL_XML_WRITER_HPP
#define CVX_UTIL_XML_WRITER_H

#include <iostream>
#include <deque>

#include <cvx/util/misc/dictionary.hpp>

namespace cvx { namespace util {

class XmlWriter {
public:
    XmlWriter(std::ostream &strm): strm_(strm) {}

    void startDocument (const std::string &encoding = "utf-8", bool standalone = true);
    void endDocument () ;

    void setPrefix (const std::string &prefix, const std::string &ns) ;

    XmlWriter &startTag (const std::string &name) ;
    XmlWriter &startTagNS (const std::string &ns, const std::string &name) ;

    XmlWriter &attribute(const std::string &name, const std::string &value);
    XmlWriter &attributeNS(const std::string &ns, const std::string &name, const std::string &value);

    XmlWriter &endTag (const std::string &name);
    XmlWriter &endTagNS (const std::string &ns, const std::string &name);

    XmlWriter &text(const std::string &txt);



private:
    std::ostream &strm_ ;
    int depth = 0 ;
    std::string encoding_ ;

    struct Element {
        std::string ns_ ;
        std::string name_ ;
        bool is_open_ ;
     };

    std::deque<Element> element_stack_ ;
    std::string pendingText ;

private:
    void indent();
    void writePending() ;
    void writeEscaped(const std::string &s, char quot);
};



}}






#endif
