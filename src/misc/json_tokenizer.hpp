#pragma once

#include <cstdint>
#include <string>
#include <istream>
#include <iterator>

namespace cvx {
namespace detail {

enum Token { TOKEN_IDENTIFIER, TOKEN_OPEN_BRACE, TOKEN_CLOSE_BRACE, TOKEN_OPEN_BRACKET, TOKEN_CLOSE_BRACKET,
             TOKEN_STRING, TOKEN_BOOLEAN, TOKEN_NUMBER, TOKEN_JSON_NULL, TOKEN_COMMA, TOKEN_COLON, TOKEN_EMPTY, TOKEN_EQUAL, TOKEN_EOF_DOCUMENT, TOKEN_INCLUDE } ;

class JSONTokenizer {
public:

    JSONTokenizer(std::istream &src, bool extended = false): src_(src), pos_(src), extended_(extended) {
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

    void throwException(const std::string msg) ;

    static std::string token_to_string(int tk) ;

private:

    struct Position {
        Position(std::istream &src): end_() { src.unsetf(std::ios::skipws) ; cursor_ = src ; }

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

        std::istream_iterator<char> cursor_, end_ ;
        uint column_ = 1;
        uint line_ = 1;
    } ;

    std::istream &src_ ;
    Position pos_ ;
    bool extended_ ;

    void skipSpace() ;
    bool expect(char c) ;
    bool expect(const char *str) ;
    void parseString(char c) ;
    void parseIdentifier() ;
    void parseIncludePath() ;
    void parseNumber() ;
    bool decodeUnicode(uint &cp) ;
    static std::string unicodeToUTF8(uint cp) ;
    void parseLiteral() ;

};

class JSONTokenizerParseException: public std::exception {
public:

    JSONTokenizerParseException(const std::string &msg, uint line, uint col);

    const char *what() const noexcept override {
        return msg_.c_str() ;
    }
protected:
    std::string msg_ ;
};

}}
