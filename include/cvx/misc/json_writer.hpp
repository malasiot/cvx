#ifndef CVX_UTIL_JSON_WRITER_HPP
#define CVX_UTIL_JSON_WRITER_HPP

#include <ostream>
#include <memory>
#include <sstream>
#include <stack>

namespace cvx {

// Helper class for writing JSON inspired by Android JsonWriter

class JSONWriter {
public:
    JSONWriter(std::ostream &strm) ;

    // start a new object
    JSONWriter &beginObject() ;

    // start a new array
    JSONWriter &beginArray() ;

    // close object
    JSONWriter &endObject() ;

    // end array
    JSONWriter &endArray() ;

    // write a boolean value
    JSONWriter &booleanValue(bool v) ;

    // write a floating point value

    JSONWriter &floatValue(double v) ;

    // write integer
    JSONWriter &integerValue( int64_t  v ) ;

    // write null value
    JSONWriter &nullValue() ;

    // write string literal (it will be escaped as needed)
    JSONWriter &stringValue(const std::string &str) ;

    // write string literal without escaping
    JSONWriter &jsonValue(const std::string &str) ;

    // write name
    JSONWriter &name(const std::string &n) ;

    // Sets the indentation string to be repeated for each level of indentation in the encoded document.
    JSONWriter &setIndent(const char *indent) {
        indent_.assign(indent) ;
        return *this ;
    }

private:

    enum State { EMPTY_DOCUMENT, NON_EMPTY_DOCUMENT, EMPTY_OBJECT, NON_EMPTY_OBJECT, EMPTY_ARRAY, NON_EMPTY_ARRAY, DANGLING_NAME } ;
    std::deque<State> stack_ ;
    std::ostream &strm_ ;
    std::string indent_ ;

    void indent() ;
    void beforeValue(bool root);
    void escapeString(const std::string &str);
    void writeInt(int64_t v) ;
};

class JSONWriterException: public std::runtime_error {
public:
    JSONWriterException(const std::string &msg): std::runtime_error(msg) {}
};


}

#endif
