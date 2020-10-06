#include <iostream>
#include <vector>
#include <sstream>
#include <cassert>
#include <functional>

class Scanner {
public:
    Scanner(std::istream &strm): strm_(strm) {}

    char getChar() {
        assert (strm_.hasMore()) ;
        return strm_.read() ;
    }

    bool hasMore() { return strm_.hasMore() ; }

private:


    struct StreamWrapper {
        StreamWrapper(std::istream &strm): strm_(strm) {
            buffer_.reserve(buffer_size_increment_) ;
            current_ = 0 ;
            end_ = current_ + buffer_.size() ;
            limit_ = current_ ;
            fetchMore() ;
        }

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

        size_t buffer_size_increment_ = 14 ;
        std::vector<char> buffer_ ;
        size_t fetch_size_ = 12 ;
        uint current_, end_, limit_ ;
        std::istream &strm_ ;
        bool has_more_ = true ;
    };

    StreamWrapper strm_ ;
};


using namespace std ;

int main(int argc, char *argv[]) {

    istringstream strm("tt 1.45 kdkk dkkkd") ;
    Scanner s(strm) ;

    while ( s.hasMore() ) {
        auto c = s.getChar() ;
        cout << c ;
    }

}
