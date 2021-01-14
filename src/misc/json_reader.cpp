#include <cvx/misc/json_reader.hpp>

#include <iterator>
#include <stack>
#include <cassert>
#include <ios>

using namespace std ;

namespace cvx {

enum Token { OPEN_BRACE, CLOSE_BRACE, OPEN_BRACKET, CLOSE_BRACKET, STRING, BOOLEAN, NUMBER, JSON_NULL, COMMA, COLON, EMPTY, EOF_DOCUMENT } ;

class JSONTokenizer {
public:

    JSONTokenizer(istream &src): src_(src), pos_(src) {
    }

    struct JSONNumber {
        union Value {
            double double_value_ ;
            int64_t int_value_ ;
        };

        int64_t toInt() const { return ( is_integer_) ? value_.int_value_ : (int64_t)value_.double_value_ ; }
        double toDouble() const { return ( is_integer_) ? (double)value_.int_value_ : value_.double_value_ ; }

        Value value_ ;
        bool is_integer_ ;
    };

    JSONNumber token_number_literal_ ;
    bool is_token_integer_literal_ ;
    bool token_boolean_literal_ ;
    std::string token_string_literal_ ;

    Token nextToken() ;
    void expectToken(Token tk, const char *msg) ;

    [[ noreturn ]] void throwException(const std::string msg) ;

private:

    struct Position {
        Position(istream &src): end_() { src.unsetf(ios::skipws) ; cursor_ = src ; }

        Position(const Position &other): column_(other.column_), line_(other.line_), cursor_(other.cursor_), end_(other.end_) {}

        operator bool () const { return cursor_ != end_ ; }
        char operator * () const { return *cursor_ ; }
        Position& operator++() { advance(); return *this ; }
        Position operator++(int) {
            Position p(*this) ;
            advance() ;
            return p ;
        }

        void advance() {
            // skip new line characters
            column_++ ;

            if ( cursor_ != end_ && *cursor_ == '\n' ) {
                column_ = 1 ; line_ ++ ;
            }

            cursor_ ++ ;

        }

        istream_iterator<char> cursor_, end_ ;
        uint column_ = 1;
        uint line_ = 1;
    } ;

    istream &src_ ;
    Position pos_ ;

    void skipSpace() ;
    bool expect(char c) ;
    bool expect(const char *str) ;
    void parseString(char c) ;
    void parseNumber() ;
    bool decodeUnicode(uint &cp) ;
    static string unicodeToUTF8(uint cp) ;
    void parseLiteral() ;

};


bool JSONTokenizer::decodeUnicode(uint &cp)
{
    int unicode = 0 ;

    for( uint i=0 ; i<4 ; i++ ) {
        if ( !pos_ ) return false ;
        char c = *pos_++ ;
        unicode *= 16 ;
        if ( c >= '0' && c <= '9')
            unicode += c - '0';
        else if (c >= 'a' && c <= 'f')
            unicode += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            unicode += c - 'A' + 10;
        else
            return false ;
    }

    cp = static_cast<unsigned int>(unicode);
    return true;
}

 void JSONTokenizer::throwException(const string msg)
{
    throw JSONParseException(msg, pos_.line_, pos_.column_) ;

}

/// Converts a unicode code-point to UTF-8.

string JSONTokenizer::unicodeToUTF8(unsigned int cp) {
    string result ;

    if ( cp <= 0x7f ) {
        result.resize(1);
        result[0] = static_cast<char>(cp);
    } else if ( cp <= 0x7FF ) {
        result.resize(2);
        result[1] = static_cast<char>(0x80 | (0x3f & cp));
        result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
    } else if ( cp <= 0xFFFF ) {
        result.resize(3);
        result[2] = static_cast<char>(0x80 | (0x3f & cp));
        result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
        result[0] = static_cast<char>(0xE0 | (0xf & (cp >> 12)));
    } else if ( cp <= 0x10FFFF ) {
        result.resize(4);
        result[3] = static_cast<char>(0x80 | (0x3f & cp));
        result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
        result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
        result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
    }

    return result;
}


void JSONTokenizer::skipSpace() {
    while ( pos_ ) {
        char c = *pos_ ;
        if ( isspace(c) ) ++pos_ ;
        else return ;
   }
}

bool JSONTokenizer::expect(char c) {
    if ( pos_ ) {
        if ( *pos_ == c ) {
            ++pos_ ;
            return true ;
        }
        return false ;
    }
    return false ;
}

void JSONTokenizer::parseString(char startc)
{
    static const char *msg_unterminated = "End of file while parsing string literal" ;
    static const char *msg_decoding_error = "Error while decoding unicode code point" ;
    static const char *msg_invalid_char = "Invalid character found while decoding string literal" ;

    string &res = token_string_literal_ ;
    res.clear() ;

    while ( pos_ ) {
        char c = *pos_ ;
        if ( c == startc ) {
            pos_++ ;
            return ;
        }
        else if ( c == '\\' ) {
            if ( !pos_ )  throwException(msg_unterminated);
            char escape = *pos_++ ;

            switch (escape) {
            case '"':
                res += '"'; break ;
            case '/':
                res += '/'; break ;
            case '\\':
                res += '\\'; break;
            case 'b':
                res += '\b'; break ;
            case 'f':
                res += '\f'; break ;
            case 'n':
                res += '\n'; break ;
            case 'r':
                res += '\r'; break ;
            case 't':
                res += '\t'; break ;
            case 'u': {
                unsigned int cp ;
                if ( !decodeUnicode(cp) ) throwException(msg_decoding_error) ;
                res += unicodeToUTF8(cp);
            } break;
            default:
                 throwException(msg_invalid_char) ;
            }

        } else {
            res += c;
            ++pos_ ;
        }
    }

}

bool JSONTokenizer::expect(const char *str)
{
    const char *c = str ;

    Position cur = pos_ ;
    while ( *c != 0 ) {
        if ( !expect(*c) ) {
            pos_ = cur ;
            return false ;
        }
        else ++c ;
    }
    return true ;
}

void JSONTokenizer::parseNumber() {

    static const char *msg_invalid = "invalid number literal" ;

    string num_str ;

    char c = *pos_ ;
    if ( c == '-' ) {
        num_str += c ;
        c = *++pos_ ;
    }
    if ( c == '0') {
        num_str += c ;
        c = *++pos_ ;
    }
    else if ( c >= '1' && c <= '9' ) {
        num_str += c ;
        c = *++pos_ ;
        while ( c >= '0' && c <= '9') {
            num_str += c ;
            c = *++pos_  ;
        }

    } else {
      throwException(msg_invalid) ;
    }

    if ( c == '.') {
        num_str += c ;
        c = *++pos_ ;

        while ( c >= '0' && c <= '9') {
            num_str += c ;
            c = *++pos_ ;
        }
    }

    if ( c == 'e' || c == 'E') {
        num_str += c ;
        c = *++pos_ ;
        if ( c == '+' || c == '-' ) {
            num_str += c ;
            c = *++pos_ ;
        }
        if ( c >= '0' && c <= '9') {
            num_str += c ;
            c = *++pos_ ;
            while ( c >= '0' && c <= '9' ) {
                num_str += c ;
                c = *++pos_ ;
            }
        } else {
            throwException(msg_invalid) ;
        }
    }

    try {
        size_t idx ;
        int64_t number = stoi(num_str, &idx) ;
        if ( idx == num_str.length() ) {
            token_number_literal_.is_integer_ = true ;
            token_number_literal_.value_.int_value_ = number ;
        } else {
            double number = stod(num_str) ;
            token_number_literal_.is_integer_ = false ;
            token_number_literal_.value_.double_value_ = number ;
        }
    }
    catch (invalid_argument & ) {
        throwException(msg_invalid) ;
    }
}

Token JSONTokenizer::nextToken() {

    static const char *msg_unknown_token = "unknown token" ;
    skipSpace() ;

    if ( !pos_ ) return EOF_DOCUMENT ;
    switch (*pos_ )  {
        case '{':
            ++pos_ ;
            return OPEN_BRACE ;
        case '[':
           ++pos_ ;
            return OPEN_BRACKET ;
        case '}':
            ++pos_ ;
            return CLOSE_BRACE ;
        case ']':
            ++pos_ ;
            return CLOSE_BRACKET ;
        case '\'':
        case '"':
           parseString(*pos_++) ;
           return STRING ;
        case 't':
        case 'T':
            if ( expect("true") || expect("TRUE")) {
                token_boolean_literal_ = true ;
                return BOOLEAN;
            } else throwException(msg_unknown_token) ;

        case 'f':
        case 'F':
            if ( expect("false") || expect("FALSE")) {
                token_boolean_literal_ = false ;
                return BOOLEAN;
            } else throwException(msg_unknown_token) ;


        case 'n':
        case 'N':
            if ( expect("null") || expect("NULL") ) {
                return JSON_NULL;
            } else throwException(msg_unknown_token) ;

        case ':':
            ++pos_ ;
            return COLON ;
        case ',':
            ++pos_ ;
            return COMMA ;
        default:
            parseNumber() ;
            return NUMBER ;
    }
}

void JSONTokenizer::expectToken(Token tk, const char *msg)
{
    Position p = pos_ ;
    Token t = nextToken() ;
    if ( t != tk ) {
        pos_ = p ;
        throwException(msg) ;
    }
}

static std::string token_to_string(int tk) {
    switch (tk) {
    case OPEN_BRACE: return "{" ;
    case OPEN_BRACKET: return "[" ;
    case CLOSE_BRACKET: return "]" ;
    case CLOSE_BRACE: return "}" ;
    case COLON: return ":" ;
    case COMMA: return "," ;
    case NUMBER: return "<number>" ;
    case STRING: return "<string>" ;
    case BOOLEAN: return "<boolean>" ;
    case JSON_NULL: return "<null>" ;
    default: return "" ;
    }
}

////////////////////////////////////////////////////////////////////////////////////////

JSONParseException::JSONParseException(const string &msg, uint line, uint col) {
    std::ostringstream strm ;
    strm << msg << ", at line " << line << ", column " << col ;
    msg_ = strm.str() ;
}

JSONReader::JSONReader(istream &strm)
{
    tokenizer_.reset(new JSONTokenizer(strm)) ;
    current_token_ = JSONToken::UNDEFINED ;
    state_stack_.push(EMPTY_DOCUMENT) ;
}

void JSONReader::beginObject()
{
    auto t = peek() ;
    if ( t != JSONToken::BEGIN_OBJECT ) tokenizer_->throwException("expecting object") ;
    advance() ;
}

void JSONReader::endObject()
{
    auto t = peek() ;
    if ( t != JSONToken::END_OBJECT ) tokenizer_->throwException("unterminated object") ;
    advance() ;
}


void JSONReader::beginArray()
{
    auto t = peek() ;
    if ( t != JSONToken::BEGIN_ARRAY ) tokenizer_->throwException("expecting array") ;
    advance() ;
}

void JSONReader::endArray()
{
    auto t = peek() ;
    if ( t != JSONToken::END_ARRAY ) tokenizer_->throwException("unterminated array") ;
    advance() ;
}

void JSONReader::advance() {
    current_token_ = JSONToken::UNDEFINED ;
}

bool JSONReader::hasNext() {
    auto token = peek();
    return ( token != JSONToken::END_OBJECT && token != JSONToken::END_ARRAY );
}

bool JSONReader::nextBoolean()
{
     auto token = peek();
     advance() ;
     if ( token == JSONToken::BOOLEAN ) {
         return tokenizer_->token_boolean_literal_ ;
     } else
         tokenizer_->throwException("expecting boolean literal") ;

}

int64_t JSONReader::nextInt()
{
     auto token = peek();
     advance() ;
     if ( token == JSONToken::NUMBER) {
         return tokenizer_->token_number_literal_.toInt() ;
     } else
         tokenizer_->throwException("expecting number literal") ;
}

void JSONReader::nextNull()
{
    auto token = peek();
    advance() ;
    if ( token == JSONToken::JSON_NULL ) {

    } else
        tokenizer_->throwException("expecting null literal") ;

}

string JSONReader::nextString()
{
     auto token = peek();
     advance() ;
     if ( token == JSONToken::STRING) {
         return tokenizer_->token_string_literal_ ;
     } else
         tokenizer_->throwException("expecting string literal") ;
}

double JSONReader::nextDouble()
{
     auto token = peek();
     advance() ;
     if ( token == JSONToken::NUMBER) {
         return tokenizer_->token_number_literal_.toDouble() ;
     } else
         tokenizer_->throwException("expecting number literal") ;
}

string JSONReader::nextName()  {
    auto token = peek();
    advance() ;
    if ( token != JSONToken::NAME )
        tokenizer_->throwException("expecting name") ;

    return tokenizer_->token_string_literal_ ;
}

void JSONReader::skipValue() {
    if ( !hasNext() || peek() == JSONToken::END_DOCUMENT ) {
        tokenizer_->throwException("no element left to skip") ;
    }

    int count = 0;
    do {
      auto token = peek() ;
      advance();

      if ( token == JSONToken::BEGIN_ARRAY || token == JSONToken::BEGIN_OBJECT) {
           count++;
      } else if ( token == JSONToken::END_ARRAY || token == JSONToken::END_OBJECT) {
           count--;
      }
    } while ( count != 0 );
}

JSONToken JSONReader::nextValue(int tk) {

    switch ( tk ) {
        case OPEN_BRACE:
            state_stack_.push(EMPTY_OBJECT) ;
            return JSONToken::BEGIN_OBJECT ;
        case OPEN_BRACKET:
            state_stack_.push(EMPTY_ARRAY) ;
            return JSONToken::BEGIN_ARRAY ;
        case JSON_NULL:
            return JSONToken::JSON_NULL ;
        case NUMBER:
            return JSONToken::NUMBER ;
        case BOOLEAN:
            return JSONToken::BOOLEAN ;
        case STRING:
            return JSONToken::STRING ;
    default:
        tokenizer_->throwException("unexpected token " + token_to_string(tk)) ;
    }
}


JSONToken JSONReader::peek()
{
    if ( current_token_ != JSONToken::UNDEFINED ) {
        return current_token_  ;
    }

    State state = state_stack_.top() ;

    if ( state == EMPTY_DOCUMENT ) {
        state_stack_.top() = NON_EMPTY_DOCUMENT ;

        auto tk = tokenizer_->nextToken() ;
        current_token_ = nextValue(tk) ;

        if ( current_token_ != JSONToken::BEGIN_ARRAY && current_token_ != JSONToken::BEGIN_OBJECT ) {
            tokenizer_->throwException("Expected JSON document to start with '[' or '{'");
        }

        return current_token_ ;

    } else if ( state == EMPTY_OBJECT || state == NON_EMPTY_OBJECT ) {
        auto tk = tokenizer_->nextToken() ;

        if ( state == EMPTY_OBJECT ) {
            if ( tk == CLOSE_BRACE ) {
                state_stack_.pop() ;
                return current_token_ = JSONToken::END_OBJECT ;
            }
        } else {
            if ( tk == CLOSE_BRACE ) {
                state_stack_.pop() ;
                return current_token_ = JSONToken::END_OBJECT ;
            } else if ( tk == COMMA ) {
                tk = tokenizer_->nextToken() ;
            } else {
                tokenizer_->throwException("unexpected token " + token_to_string(tk)) ;
            }
        }

        state_stack_.top() = NON_EMPTY_OBJECT ;
        // read name

        if ( tk == STRING ) {
            state_stack_.push(DANGLING_NAME) ;
            return current_token_ = JSONToken::NAME ;
        } else
            tokenizer_->throwException("unexpected token " + token_to_string(tk)) ;

    } else if ( state == EMPTY_ARRAY || state == NON_EMPTY_ARRAY ) {
        auto tk = tokenizer_->nextToken() ;

        if ( state == EMPTY_ARRAY ) {
            if ( tk == CLOSE_BRACKET ) {
                state_stack_.pop() ;
                return current_token_ = JSONToken::END_ARRAY ;
            }
        } else {
            if ( tk == CLOSE_BRACKET ) {
                state_stack_.pop() ;
                return current_token_ = JSONToken::END_ARRAY ;
            } else if ( tk == COMMA ) {
                tk = tokenizer_->nextToken() ;
            } else {
                tokenizer_->throwException("unexpected token " + token_to_string(tk)) ;
            }
        }

        state_stack_.top() = NON_EMPTY_ARRAY ;
        // read value

        return current_token_ = nextValue(tk) ;
    } else if ( state == DANGLING_NAME ) {
        auto tk = tokenizer_->nextToken() ;

        if ( tk == COLON ) {
            state_stack_.pop() ;
            tk = tokenizer_->nextToken() ;
            return current_token_ = nextValue(tk) ;
        } else {
            tokenizer_->throwException("unexpected token " + token_to_string(tk)) ;
        }
    } else if ( state == NON_EMPTY_DOCUMENT ) {
        state_stack_.pop() ;
        auto tk = tokenizer_->nextToken() ;

        if ( tk != EOF_DOCUMENT )
            tokenizer_->throwException("unexpected token " + token_to_string(tk)) ;

        return current_token_ = JSONToken::END_DOCUMENT ;
    }

    assert(true) ;
    return current_token_ = JSONToken::UNDEFINED ; // should not end up here
}


}
