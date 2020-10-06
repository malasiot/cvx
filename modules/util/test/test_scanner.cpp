#include <iostream>
#include <vector>
#include <sstream>
#include <cassert>
#include <functional>

class Scanner {
public:
    Scanner(std::istream &strm): strm_(strm) {}

    // get single character
    char read() {
        assert (strm_.hasMore()) ;
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
        size_t pos = strm_.position() ;
        bool match = true ;
        const char *p = seq ;

        while ( strm_.hasMore() && *p ) {
            char c = strm_.read() ;
            if ( *p != c ) {
                match = false ;
                strm_.setPosition(pos) ;
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
            }
        }
    }

    std::string nextToken() {

        std::string token ;
        skipDelimeters();

        while ( hasMore() ) {
            char c = strm_.read() ;
            if ( delimeter_(c) ) break ;
            token.push_back(c) ;
        }

        return token ;
    }

    bool expectDouble(double &v) {

        size_t anchor = strm_.position() ;

        enum state { sign, asign, a, b, esign, e, end, error } ;

        state state = sign ;

        std::string n_str ;

        while ( hasMore() ) {

            char c = read() ;

            if ( state == sign )  {
                if ( c == '+' || c == '-' ) {
                    state = asign ;
                } else if ( isdigit(c) ) {
                    state = a ;
                } else if ( c == '.' ) {
                    state = b ;
                }
            } else if ( state == asign ) {
                if ( c == '.' ) {
                    state = b ;
                } else if ( isdigit(c) ) {
                    state = a ;
                } else {
                    state = error ;
                }
            } else if ( state == a ) {
                if ( isdigit(c) ) {
                } else if ( c == '.' )  {
                    state = b ;
                } else {
                    state = end ;
                }
            } else if ( state == b ) {
                if ( isdigit(c) ) {
                } else if ( ( c == 'e' || c == 'E' )  ) {
                    state = esign ;
                } else {
                    if ( isdigit(n_str.back()) )
                        state = end ;
                    else
                        state = error;
                }
            } else if ( state == esign ) {
                if ( c == '+' || c == '-' || isdigit(c) ) {
                    state = e ;
                } else
                    state = error ;
            } else if ( state == e ) {
                if ( isdigit(c) ) ;
                else state = end ;
            }

            if ( state == error ) {
                strm_.setPosition(anchor) ;
                return false ;
            } else if ( state == end ) {
                strm_.putBack() ;
                v = atof(n_str.c_str()) ;
                return true ;
            } else {
                n_str += c ;
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

    StreamWrapper strm_ ;

};


using namespace std ;

int main(int argc, char *argv[]) {

    istringstream strm("10.4e01 etet") ;
    Scanner s(strm) ;

    double v ;
    s.expectDouble(v) ;
    cout <<v << endl ;
    while (s.hasMore() ) {
        string token = s.nextToken() ;
        cout << token << endl ;
    }


}
