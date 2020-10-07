#include <iostream>
#include <vector>
#include <sstream>
#include <cassert>
#include <functional>

class Scanner {
public:
    Scanner(std::istream &strm): strm_(strm) {}

    // get a single character (this should not be a delimeter)
    char read() {
        assert (strm_.hasMore()) ;
        column_++ ;
        return strm_.read() ;
    }

    char peek() {
        return strm_.peek() ;
    }

    // input has more characters
    bool hasMore() { return strm_.hasMore() ; }

    // consume characters as long as they match seq otherwise return false and
    // backtrack the scanner

    bool expect(const char *seq) {
        saveState() ;

        bool match = true ;
        const char *p = seq ;

        while ( strm_.hasMore() && *p ) {
            char c = strm_.read() ;
            column_ ++ ;
            if ( *p != c ) {
                match = false ;
                restoreState() ;
                break ;
            }
            ++p ;
        }

        return match ;
    }

    void skipDelimeters() {
        while ( hasMore() ) {
            char c = strm_.read() ;

            if ( !delimeter_(c) ) {
                strm_.putBack() ;
                break ;
            } else if ( c == '\n' ) {
                line_ ++ ;
                column_ = 0 ;
            } else {
                column_++ ;
            }
        }
    }

    // get a delimeted token
    std::string nextToken() {

        std::string token ;
        skipDelimeters();

        while ( hasMore() ) {
            char c = strm_.read() ;

            if ( delimeter_(c) ) {
                strm_.putBack() ;
                break ;
            }
            else
                token.push_back(c) ;
        }

        column_ += token.size() ;
        return token ;
    }

    // parse floating point number

    bool expectDouble(double &v, bool &is_decimal) {

        saveState() ;

        is_decimal = false ;

        enum state { S_BEGIN, S_INF, S_NAN, S_AFTER_SIGN, S_DEC, S_FRAC, S_ESIGN, S_EXP, S_END, S_ERROR } ;

        state state = S_BEGIN ;

        std::string n_str ;

        while ( hasMore() ) {

            char c = read() ;

            switch ( state ) {
            case S_BEGIN: {
                if ( c == 'i' || c == 'I' ) state = S_INF ;
                else if ( c == 'n' || c == 'N' ) state = S_NAN ;
                else if ( c == '+' || c == '-' ) state = S_AFTER_SIGN ;
                else if ( isdigit(c) ) state = S_DEC ;
                else if ( c == '.' ) state = S_FRAC ;
                else state = S_ERROR ;
                break ;
            }
            case S_AFTER_SIGN: {
                if ( c == '.' ) state = S_FRAC ;
                else if ( isdigit(c) ) state = S_DEC ;
                else state = S_ERROR ;
                break ;
            }
            case S_DEC: {
                if ( isdigit(c) ) ;
                else if ( c == '.' ) state = S_FRAC ;
                else {
                    is_decimal = true ;
                    strm_.putBack() ;
                    v = atoi(n_str.c_str()) ;
                    return true ;
                }
                break ;
            }
            case S_FRAC: {
                if ( isdigit(c) ) ;
                else if ( ( c == 'e' || c == 'E' ) ) state = S_ESIGN ;
                else if ( n_str.size() > 1 ) state = S_END ;
                else state = S_ERROR ;

                break ;
            }
            case S_ESIGN: {
                if ( c == '+' || c == '-' || isdigit(c) ) state = S_EXP ;
                else state = S_ERROR ;
                break ;
            }
            case S_EXP: {
                if ( isdigit(c) ) ;
                else state = S_END;
                break ;
            }
            case S_NAN: {
                size_t n = n_str.size() ;
                if ( n == 1 && ( c == 'a' || c == 'A' )) ;
                else if ( n == 2 && ( c == 'n' || c == 'N') ) ;
                else if ( n == 3 ) state = S_END ;
                else state = S_ERROR ;
                break ;
            }
            case S_INF: {
                size_t n = n_str.size() ;
                if ( n == 1 && ( c == 'n' || c == 'N' )) ;
                else if ( n == 2 && ( c == 'f' || c == 'F') ) ;
                else if ( n == 3 ) state = S_END ;
                else state = S_ERROR ;
                break ;
            }

            default:
                break ;

            }

            if ( state == S_ERROR )  {
                restoreState() ;
                return false ;
            } else if ( state == S_END ) {
                strm_.putBack() ;
                v = atof(n_str.c_str()) ;
                return true ;
            } else {
                n_str += c ;
                column_ ++ ;
            }
        }

        return true ;
    }

private:

    struct WhiteSpaceDelimeter {
    public:
        bool operator() (char c) const {
            switch ( c ) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                return true ;
            default:
                return false ;
            }
        }
    };

    // The delimeter functor will return true of the input character belongs to delimeters
    std::function<bool (char)> delimeter_ = WhiteSpaceDelimeter() ;

    struct StreamWrapper {
        StreamWrapper(std::istream &strm): strm_(strm) {
            buffer_.reserve(buffer_size_increment_ + 1) ;
            buffer_.back() = 0 ;
            current_ = 0 ;
            end_ = current_ + buffer_.size() ;
            limit_ = current_ ;
            fetchMore() ;
        }

        const char *ptr() const { return &buffer_[current_] ; }
        size_t position() const { return current_ ; }

        void setPosition(size_t pos) {
            assert(pos < limit_) ;
            current_ = pos ;
        }

        char peek() { return buffer_[current_] ; }

        void fetchMore( ) {
            if ( limit_ < end_ ) {
                size_t sz = std::min(fetch_size_, static_cast<size_t>(end_ - limit_)) ;
                strm_.read(&buffer_[limit_], sz) ;
                size_t rd = strm_.gcount() ;
                has_more_ = rd > 0 ;
                limit_ += rd ;
            } else { // we need to resize the buffer
                buffer_.resize(buffer_.size() + buffer_size_increment_) ;
                end_ += buffer_size_increment_ ;
                buffer_.back() = 0 ;
                fetchMore() ;
            }

        }

        char read() {
            char c = buffer_[current_++] ;
            if ( current_ == limit_ ) fetchMore() ;
            return c ;
        }


        bool hasMore() const {
            return has_more_ ;
        }

        void putBack(int c = 1) {
            setPosition(current_ - c) ;
        }

        size_t buffer_size_increment_ = 14 ;
        std::vector<char> buffer_ ;
        size_t fetch_size_ = 12 ;
        size_t current_, end_, limit_ ;
        std::istream &strm_ ;
        bool has_more_ = true ;
    };


    struct State {
        size_t position_ = 0 ;
        size_t column_ = 0 ;
        size_t line_ = 0 ;
    };

    void saveState() {
        anchor_.position_ = strm_.position() ;
        anchor_.column_= column_ ;
        anchor_.line_ = line_ ;
    }

    void restoreState() {
        strm_.setPosition(anchor_.position_) ;
        column_ = anchor_.column_ ;
        line_ = anchor_.line_ ;
    }


    StreamWrapper strm_ ;
    State anchor_ ;
    size_t column_ = 0, line_ = 0  ;

};

std::string parseString(Scanner &s) {
    char startc = s.read() ;

    std::string res ;
    bool is_escape = false ;
    char c ;
    while ( s.hasMore() ) {
        c = s.read() ;

        if ( c == startc ) {
            return res ;
        }
        else if ( c == '\\' ) {
            is_escape = true ;
        } else if ( is_escape ) {
            switch (c) {
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
                 return std::string() ;
            }
            is_escape = false ;
        } else {
            res += c ;
        }
    }
    if ( c != startc ) return std::string() ;

    return res ;
}

using namespace std ;

int main(int argc, char *argv[]) {

    istringstream strm("2. \"et\"45'et\" dd") ;
    Scanner s(strm) ;

    while ( s.hasMore() ) {
        s.skipDelimeters() ;
        char c = s.peek() ;
        if ( c == '\'' || c == '"' ) cout << parseString(s) << endl ;
        else cout << s.nextToken() << endl ;
    }


}
