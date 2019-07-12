#include <cvx/util/misc/json_writer.hpp>

namespace cvx { namespace  util {

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

JSONWriter &JSONWriter::value(bool v) {
    beforeValue(false) ;
    if ( v ) strm_ << "true" ;
    else strm_ << "false" ;
    return *this ;
}

JSONWriter &JSONWriter::value(int64_t v) {
    beforeValue(false) ;
    strm_ << v ;
    return *this ;
}

JSONWriter &JSONWriter::value(double v) {
    beforeValue(false) ;
    strm_ << v ;
    return *this ;
}

JSONWriter &JSONWriter::value(const std::string &v) {
    beforeValue(false) ;
    escapeString(v) ;
    return *this ;
}

JSONWriter &JSONWriter::null() {
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

void JSONWriter::escapeString(const std::string &str) {
    strm_ << '"' << str <<'"' ;
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


}}
