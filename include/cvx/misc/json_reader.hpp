#ifndef CVX_UTIL_JSON_READER_HPP
#define CVX_UTIL_JSON_READER_HPP

#include <istream>
#include <memory>
#include <sstream>
#include <stack>

namespace cvx {

namespace detail {
class JSONTokenizer ;
}

enum class JSONToken { BEGIN_ARRAY, BEGIN_OBJECT, BOOLEAN, END_ARRAY,END_DOCUMENT, END_OBJECT,
       NAME, JSON_NULL, NUMBER, STRING, UNDEFINED } ;

// Pull-like JSON parser class inspired by android JsonReader

class JSONReader {
public:
    JSONReader(std::istream &strm) ;

    // expect a new object
    void beginObject() ;

    // expect array
    void beginArray() ;

    // expect end object
    void endObject() ;

    // expect end of array
    void endArray() ;

    // assert object or array has more elements
    bool hasNext() ;

    // expect a boolean value
    bool nextBoolean() ;

    // expect JSON number converted to double
    double nextDouble() ;

    // expect JSON number converted to integer
    int64_t nextInt() ;

    // expect null value
    void nextNull() ;

    // expect string literal
    std::string nextString() ;

    // expect name
    std::string nextName() ;

    // peek next token without consuming it
    JSONToken peek() ;

    // skip value and each descendents (if object or array)
    void skipValue() ;

private:

    enum State { EMPTY_DOCUMENT, NON_EMPTY_DOCUMENT, EMPTY_OBJECT, NON_EMPTY_OBJECT, EMPTY_ARRAY, NON_EMPTY_ARRAY, DANGLING_NAME, NV_VALUE } ;
    std::shared_ptr<detail::JSONTokenizer> tokenizer_ ;

    JSONToken current_token_ ;
    std::stack<State> state_stack_ ;
    bool skipping_ = false ;

    void advance() ;
    JSONToken nextValue(int tk);
};

// Thrown by JSONReader in case of error

class JSONParseException: public std::exception {
public:

    JSONParseException(const std::string &msg, uint line, uint col);

    const char *what() const noexcept override {
        return msg_.c_str() ;
    }
protected:
    std::string msg_ ;
};


}

#endif
