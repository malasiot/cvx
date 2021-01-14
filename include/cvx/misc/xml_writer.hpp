#ifndef CVX_UTIL_XML_WRITER_HPP
#define CVX_UTIL_XML_WRITER_HPP

#include <iostream>
#include <deque>

#include <cvx/misc/dictionary.hpp>

namespace cvx {

// Minimal XML serializer class

class XmlWriter {
public:
    XmlWriter(std::ostream &strm): strm_(strm) {}

    void startDocument (const std::string &encoding = "utf-8", bool standalone = true);
    void endDocument () ;


    XmlWriter &startElement (const std::string &name) ;

    XmlWriter &attribute(const std::string &name, const std::string &value);

    XmlWriter &attributes(const std::map<std::string, std::string> &attrs) ;

    XmlWriter &endElement ();

    XmlWriter &text(const std::string &txt);

    // write a single element with associated text
    XmlWriter &element(const std::string &tag, const std::string &value) {
        return startElement(tag).text(value).endElement() ;
    }


    // enable/disable identation
    XmlWriter &setIndent(bool indent) {
        indent_ = indent ;
        return *this ;
    }

    // set characters used for indentation
    XmlWriter &setIndentChars(const std::string &s) {
        indent_chars_ = s ;
        return *this ;
    }

    XmlWriter &setIndentAttributes(bool ia) {
        indent_attrs_ = ia ;
        return *this ;
    }

private:
    std::ostream &strm_ ;
    std::string encoding_ ;

    struct Element {
        std::string name_ ;
        bool is_open_ ;
     };

    std::deque<Element> element_stack_ ;
    std::string indent_chars_= "  ";
    bool indent_ = true, indent_attrs_ = false ;

private:
    void indent();
    void writePending() ;
    void writeEscaped(const std::string &s, char quot);
};


}

#endif
