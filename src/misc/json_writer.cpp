#include <cvx/misc/json_writer.hpp>
#include <iomanip>

using namespace std ;

namespace cvx {

// Original: https://gist.github.com/kevinkreiser/bee394c60c615e0acdad

void JSONWriter::escapeString(const std::string &str) {
    strm_ << '"';

    for (const auto& c : str) {
        switch (c) {
        case '\\': strm_ << "\\\\"; break;
        case '"': strm_ << "\\\""; break;
        case '/': strm_ << "\\/"; break;
        case '\b': strm_ << "\\b"; break;
        case '\f': strm_ << "\\f"; break;
        case '\n': strm_ << "\\n"; break;
        case '\r': strm_ << "\\r"; break;
        case '\t': strm_ << "\\t"; break;
        default:
            if(c >= 0 && c < 32) {
                //format changes for json hex
                strm_.setf(std::ios::hex, std::ios::basefield);
                strm_.setf(std::ios::uppercase);
                strm_.fill('0');
                //output hex
                strm_ << "\\u" << std::setw(4) << static_cast<int>(c);
            }
            else
                strm_ << c;
            break;
        }
    }
    strm_ << '"';
}


JSONWriter::JSONWriter(std::ostream &strm): strm_(strm) {
    stack_.push_back(EMPTY_DOCUMENT) ;
}

JSONWriter &JSONWriter::beginObject() {
    beforeValue(true) ;
    stack_.push_back(EMPTY_OBJECT) ;
    strm_ << '{' ;
    return *this;
}

JSONWriter &JSONWriter::endObject() {
    auto state = stack_.back() ;
    stack_.pop_back() ;
    if ( state == EMPTY_OBJECT ) {
        strm_ << '}' ;
    } else if ( state == NON_EMPTY_OBJECT ) {
        indent() ;
        strm_ << "}" ;
    } else {
        throw JSONWriterException("end object called out of context") ;
    }

    return *this;
}

JSONWriter &JSONWriter::endArray() {

    auto state = stack_.back() ;
    stack_.pop_back() ;
    if ( state == EMPTY_ARRAY ) {
        strm_ << ']' ;
    } else if ( state == NON_EMPTY_ARRAY ) {
        indent() ;
        strm_ << "]" ;
    } else {
        throw JSONWriterException("end array called out of context") ;
    }

    return *this;
}

JSONWriter &JSONWriter::booleanValue(bool v) {
    beforeValue(false) ;
    if ( v ) strm_ << "true" ;
    else strm_ << "false" ;
    return *this ;
}

JSONWriter &JSONWriter::integerValue(int64_t v) {
    beforeValue(false) ;
    strm_ << v ;
    return *this ;
}

JSONWriter &JSONWriter::floatValue(double v) {
    beforeValue(false) ;
    strm_ << v ;
    return *this ;
}

JSONWriter &JSONWriter::stringValue(const std::string &v) {
    beforeValue(false) ;
    escapeString(v) ;
    return *this ;
}

JSONWriter &JSONWriter::nullValue() {
    beforeValue(false) ;
    strm_ << "null" ;
    return *this ;
}

JSONWriter &JSONWriter::name(const std::string &n) {
    auto context = stack_.back() ;
    if ( context == NON_EMPTY_OBJECT ) {
       strm_.put(',') ;
    } else if ( context != EMPTY_OBJECT ) {
        throw JSONWriterException("nesting problem") ;
    }
    stack_.back() = DANGLING_NAME;
    indent() ;
    escapeString(n) ;
    return *this ;
}

void JSONWriter::indent() {
    if ( indent_.empty() ) return ;

    strm_ << std::endl ;

    for ( size_t i = 1 ; i < stack_.size() ; i++ ) {
        strm_<< indent_ ;
    }
}


JSONWriter &JSONWriter::beginArray() {
    beforeValue(true) ;
    stack_.push_back(EMPTY_ARRAY) ;
    strm_ << '[' ;
    return *this;
}


void JSONWriter::beforeValue(bool root)  {
    auto state = stack_.back() ;
    switch ( state ) {
    case EMPTY_DOCUMENT: // first in document
        if ( !root ) {
            throw JSONWriterException("JSON must start with an array or an object.");
        }
        stack_.back() = NON_EMPTY_DOCUMENT ;
        break;
    case EMPTY_ARRAY: // first in array
        stack_.back() = NON_EMPTY_ARRAY ;
        indent() ;
        break;
    case NON_EMPTY_ARRAY: // another in array
        strm_.put(',');
        indent() ;
        break;
    case DANGLING_NAME: // value for name
        strm_.put(':') ;
        stack_.back() = NON_EMPTY_OBJECT ;
        break;
    case NON_EMPTY_DOCUMENT:
        throw JSONWriterException("JSON must have only one top-level value.");
    default:
        throw JSONWriterException("nesting problem") ;
    }
}


}
