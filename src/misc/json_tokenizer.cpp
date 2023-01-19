#include "json_tokenizer.hpp"

#include <sstream>
#include <cctype>

using namespace std ;

namespace cvx {
namespace detail {

JSONTokenizerParseException::JSONTokenizerParseException(const string &msg, uint line, uint col) {
    std::ostringstream strm ;
    strm << msg << ", at line " << line << ", column " << col ;
    msg_ = strm.str() ;
}

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

void JSONTokenizer::throwException(const string msg) {
    throw JSONTokenizerParseException(msg, pos_.line_, pos_.column_) ;

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
        else if ( extended_ ) {
            char pc ;
            if ( c == '#' ) {
                while ( pos_ && *pos_ != '\n') {
                    ++pos_ ;
                }
            } else if ( c == '/' ) {
                ++pos_ ;
                c = *pos_ ;
                if ( c == '/' ) {
                    while ( pos_ && *pos_ != '\n') {
                        ++pos_ ;
                    }
                } else if ( c == '*' ) {
                    ++pos_ ;
                    pc = *pos_++ ;
                    while ( pos_ && *pos_ != '/' && pc != '*' )
                        pc = *pos_++ ;
                }
            } else if ( c == ';' ) ++pos_ ;
        }
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

void JSONTokenizer::parseIdentifier()   {
    static const char *msg_invalid_char = "Invalid character in name" ;

    string &res = token_string_literal_ ;
    res.clear() ;

    res += *pos_++ ;

    while ( pos_ ) {
        char c = *pos_ ;
        if ( std::isalpha(c) || c == '_' ) {
            res += c ;
            ++pos_ ;
        }
        else if ( std::isspace(c) || c == '=' || c == ':' )
            break ;
        else
            throwException(msg_invalid_char);
    }
}

void JSONTokenizer::parseIncludePath() {
    static const char *msg_invalid_char = "Invalid character while parsing include path" ;
    static const char *msg_unterminated = "End of file/line while parsing include path" ;
    static const char *msg_trailing_chars = "Trailing characters after include directive" ;

    skipSpace() ;

    string &res = token_string_literal_ ;
    res.clear() ;


    char startc = *pos_ ;

    if ( startc != '"' && startc != '\'' )
        throwException(msg_invalid_char);

    ++pos_ ;

    while ( pos_ ) {
        char c = *pos_ ;
        if ( !pos_ || c == '\n' )  throwException(msg_unterminated);
        if ( c == startc ) {
            pos_++ ;
            break ;
        }
        else if ( c == '\\' ) {
            if ( !pos_ || c == '\n' )  throwException(msg_unterminated);
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
            default:
                throwException(msg_invalid_char) ;
            }

        } else {
            res += c;
            ++pos_ ;
        }
    }

    int line = pos_.line_ ;
    skipSpace() ;
    if ( pos_.line_ == line )
        throwException(msg_trailing_chars);

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

static bool is_number_prefix(char c) {
    if ( std::isdigit(c) ) return true ;
    if ( c == '+' || c == '-' || c == '.' ) return true ;
    return false ;
}

Token JSONTokenizer::nextToken() {

    static const char *msg_unknown_token = "unknown token" ;
    skipSpace() ;

    if ( !pos_ ) return TOKEN_EOF_DOCUMENT ;
    switch (*pos_ )  {
    case '{':
        ++pos_ ;
        return TOKEN_OPEN_BRACE ;
    case '[':
        ++pos_ ;
        return TOKEN_OPEN_BRACKET ;
    case '}':
        ++pos_ ;
        return TOKEN_CLOSE_BRACE ;
    case ']':
        ++pos_ ;
        return TOKEN_CLOSE_BRACKET ;
    case '=':
        if ( extended_ ) {
            ++pos_ ;
            return TOKEN_EQUAL ;
        } else
            throwException(msg_unknown_token);
    case '\'':
    case '"':
        parseString(*pos_++) ;
        return TOKEN_STRING ;
    case '@':
        if ( extended_ ) {
            if ( expect("@include") ) {
                parseIncludePath() ;
                return TOKEN_INCLUDE;
            } else
                throwException(msg_unknown_token);
        } else
            throwException(msg_unknown_token);
        break ;
    case 't':
        if ( expect("true") ) {
            token_boolean_literal_ = true ;
            return TOKEN_BOOLEAN;
        } else {
            if ( extended_ ) {
                parseIdentifier() ;
                return TOKEN_IDENTIFIER ;
            }
            else
                throwException(msg_unknown_token) ;
        }
        break ;

    case 'f':
        if ( expect("false") ) {
            token_boolean_literal_ = false ;
            return TOKEN_BOOLEAN;
        } else {
            if ( extended_ ) {
                parseIdentifier() ;
                return TOKEN_IDENTIFIER ;
            }
            else
                throwException(msg_unknown_token) ;
        }
        break ;
    case 'n':
        if ( expect("null") ) {
            return TOKEN_JSON_NULL;
        } else {
            if ( extended_ ) {
                parseIdentifier() ;
                return TOKEN_IDENTIFIER ;
            }
            else
                throwException(msg_unknown_token) ;
        }
        break ;
    case ':':
        ++pos_ ;
        return TOKEN_COLON ;
    case ',':
        ++pos_ ;
        return TOKEN_COMMA ;
    default:
        if ( std::isalpha(*pos_) && extended_ ) {
            parseIdentifier();
            return TOKEN_IDENTIFIER ;
        } else if ( is_number_prefix(*pos_) ) {
            parseNumber() ;
            return TOKEN_NUMBER ;
        }
        else
            throwException(msg_unknown_token) ;

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

std::string JSONTokenizer::token_to_string(int tk) {
    switch (tk) {
    case TOKEN_OPEN_BRACE: return "{" ;
    case TOKEN_OPEN_BRACKET: return "[" ;
    case TOKEN_CLOSE_BRACKET: return "]" ;
    case TOKEN_CLOSE_BRACE: return "}" ;
    case TOKEN_COLON: return ":" ;
    case TOKEN_COMMA: return "," ;
    case TOKEN_NUMBER: return "<number>" ;
    case TOKEN_STRING: return "<string>" ;
    case TOKEN_BOOLEAN: return "<boolean>" ;
    case TOKEN_JSON_NULL: return "<null>" ;
    case TOKEN_IDENTIFIER: return "<identifier>" ;
    case TOKEN_EQUAL: return "=" ;
    case TOKEN_INCLUDE: return "@include" ;
    default: return "" ;
    }
}
}
}
