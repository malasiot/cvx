#include <cvx/util/misc/xml_writer.hpp>

#include <cvx/util/misc/strings.hpp>

using namespace std ;

namespace cvx { namespace util {

void XmlWriter::startDocument(const string &encoding, bool standalone)
{
    strm_ << "<?xml version='1.0' " ;

    if ( !encoding.empty() ) {
        encoding_ = toLowerCopy(encoding) ;
        if ( startsWith(encoding_, "utf" ) ) ;

        strm_ << "encoding='" <<  encoding_ << "' ";
    }

    strm_ << "standalone='" << (( standalone ) ? "yes" : "no") << "' ";
    strm_ << "?>\n";
}

void XmlWriter::endDocument() {
   while ( !element_stack_.empty() ) {
       Element &e = element_stack_.back() ;
       if ( e.ns_.empty() )
           endTag(e.name_) ;
       else
           endTagNS(e.ns_, e.name_) ;

   }
   strm_.flush();
}

void XmlWriter::indent() {
     strm_ << "\n" ;
    if (! element_stack_.empty() ) {


        for( auto &e: element_stack_ )
            strm_ << "  " ;
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

XmlWriter &XmlWriter::startTagNS(const std::string &ns, const std::string &tag) {
    writePending() ;
    indent() ;
    Element e ;
    e.ns_ = ns ;
    e.name_ = tag ;
    e.is_open_ = true ;
    element_stack_.emplace_back(e) ;
    strm_ << "<" << tag ;
    return *this;
}

XmlWriter &XmlWriter::startTag(const std::string &tag) {
    writePending() ;
    indent() ;
    Element e ;
    e.name_ = tag ;
    e.is_open_ = true ;
    element_stack_.emplace_back(e) ;
    strm_ << "<" << tag ;
    return *this;
}

XmlWriter &XmlWriter::endTag(const std::string &name) {

    if ( !element_stack_.empty() ) {
        Element e = element_stack_.back() ;
        if ( e.name_ == name ) {

            element_stack_.pop_back() ;

            if ( e.is_open_ )
                strm_ << "/>";
            else {

                indent() ;
                strm_ << "</" << name << ">" ;
            }

        }
    }
    return *this ;
}

XmlWriter &XmlWriter::endTagNS(const std::string &ns, const std::string &name) {
    strm_ << "</" << name << ">";
    element_stack_.pop_back() ;
    return *this ;
}

XmlWriter &XmlWriter::attribute(const std::string &key, const std::string &value) {
    strm_ << " " << key<< "='" ;
    writeEscaped(value, '\'') ;
    strm_ << "'" ;
    return *this ;
}

XmlWriter &XmlWriter::text(const std::string &t) {
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


}}
