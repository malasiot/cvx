#include <cvx/misc/xml_writer.hpp>

#include <cvx/misc/strings.hpp>

using namespace std ;

namespace cvx {

void XmlWriter::startDocument(const string &encoding, bool standalone)
{
    strm_ << "<?xml version=\"1.0\" " ;

    if ( !encoding.empty() ) {
        encoding_ = toLowerCopy(encoding) ;
        if ( startsWith(encoding_, "utf" ) ) ;

        strm_ << "encoding=\"" <<  encoding_ << "\" ";
    }

    strm_ << "standalone=\"" << (( standalone ) ? "yes" : "no") << "\" ";
    strm_ << "?>" ;

}

void XmlWriter::endDocument() {
    while ( !element_stack_.empty() ) {
        Element &e = element_stack_.back() ;
        endElement() ;
    }
    strm_.flush();
}

void XmlWriter::indent() {
    if ( indent_ ) {
        strm_ << endl ;
        if (! element_stack_.empty() ) {
            for( auto &e: element_stack_ )
                strm_ << indent_chars_ ;
        }
    }
}

void XmlWriter::writePending() {
    if ( !element_stack_.empty() ) {
        Element &e = element_stack_.back() ;
        if ( e.is_open_ ) {
            strm_ << ">" ;
            e.is_open_ = false ;
        }
    }
}


XmlWriter &XmlWriter::startElement(const std::string &tag) {
    writePending() ;
    indent() ;
    Element e ;
    e.name_ = tag ;
    e.is_open_ = true ;
    element_stack_.emplace_back(e) ;
    strm_ << "<" << tag ;
    return *this;
}

XmlWriter &XmlWriter::endElement() {

    if ( !element_stack_.empty() ) {
        Element e = element_stack_.back() ;
        element_stack_.pop_back() ;

        if ( e.is_open_ )
            strm_ << "/>";
        else {

            indent() ;
            strm_ << "</" << e.name_ << ">" ;
        }
    }
    return *this ;
}


XmlWriter &XmlWriter::attribute(const std::string &key, const std::string &value) {
    if ( indent_attrs_ && indent_ ) {
        indent() ;
        strm_ << indent_chars_ ;
    } else
        strm_ << " " ;

    strm_ << key<< "=\"" ;
    writeEscaped(value, '"') ;
    strm_ << "\"" ;
    return *this ;
}

XmlWriter &XmlWriter::attributes(const std::map<string, string> &attrs) {
    for ( const auto &a: attrs ) {
        attribute(a.first, a.second) ;
    }
    return *this ;
}


XmlWriter &XmlWriter::text(const std::string &t) {
    indent() ;
    writeEscaped(t, -1) ;
    return *this ;
}

void XmlWriter::writeEscaped(const std::string &s, char quot) {
    for( char c: s ) {
        switch (c) {
        case '\n':
        case '\r':
        case '\t':
            if ( quot == -1 )
                strm_.put(c) ;
            else
                strm_ << "&#" << (int)c << ';';
            break;
        case '&' :
            strm_ << "&amp;";
            break;
        case '>' :
            strm_ << "&gt;";
            break;
        case '<' :
            strm_ << "&lt;";
            break;
        default:
            if ( c == quot ) {
                strm_ << (c == '"' ? "&quot;" : "&apos;");
                break;
            }
            strm_.put(c) ;

        }
    }
}


}
