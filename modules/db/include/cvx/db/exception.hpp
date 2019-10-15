#ifndef CVX_DB_EXCEPTION_HPP
#define CVX_DB_EXCEPTION_HPP

#include <string>
#include <stdexcept>

namespace cvx { namespace db {

class ConnectionHandle ;

class Exception: public std::runtime_error
{
public:
    Exception(const std::string &msg) ;
    Exception(ConnectionHandle &h) ;
};


}
}



#endif
